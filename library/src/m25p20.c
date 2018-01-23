#include <project.h>
#include <common.h>
#include <string.h>
#include <cfg.h>
#include STMINCP()
#include STMINCS(_flash)
#include "FreeRTOS.h"
#include "task.h"
#include <m25p20.h>
#include <bqueue.h>
#include <messageProcessorSMOK.h>


#define WAIT_FOR(arg_timeout, arg_cmp)			\
  {							\
    int timeout = arg_timeout;				\
    do							\
      {							\
	tx->cmd = M25P20_CMD_READ_STATUS;				\
	sharcTransfer((uint8_t*)tx, (uint8_t*)rx, M25P20_XFER_SIZE(rx, rwStatus)); \
	if(arg_cmp)							\
	  {								\
	    timeout--;							\
	    mdelay(1);							\
	  }								\
	else								\
	  timeout=0;							\
      }while(timeout);							\
  }

#define WAIT_FOR_NOWIP(arg_timeout) WAIT_FOR(arg_timeout, (rx->rwStatus.data & (1<<M25P20_STATUS_WIP)))
#define WAIT_FOR_WEL(arg_timeout)   WAIT_FOR(arg_timeout, !(rx->rwStatus.data & (1<<M25P20_STATUS_WEL)))

int m25p20Program(const uint8_t *src, int size)
{
  dprintf(LL_INFO, "Sharc image programming....\n");

  m25p20_cmd_t *tx = dAlloc(sizeof(m25p20_cmd_t));
  m25p20_cmd_t *rx = dAlloc(sizeof(m25p20_cmd_t));

  //check ID
  tx->cmd = M25P20_CMD_READ_ID;
  sharcTransfer((uint8_t*)tx, (uint8_t*)rx, M25P20_XFER_SIZE(rx, rId));
  if(rx->rId.manufacturerId != 0x20 || rx->rId.memoryType != 0x20 || rx->rId.memoryCapacity < 0x12)
    {
      dprintf(LL_ERROR, "Unrecognized flash chip for sharc (manId=%02x memType=%02x memCap=%02x\n",
              rx->rId.manufacturerId, rx->rId.memoryType, rx->rId.memoryCapacity);
      dFree(tx);
      dFree(rx);
      return 1;
    }
  dprintf(LL_INFO, "Found flash chip for sharc (manId=%02x memType=%02x memCap=%02x\n",
	  rx->rId.manufacturerId, rx->rId.memoryType, rx->rId.memoryCapacity);

  //Unprotect the chip
  tx->cmd = M25P20_CMD_WRITE_ENABLE;
  sharcTransfer((uint8_t*)tx, (uint8_t*)rx, 1);
  WAIT_FOR_WEL(100);
  tx->cmd = M25P20_CMD_WRITE_STATUS;
  tx->rwStatus.data = 0;
  sharcTransfer((uint8_t*)tx, (uint8_t*)rx, M25P20_XFER_SIZE(rx, rwStatus));
  WAIT_FOR_NOWIP(100);

  //write Enable & bulk erase
  tx->cmd = M25P20_CMD_WRITE_ENABLE;
  sharcTransfer((uint8_t*)tx, (uint8_t*)rx, 1);
  WAIT_FOR_WEL(100);
  tx->cmd = M25P20_CMD_BULK_ERASE;
  sharcTransfer((uint8_t*)tx, (uint8_t*)rx, 1);
  WAIT_FOR_NOWIP(5000);

  //Program chip
  int offset = 0;
  for(offset =0; offset < size; offset += 256)
    {
      //write Enable & page program
      tx->cmd = M25P20_CMD_WRITE_ENABLE;
      sharcTransfer((uint8_t*)tx, (uint8_t*)rx, 1);
      WAIT_FOR_WEL(100);
      tx->cmd = M25P20_CMD_PAGE_PROGRAM;
      memcpy(tx->programPage.data, &src[offset], 256);
      M25P20_XFER_ADDRESS(tx->programPage.address, offset);
      sharcTransfer((uint8_t*)tx, (uint8_t*)rx, M25P20_XFER_SIZE(rx, programPage));
      WAIT_FOR_NOWIP(5000);
    }

  //Verify chip
  for(offset =0; offset < size; offset += 256)
    {
      tx->cmd = M25P20_CMD_READ_DATA;
      M25P20_XFER_ADDRESS(tx->programPage.address, offset);
      sharcTransfer((uint8_t*)tx, (uint8_t*)rx, M25P20_XFER_SIZE(rx, rData));
      WAIT_FOR_NOWIP(5000);
      if(memcmp(rx->rData.data, &src[offset], 256))
        {
          dprintf(LL_ERROR, "Sharc image verification failed at 0x%06x\n", offset);
          dFree(tx);
          dFree(rx);
          return 1;
        }

    }
  dprintf(LL_INFO, "Sharc image programming success\n");

  dFree(tx);
  dFree(rx);
  return 0;
}
