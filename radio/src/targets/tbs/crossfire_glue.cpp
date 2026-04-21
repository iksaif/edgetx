/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

// Thin modern replacement for the legacy radio/src/io/crsf/crossfire.cpp —
// just the TBS-side glue functions the rest of the firmware needs to talk
// to the CRSF blob through CrossfireSharedData at 0x10000000.
//
// The legacy crossfire.cpp also carried:
//   - libCrsf port routing (moved to crsfInit here, via libCrsfAddDeviceFunctionList)
//   - BKPSRAM-based readBackupReg / writeBackupReg (F413 doesn't have BKPSRAM;
//     tbs/board.cpp provides RTC-backup-register equivalents)
//   - a SD-card debug protocol (LIBCRSF_ENABLE_SD, TBS-internal; removed)
//   - the RF-off confirmation popup using removed STR_* translations and the
//     old getEvent()/warningResult API (Phase B GUI work — move to caller)
//
// Functions here are ONLY called when the blob is running. The
// !crossfireSharedData.rtosApiVersion check (populated by tbsCrsfSharedDataInit
// before the blob starts) guards against callers hitting these on a
// no-blob build.

#include <string.h>

#include "board.h"
#include "edgetx.h"
#include "stamp.h"         // VERSION_MAJOR/MINOR/REVISION
#include "fifo.h"
#include "hal/watchdog_driver.h"

#include "io/crsf/crsf.h"
#include "io/crsf/crossfire.h"
#include "io/crsf/crsf_write.h"
#include "io/crsf/crsf_utilities.h"

// Backing FIFO for telemetry frames targeted at DEVICE_INTERNAL.
// crossfire.h declares this extern elsewhere in the tree; owning the
// definition here keeps it paired with the handler that fills it.
extern Fifo<uint8_t, TELEMETRY_FIFO_SIZE> intCrsfTelemetryFifo;

// Forward declarations must match the signatures libCrsfPort expects
// (extern linkage, NOT static — the libCrsfPort table is passed to
// libCrsfAddDeviceFunctionList which stores the pointers).
void crsfToSharedFIFO(uint8_t* pArr);
void crsfThisDevice(uint8_t* pArr);

static libCrsfPort libCrsfPorts[] = {
  { DEVICE_INTERNAL,  &crsfThisDevice   },
  { CRSF_SHARED_FIFO, &crsfToSharedFIFO },
};

uint8_t  libCrsfMySlaveAddress = 0;
char*    libCrsfMyDeviceName = nullptr;
uint32_t libCrsfMyHwID = 0;
uint32_t libCrsfMySerialNo = 0;
uint32_t libCrsfMyFwID = 0;
uint8_t  currentCrsfModelId = 0;

// tbsCrsfSharedDataInit() in board.cpp has already zeroed the struct,
// set rtosApiVersion and the trampoline. We only need to register the
// port routing with libcrsf here.
void crsfInit()
{
  uint32_t fw_id = (VERSION_MAJOR << 8 | (VERSION_MINOR * 16)) + VERSION_REVISION;
  uint32_t hw_id = readBackupReg(BKREG_HW_ID_RADIO);
  uint32_t serial_num = readBackupReg(BKREG_SERIAL_NO_RADIO);
  writeBackupReg(BKREG_HW_ID_RADIO, 0);
  writeBackupReg(BKREG_SERIAL_NO_RADIO, 0);

  libCrsfInit(LIBCRSF_REMOTE_ADD, (char*)MY_DEVICE_NAME, serial_num, hw_id, fw_id);
  libCrsfAddDeviceFunctionList(&libCrsfPorts[0],
                               sizeof(libCrsfPorts) / sizeof(libCrsfPorts[0]));
}

uint32_t crsfGetHWID()
{
  return readBackupReg(BKREG_HW_ID_XF);
}

// TBS RF on/off is toggled by setting a flag in crossfireSharedData and
// letting the blob's own task acknowledge it. In the legacy code this
// had a UI popup + event loop waiting for confirmation; move that to
// the caller if it comes back in Phase D.
bool isCrossfireRfOn()
{
  return !getCrsfFlag(CRSF_FLAG_RF_OFF);
}

void crossfireTurnOnRf()
{
  clearCrsfFlag(CRSF_FLAG_RF_OFF);
}

void crossfireTurnOffRf(bool /*ask*/)
{
  // Non-blocking: request the blob turn RF off, don't wait for
  // acknowledgement. The legacy version busy-waited here with a UI
  // popup; the modern flow should be triggered by a menu action and
  // check isCrossfireRfOn() asynchronously.
  setCrsfFlag(CRSF_FLAG_RF_OFF);
}

void crossfirePowerOff()
{
  // Request the blob power down, with a short timeout so shutdown
  // never hangs. getCrsfFlag/setCrsfFlag are simple bit ops; no
  // semaphore needed.
  if (!isCrossfireRfOn()) {
    return;
  }
  setCrsfFlag(CRSF_FLAG_POWER_OFF);
  uint32_t offTimeout = get_tmr10ms();
  while (getCrsfFlag(CRSF_FLAG_POWER_OFF)) {
    watchdogReset();
    if (get_tmr10ms() - offTimeout >= 200) {
      break;
    }
  }
}

void boardSetSkipWarning()
{
  bkregSetStatusFlag(DEVICE_RESTART_WITHOUT_WARN_FLAG);
}

// ---------------------------------------------------------------------------
// CRSF port callbacks
// ---------------------------------------------------------------------------

// Tx: EdgeTX → blob. Frames routed to CRSF_SHARED_FIFO get pushed into
// the blob's crsf_rx ring buffer.
void crsfToSharedFIFO(uint8_t* pArr)
{
  *pArr = LIBCRSF_UART_SYNC;
  uint8_t len = *(pArr + LIBCRSF_LENGTH_ADD) + LIBCRSF_HEADER_OFFSET + LIBCRSF_CRC_SIZE;
  for (uint8_t i = 0; i < len; i++) {
    crossfireSharedData.crsf_rx.push(*(pArr + i));
  }
}

// Rx: handler for frames destined to DEVICE_INTERNAL (EdgeTX itself).
// For now we buffer telemetry into intCrsfTelemetryFifo and drop everything
// else — the full device-ping/setting/command machinery is Phase D.
void crsfThisDevice(uint8_t* pArr)
{
  uint8_t len = *(pArr + LIBCRSF_LENGTH_ADD) + 2;
  for (uint8_t i = 0; i < len; i++) {
    intCrsfTelemetryFifo.push(*(pArr + i));
  }
}

// Pulled from the blob's crsf_tx buffer by crossfireTasksCreate's systemTask
// (see crsf_tasks.cpp). One byte per call, parsed incrementally by
// libCrsfParse; a complete frame triggers libCrsfRouting which fans out
// through libCrsfPorts[].
void crsfSharedFifoHandler()
{
  uint8_t byte;
  static libCrsfParseData crsfData;
  if (crossfireSharedData.crsf_tx.pop(byte)) {
    if (libCrsfParse(&crsfData, byte)) {
      libCrsfRouting(CRSF_SHARED_FIFO, crsfData.payload);
    }
  }
}

void crsfSetModelID()
{
  // Stubbed — full implementation needs LIBCRSF_CMD_FRAME / LIBCRSF_RC_RX_CMD
  // enums that the current libcrsf headers don't export. Phase D.
}

void crsfGetModelID()
{
  // Stubbed — same reason as above.
}

void updateIntCrossfireChannels()
{
  for (uint8_t i = 0; i < CROSSFIRE_CHANNELS_COUNT; ++i) {
    crossfireSharedData.channels[i] = channelOutputs[i];
  }
}
