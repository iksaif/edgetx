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
#include "io/crsf/crc8.h"

// Declared in libcrsf's crsf_utilities.cpp — writes one byte into a frame
// buffer and advances the count. Needed to build command frames below.
extern void libUtilWrite8(uint8_t* pArr, uint32_t* pCount, uint8_t value);

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
  // Matches the legacy crossfire.cpp::crsfGetHWID() semantics: return the
  // radio-side HWID that crsfInit() above read from BKREG_HW_ID_RADIO and
  // cached into libCrsfMyHwID (before wiping the backup slot). The
  // TANGO pcbrev low nibble comes out of this value; board.cpp:
  //   hardwareOptions.pcbrev = crsfGetHWID() & 0x0F;
  // Reading BKREG_HW_ID_XF here would be wrong on two counts:
  //   1. BKREG_HW_ID_XF is the *co-processor* HWID, not the radio's.
  //   2. The blob hasn't started yet when boardInit() first calls this
  //      — the slot is 0 on a cold boot, which selected the V2 battery
  //      scale on every first power-up.
  return libCrsfMyHwID;
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
// Recognised command frames update internal state; everything else is
// buffered into intCrsfTelemetryFifo so the telemetry subsystem can
// drain it at its own pace.
void crsfThisDevice(uint8_t* pArr)
{
#ifdef LIBCRSF_ENABLE_COMMAND
  // Command frame (0x32) from the RF module. Besides the outer POLYNOM_1
  // CRC that libCrsfParse already checked, CMD frames carry an inner
  // POLYNOM_2 CRC that TBS uses to guard the subcommand payload — verify
  // it before trusting any bytes, matching the legacy crossfire.cpp behaviour.
  if (*(pArr + LIBCRSF_TYPE_ADD) == LIBCRSF_CMD_FRAME) {
    uint8_t plen = *(pArr + LIBCRSF_LENGTH_ADD);
    uint8_t innerCrc =
        libCRC8GetCRCArr(pArr + LIBCRSF_TYPE_ADD, plen - 2, POLYNOM_2);
    if (*(pArr + plen + LIBCRSF_HEADER_OFFSET - 1) != innerCrc) {
      return; // corrupted CMD frame — drop silently, don't push to telemetry
    }
    // RC_RX_CMD.REPLY_CURRENT_MODEL: third payload byte is the model
    // number the receiver believes is active. Tracked to know whether
    // the Set-Model-ID handshake has converged.
    if (*(pArr + LIBCRSF_EXT_PAYLOAD_START_ADD)     == LIBCRSF_RC_RX_CMD &&
        *(pArr + LIBCRSF_EXT_PAYLOAD_START_ADD + 1) == LIBCRSF_RC_RX_REPLY_CURRENT_MODEL_SUBCMD) {
      currentCrsfModelId = *(pArr + LIBCRSF_EXT_PAYLOAD_START_ADD + 2);
      return;
    }
  }
#endif

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

#ifdef LIBCRSF_ENABLE_COMMAND
// Build a 0x32 command frame targeting the RF-module receiver path, with
// the given RC_RX subsubcommand and an optional payload byte. Sends the
// finished frame through the shared FIFO to the blob. Common between
// Set-Model-ID (push our active model number) and Get-Model-ID (poll the
// receiver's notion of active model number).
static void tbs_crsf_send_rc_rx_cmd(uint8_t subsubcmd, uint8_t payload)
{
  uint8_t txBuf[LIBCRSF_MAX_BUFFER_SIZE];
  uint32_t count = 0;

  libUtilWrite8(txBuf, &count, LIBCRSF_UART_SYNC);       /* sync */
  libUtilWrite8(txBuf, &count, 0);                       /* frame length (filled in below) */
  libUtilWrite8(txBuf, &count, LIBCRSF_CMD_FRAME);       /* type = 0x32 */
  libUtilWrite8(txBuf, &count, LIBCRSF_RC_TX);           /* destination = RC TX module */
  libUtilWrite8(txBuf, &count, LIBCRSF_REMOTE_ADD);      /* origin = us */
  libUtilWrite8(txBuf, &count, LIBCRSF_RC_RX_CMD);       /* cmd subgroup = RC RX */
  libUtilWrite8(txBuf, &count, subsubcmd);               /* sub-sub action */
  libUtilWrite8(txBuf, &count, payload);                 /* payload byte */

  /* Two CRC8s, matching the frame layout TBS expects: inner CRC covers
   * [type..payload] with POLYNOM_2; outer covers same range with POLYNOM_1. */
  uint8_t crc2 = libCRC8GetCRCArr(&txBuf[2], count - 2, POLYNOM_2);
  libUtilWrite8(txBuf, &count, crc2);
  uint8_t crc1 = libCRC8GetCRCArr(&txBuf[2], count - 2, POLYNOM_1);
  libUtilWrite8(txBuf, &count, crc1);

  txBuf[LIBCRSF_LENGTH_ADD] = count - 2;
  crsfToSharedFIFO(txBuf);
}

void crsfSetModelID()
{
  tbs_crsf_send_rc_rx_cmd(LIBCRSF_RC_RX_MODEL_SELECTION_SUBCMD,
                          g_model.header.modelId[INTERNAL_MODULE]);
}

void crsfGetModelID()
{
  /* Payload byte is a don't-care for the "query" subcommand — the RF
   * module replies with REPLY_CURRENT_MODEL_SUBCMD which crsfThisDevice()
   * turns into a write to currentCrsfModelId. */
  tbs_crsf_send_rc_rx_cmd(LIBCRSF_RC_RX_CURRENT_MODEL_SELECTION_SUBCMD, 0);
}
#else
void crsfSetModelID() {}
void crsfGetModelID() {}
#endif

void updateIntCrossfireChannels()
{
  for (uint8_t i = 0; i < CROSSFIRE_CHANNELS_COUNT; ++i) {
    crossfireSharedData.channels[i] = channelOutputs[i];
  }
}
