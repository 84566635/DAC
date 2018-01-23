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
#include <hwCommunicator.h>
#include <hwCommunicatorSPIhelper.h>
//////////////////////////////////////////////////////////////////////////////


// Shared parameter/data memory for most EHIF commands (to save RAM and avoid using stack)
EHIF_CMD_PARAM_T ehifCmdParam;
EHIF_CMD_DATA_T  ehifCmdData;

EHIF_CMD_DI_GET_CHIP_INFO_DATA_T    ehifNwmGetChipIdData;

static void initParam(void)
{
  memset(&ehifCmdParam, 0x00, sizeof(ehifCmdParam));
} // initParam



EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T getDeviceInfo(void)
{
  uint16_t readbcLength;

  EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T devInfo;
  memset(&devInfo, 0x00, sizeof(devInfo) );
  initParam();
  readbcLength = 12;

  ehifCmdExecWithRead(EHIF_EXEC_ALL, EHIF_CMD_DI_GET_DEVICE_INFO,
                      0, &ehifCmdParam,
                      readbcLength, &devInfo);

  return devInfo;
}

EHIF_CMD_DI_GET_CHIP_INFO_DATA_T getChipInfo(void)
{
  uint16_t readbcLength;

  EHIF_CMD_DI_GET_CHIP_INFO_DATA_T chipInfo;
  memset(&chipInfo, 0x00, sizeof(chipInfo) );
  initParam();

  *((unsigned short *) &ehifCmdParam) = 0x00B0;
  readbcLength = 24;

  ehifCmdExecWithRead(EHIF_EXEC_ALL, EHIF_CMD_DI_GET_DEVICE_INFO,
                      2, &ehifCmdParam,   // command lenth and params
                      readbcLength, &chipInfo);

  return chipInfo;
}


uint8_t cc85xx_Check_RX(void)
{
  //CheckNetwork();   ???

  uint16_t status = ehifGetStatus();
  uint16_t clrMask = status & (BV_EHIF_EVT_VOL_CHG | BV_EHIF_EVT_PS_CHG | BV_EHIF_EVT_NWK_CHG | BV_EHIF_EVT_SR_CHG);
  if(clrMask)
    {
      if(clrMask&BV_EHIF_EVT_VOL_CHG)dprintf(LL_INFO, "Volume has changed\n");
      if(clrMask&BV_EHIF_EVT_PS_CHG) dprintf(LL_INFO, "Power state has changed\n");
      if(clrMask&BV_EHIF_EVT_NWK_CHG)dprintf(LL_INFO, "Network state has changed\n");
      if(clrMask&BV_EHIF_EVT_SR_CHG) dprintf(LL_INFO, "Sample rate has changed\n");
      clearFlags(clrMask);
    }
  if ( status & BV_EHIF_STAT_CONNECTED && status != 0xFFFF )  //0xFFFF when no CS or no cable connection
    {
      if (status & BV_EHIF_EVT_DSC_RX_AVAIL)
        {
          return 1;
        }
      else
        {
          return 0;
        }
    }
  else
    {
      //#warning "TODO: Handle no connection case"
      return 0;
    }
}

void clearFlags( uint8_t mask )
{
  mask &= 0x3F;
  initParam();
  ehifCmdParam.ehcEvtClr.clearedEvents = mask;
  ehifCmdExec(EHIF_CMD_EHC_EVT_CLR, sizeof(EHIF_CMD_EHC_EVT_CLR_PARAM_T), &ehifCmdParam);
}

void setIntEvents( uint8_t mask )
{
  initParam();
  ehifCmdParam.ehcEvtMask.irqGioLevel = ACTIVE_LOW;
  ehifCmdParam.ehcEvtMask.eventFilter = mask;
  ehifCmdExec(EHIF_CMD_EHC_EVT_MASK, sizeof(EHIF_CMD_EHC_EVT_MASK_PARAM_T), &ehifCmdParam);
}



// returns RX data length and address
int queueRX(uint32_t *address, uint8_t *ret_data, uint8_t *reset)
{
  int ret = 0;

  uint16_t readbcLength;
  unsigned char data[PAYLOAD_PLUS_ADR];
#define RX_FRAME_HEADER_SIZE 5

  // check Data avalibility
  if (ehifGetStatus() &  BV_EHIF_EVT_DSC_RX_AVAIL )
    {
      initParam();
      readbcLength = PAYLOAD_PLUS_ADR;
      ehifCmdExecWithReadbc(EHIF_EXEC_ALL,  EHIF_CMD_DSC_RX_DATAGRAM,
                            0, &ehifCmdParam,
                            &readbcLength, &data);
      if (readbcLength)
        {
          ret = readbcLength-RX_FRAME_HEADER_SIZE;
          *address = *(uint32_t *)(data + 1);
          memcpy(ret_data, data + RX_FRAME_HEADER_SIZE, readbcLength - RX_FRAME_HEADER_SIZE);     // 5 = 4bytes of address and one additional reserved byte
        }
      else
        {
          ret = 0;
        }

      if(reset)
        {
          if (data[0] & 0x01) // check reset conection flag in received datagram
            *reset = 1;
          else
            *reset = 0;
        }
    }
  return ret;
}

int queueTX(remoteDev_t *dev, uint16_t dataLength, uint8_t *data, uint8_t reset)
{
  //#warning "Check for data-side connection availability and remote device compatibility"
  int ret = 0;

  massert(dataLength <= MAX_PAYLOAD);
  uint16_t status = ehifGetStatus();

  if (  status &  BV_EHIF_STAT_CONNECTED )
    {
      if ( status & BV_EHIF_EVT_SPI_ERROR)
        {
          // clear SPI_ERROR_FLAG:
          clearFlags( BV_EHIF_EVT_SPI_ERROR );
          int timeout = 100;
          do
            {
              status = ehifGetStatus();
            }
          while ( status & BV_EHIF_EVT_SPI_ERROR && --timeout );
          massert(timeout);
        }
      if ( status & BV_EHIF_EVT_DSC_TX_AVAIL )
        {
          initParam();
          // check wheather datagram connection needs to be reestablished
          if (reset)
            ehifCmdParam.dscTxDatagram.connReset = 1;

          ehifCmdParam.dscTxDatagram.addr = dev->devID;

          WAIT_BUSY;
          ehifCmdExecWithWrite(EHIF_EXEC_ALL, EHIF_CMD_DSC_TX_DATAGRAM,
                               sizeof(EHIF_CMD_DSC_TX_DATAGRAM_PARAM_T) , &ehifCmdParam,
                               dataLength, (void *) data);
        }
      else
        {
          return -1;//TxFull
        }

    }
  else
    {
      return -2;
    }

  return ret;
}

void powerOff(void)
{
  // HANDLE POWER OFF

  // Ensure known state (power state 5)
  ehifSysResetSpi(1); // with wait rready flag

  // Set power state 0
  initParam();
  ehifCmdParam.pmSetState.state = 0;
  ehifCmdExec(EHIF_CMD_PM_SET_STATE, sizeof(EHIF_CMD_PM_SET_STATE_PARAM_T), &ehifCmdParam);
}


int reset_powerActive(void)
{
  // HANDLE POWER ON
  // Ensure known state (power state 5)
  ehifSysResetSpi(1); // with wait rready flag
  ehifWaitReadyMs(300);
  return ehifGetWaitReadyError();
}
