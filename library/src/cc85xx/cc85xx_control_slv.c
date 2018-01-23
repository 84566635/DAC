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
#include <cc85xx_control_slv.h>
//////////////////////////////////////////////////////////////////////////////

// Shared parameter/data memory for most EHIF commands (to save RAM and avoid using stack)
EHIF_CMD_PARAM_T ehifCmdParam;
EHIF_CMD_DATA_T  ehifCmdData;

EHIF_CMD_DI_GET_CHIP_INFO_DATA_T    ehifNwmGetChipIdData;

// Scan data (not included in EHIF_CMD_DATA_T due to size)
EHIF_CMD_NWM_DO_SCAN_DATA_T ehifNwmDoScanData;

// Have the remote control data in a separate variable because mouse operation depends on previous values
EHIF_CMD_RC_SET_DATA_PARAM_T ehifRcSetDataParam;


static void initParam(void)
{
  memset(&ehifCmdParam, 0x00, sizeof(ehifCmdParam));
} // initParam

unsigned char receiveDatagram(void);
unsigned char receiveDatagram(void)
{
  uint16_t readbcLength;
  unsigned char data[PAYLOAD_PLUS_ADR];
  if (ehifGetStatus() &  BV_EHIF_EVT_DSC_RX_AVAIL )
    {
      initParam();
      readbcLength = PAYLOAD_PLUS_ADR;
      ehifCmdExecWithReadbc(EHIF_EXEC_ALL,  EHIF_CMD_DSC_RX_DATAGRAM,
                            0, &ehifCmdParam,
                            &readbcLength, &data);
    }

  EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T devInfo;
  memset(&devInfo, 0x00, sizeof(devInfo) );
  readbcLength = 12;

  ehifCmdExecWithRead(EHIF_EXEC_ALL, EHIF_CMD_DI_GET_DEVICE_INFO,
                      0, &ehifCmdParam,
                      readbcLength, &devInfo);

  return 0;
}


EHIF_CMD_NWM_GET_STATUS_SLAVE_DATA_T getStatus(void)
{

  uint16_t readbcLength;

  WAIT_BUSY;
  EHIF_CMD_NWM_GET_STATUS_SLAVE_DATA_T statusSlave;
  memset(&statusSlave, 0x00, sizeof(statusSlave) );
  initParam();

  *((unsigned short *) &ehifCmdParam) = 0x00B0;
  readbcLength = 30;

  ehifCmdExecWithReadbc(EHIF_EXEC_ALL, EHIF_CMD_NWM_GET_STATUS_S,
                        0, &ehifCmdParam,   // command lenth and params
                        &readbcLength, &statusSlave);

  if (readbcLength == 0 || statusSlave.nwkId == 0)
    {
      memset(&statusSlave, 0x00, sizeof(statusSlave) );
    }

  return statusSlave;
}

EHIF_CMD_VC_GET_VOLUME_PARAM_T getVolume(void)
{

  EHIF_CMD_VC_GET_VOLUME_PARAM_T   getVolumeStruct;
  uint16_t readLength;

  initParam();
  readLength = 1;

  //#warning "Set params for slave or master"
  ehifCmdExecWithRead(EHIF_EXEC_ALL, EHIF_CMD_VC_GET_VOLUME,
                      1, &ehifCmdParam,   // command lenth and params
                      readLength, &getVolumeStruct);

  return getVolumeStruct;
}

EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T getNetworkID(uint32_t masterID, uint32_t timeout_ms)
{
  EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T retInfo;
  memset(&retInfo, 0x00, sizeof(retInfo));
  //  massert(timeout_ms > 4000);
#define MAX_NETWORKS_NUM 1
  ehifWaitReadyMs(1000);

  EHIF_CMD_NWM_DO_SCAN_DATA_T ehifNwmDoScanData[MAX_NETWORKS_NUM];

  uint16_t readbcLength;
  // TODO: check device state

  // Search for one protocol master with pairing signal enabled for 10 seconds
  initParam();
  EHIF_CMD_NWM_DO_SCAN_PARAM_T params;
  memset(&params, 0x00, sizeof(params));

  ehifCmdParam.nwmDoScan.scanTo           = timeout_ms / 10; //timeout in 10ms
  ehifCmdParam.nwmDoScan.scanMax          = MAX_NETWORKS_NUM;
  ehifCmdParam.nwmDoScan.reqPairingSignal = 0;
  ehifCmdParam.nwmDoScan.reqRssi          = -128;
  ehifCmdParam.nwmDoScan.mfctId           = masterID;
  ehifCmdExecWithReadbc(EHIF_EXEC_CMD, EHIF_CMD_NWM_DO_SCAN,
                        sizeof(EHIF_CMD_NWM_DO_SCAN_PARAM_T), &ehifCmdParam,
                        NULL, NULL);

  // Fetch network information once ready
  ehifWaitReadyMs(timeout_ms + 1000);
  readbcLength = sizeof(ehifNwmDoScanData);


  memset(&ehifNwmDoScanData, 0x00, sizeof(ehifNwmDoScanData));
  ehifCmdExecWithReadbc(EHIF_EXEC_DATA, EHIF_CMD_NWM_DO_SCAN,
                        0, NULL,
                        &readbcLength, &ehifNwmDoScanData);

  if (readbcLength == 0)
    {
      // return empty result
      memset(&ehifNwmDoScanData, 0x00, sizeof(ehifNwmDoScanData));
    }

  uint16_t i = 0;
  for ( i = 0; i < MAX_NETWORKS_NUM; i++)
    if (ehifNwmDoScanData[i].mfctId == masterID)
      {
        retInfo.deviceId = ehifNwmDoScanData[i].deviceId;
        retInfo.mfctId = ehifNwmDoScanData[i].mfctId;
        retInfo.prodId = ehifNwmDoScanData[i].prodId;
        /* dprintf("%d %d %d %d %d %d %d \n", */
        /*         ehifNwmDoScanData[i].wpmAllowsJoin, */
        /*         ehifNwmDoScanData[i].wpmPairSignal, */
        /*         ehifNwmDoScanData[i].wpmMfctFilt, */
        /*         ehifNwmDoScanData[i].wpmDscEn, */
        /*         ehifNwmDoScanData[i].wpmPowerState, */
        /*         ehifNwmDoScanData[i].achSupport, */
        /*         ehifNwmDoScanData[i].rssi); */

        return  retInfo;
      }

  return retInfo;
}

EHIF_CMD_NWM_DO_SCAN_DATA_T doScan(uint32_t timeout_ms)
{
  ehifWaitReadyMs(1000);

  uint16_t readbcLength;

  // TODO: check device state

  // Search for one protocol master with pairing signal enabled for 10 seconds
  initParam();
  EHIF_CMD_NWM_DO_SCAN_PARAM_T params;
  memset(&params, 0x00, sizeof(params));

  ehifCmdParam.nwmDoScan.scanTo           = timeout_ms / 10; //timeout in 10ms
  ehifCmdParam.nwmDoScan.scanMax          = 1;
  ehifCmdParam.nwmDoScan.reqPairingSignal = 0;
  ehifCmdParam.nwmDoScan.reqRssi          = -128;
  ehifCmdExecWithReadbc(EHIF_EXEC_CMD, EHIF_CMD_NWM_DO_SCAN,
                        sizeof(EHIF_CMD_NWM_DO_SCAN_PARAM_T), &ehifCmdParam,
                        NULL, NULL);

  // Fetch network information once ready
  ehifWaitReadyMs(timeout_ms + 1000);
  readbcLength = sizeof(ehifNwmDoScanData);


  memset(&ehifNwmDoScanData, 0x00, sizeof(ehifNwmDoScanData));
  ehifCmdExecWithReadbc(EHIF_EXEC_DATA, EHIF_CMD_NWM_DO_SCAN,
                        0, NULL,
                        &readbcLength, &ehifNwmDoScanData);

  if (readbcLength != sizeof(EHIF_CMD_NWM_DO_SCAN_DATA_T))
    {
      // return empty result
      memset(&ehifNwmDoScanData, 0x00, sizeof(ehifNwmDoScanData));
    }
  return ehifNwmDoScanData;

}

void disconnectFromMaster(void)
{
  // Let the last executed EHIF command complete, with 1 second timeout
  ehifWaitReadyMs(1000);

  // Disconnect if currently connected
  int stat = ehifGetStatus();
  if (stat & BV_EHIF_STAT_CONNECTED)
    {
      initParam();
      // All parameters should be zero
      ehifCmdExec(EHIF_CMD_NWM_DO_JOIN, sizeof(EHIF_CMD_NWM_DO_JOIN_PARAM_T), &ehifCmdParam);
      ehifWaitReadyMs(5000);
    }
}

uint8_t doJoin(uint32_t masterAddress, uint32_t masterManID)
{
  uint8_t ret = 0 ;

  // Let the last executed EHIF command complete, with 1 second timeout
  ehifWaitReadyMs(1000);

  disconnectFromMaster();

  initParam();
  ehifCmdParam.nwmDoJoin.joinTo = 100;//1s
  ehifCmdParam.nwmDoJoin.deviceId = masterAddress;
  ehifCmdParam.nwmDoJoin.mfctId = masterManID;
  ehifCmdExec(EHIF_CMD_NWM_DO_JOIN, sizeof(EHIF_CMD_NWM_DO_JOIN_PARAM_T), &ehifCmdParam);
  // }

  ehifWaitReadyMs(5000);
  uint16_t status = ehifGetStatus();
  if (status & BV_EHIF_STAT_CONNECTED)
    {
      ret = 1;
    }
  else
    {
      ret = 0;
    }
  return ret;
}

uint8_t CheckNetwork(void)
{
  uint8_t ret = 0;
  // GET_STATUS();  //Pobierz aktualny status z informacjami o zmianach w sieci i datagramach
  // If (STATUS & EVT_NWK_CHG)
  // NWM_GET_STATUS_S();
  // // Jeżeli nie ma polaczenia z Masterem wykonaj ponownie NWM_DO_JOIN

  uint16_t status = ehifGetStatus();

  if ( status & BV_EHIF_STAT_CONNECTED)
    {
      /* if (status & EVT_NWK_CHG){
        // zmiana stanu sieci
      } */
      ret = 1;
    }
  else
    {
      uint32_t remoteAddress = getLastDisconnected();
      if ( doJoin(remoteAddress, 0) )
        {
          ret = 1;  // reconnection successful
        }
      else
        {
          ret = 0;  // reconnection failed
        }
    }

  return ret;
}

uint32_t getLastDisconnected(void)
{
  // Get the last used network ID from CC85XX non-volatile storage
  initParam();
  ehifCmdParam.nvsGetData.index = 0;
  ehifCmdExecWithRead(EHIF_EXEC_ALL, EHIF_CMD_NVS_GET_DATA,
                      sizeof(EHIF_CMD_NVS_GET_DATA_PARAM_T), &ehifCmdParam,
                      sizeof(EHIF_CMD_NVS_GET_DATA_DATA_T), &ehifCmdData);
  uint32_t nwkId = ehifCmdData.nvsGetData.data;

  // Handle illegal default network IDs that may occur first time after programming
  if ((nwkId == 0x00000000) || (nwkId == 0xFFFFFFFF))
    {
      nwkId = 0xFFFFFFFE;
    }
  return nwkId;
}

void saveAdrNonVolatile(void)
{

  // Place the new network ID in CC85XX non-volatile storage
  initParam();
  ehifCmdParam.nvsSetData.index = 0;
  ehifCmdParam.nvsSetData.data  = ehifNwmDoScanData.deviceId;
  ehifCmdExec(EHIF_CMD_NVS_SET_DATA, sizeof(EHIF_CMD_NVS_SET_DATA_PARAM_T), &ehifCmdParam);
}




void activateChannels(uint8_t *ch)
{
  // Connected: Subscribe to audio channels (0xFF = unused)
  memset(&ehifCmdParam, 0xFF, sizeof(EHIF_CMD_NWM_ACH_SET_USAGE_PARAM_T));
  int i = 0;
  for (i = 0; i < MAX_SLOT_NUM; i++)
    {
      ehifCmdParam.nwmAchSetUsage.pAchUsage[i] = ch[i];
    }
  ehifCmdExec(EHIF_CMD_NWM_ACH_SET_USAGE, sizeof(EHIF_CMD_NWM_ACH_SET_USAGE_PARAM_T), &ehifCmdParam);
}




