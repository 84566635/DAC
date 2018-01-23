#include <project.h>
#include <common.h>
#include STMINCP()
#include STMINCS(_gpio)
#include STMINCS(_spi)
#include STMINCS(_rcc)
#include <moduleCommon.h>
#include <led.h>
#include "FreeRTOS.h"
#include <task.h>
#include <stdio.h>
#include <string.h>

#include <cc85xx_common.h>
#include <cc85xx_control_mst.h>

//////////////////////////////////////////////////////////////////////////////

// Shared parameter/data memory for most EHIF commands (to save RAM and avoid using stack)
EHIF_CMD_PARAM_T ehifCmdParam;
EHIF_CMD_DATA_T  ehifCmdData;


static void initParam(void)
{
  memset(&ehifCmdParam, 0x00, sizeof(ehifCmdParam));
} // initParam


EHIF_CMD_NWM_GET_STATUS_MASTER_DATA_T getStatus(void)
{
  uint16_t readbcLength;

  EHIF_CMD_NWM_GET_STATUS_MASTER_DATA_T statusMaster;
  memset(&statusMaster, 0x00, sizeof(statusMaster) );

  initParam();
  readbcLength = sizeof(statusMaster);

  // Get full status
  ehifCmdExecWithReadbc(EHIF_EXEC_ALL, EHIF_CMD_NWM_GET_STATUS_M,
                        0, &ehifCmdParam,   // command length and params
                        &readbcLength, &statusMaster);  // data length and params

  // if(readbcLength == MASTER_STATUS_SIZE){
  //   //No slaves
  // }

  return statusMaster;
}

void enablePairing(void)
{
  //#warning Check prerequisites: connection, ... etc.
  initParam();
  ehifCmdParam.nwmControlSignal.wmPairSignal = 1;
  ehifCmdExec(EHIF_CMD_NWM_CONTROL_SIGNAL, sizeof(EHIF_CMD_NWM_CONTROL_SIGNAL_PARAM_T), &ehifCmdParam);
}

void disablePairing(void)
{
  //#warning Check prerequisites: connection, ... etc.
  initParam();
  ehifCmdParam.nwmControlSignal.wmPairSignal = 0;
  ehifCmdExec(EHIF_CMD_NWM_CONTROL_SIGNAL, sizeof(EHIF_CMD_NWM_CONTROL_SIGNAL_PARAM_T), &ehifCmdParam);
}


void test_master(void)
{

  reset_powerActive();

  EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T devInfo = getDeviceInfo();   // contains prodID manID devID
  (void)devInfo;

  chipRadioEnable(ENABLE);  // start audio network


  EHIF_CMD_NWM_GET_STATUS_MASTER_DATA_T status_m;

  while (1)
    {
      status_m = getStatus();   // full status :
      (void) status_m;

      vTaskDelay(1000);
    }

  // Get the network status, which is the first byte
  // uint8_t nwkStatus = 0;
  // uint16_t readbcLength = 1;
  // ehifCmdExecWithReadbc(EHIF_EXEC_ALL, EHIF_CMD_NWM_GET_STATUS_M, 0, NULL, &readbcLength, &nwkStatus);

  // Check for errors
  uint16_t status = ehifGetStatus();
  if (ehifGetWaitReadyError() || (status & BV_EHIF_EVT_SPI_ERROR) )
    {
      {
        int s = 1;
        while (s);
      }

    }
}

void chipRadioEnable(uint8_t state)
{
  initParam();
  ehifCmdParam.nwmControlEnable.wmEnable = state;
  ehifCmdExec(EHIF_CMD_NWM_CONTROL_ENABLE, sizeof(EHIF_CMD_NWM_CONTROL_ENABLE_PARAM_T), &ehifCmdParam);

}

EHIF_CMD_VC_SET_VOLUME_PARAM_T setVolume(void)
{

  EHIF_CMD_VC_SET_VOLUME_PARAM_T   setVolumeStruct = {0};

  initParam();
  //ehifCmdParam.vcSetVolume;
  //#warning "Set params for slave or master"
  ehifCmdExec(EHIF_CMD_VC_SET_VOLUME, sizeof(EHIF_CMD_VC_SET_VOLUME_PARAM_T), &ehifCmdParam);

  return setVolumeStruct;
}


void rfChannels(uint32_t channelsMask)
{
  initParam();
  ehifCmdParam.nwmSetRfChMask.rfChMask = ((channelsMask&0x3FFFF) << 1);
  ehifCmdExec(EHIF_CMD_NWM_SET_RF_CH_MASK, sizeof(EHIF_CMD_NWM_SET_RF_CH_MASK_PARAM_T), &ehifCmdParam);
}
