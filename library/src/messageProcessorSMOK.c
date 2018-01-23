#include <project.h>
#include <common.h>
#include <string.h>

#include STMINCP()
#include STMINCS(_flash)
#include STMINCS(_iwdg)
#include STMINCS(_gpio)
#include "FreeRTOS.h"
#include "task.h"
#include <bqueue.h>
#include <hwCommunicator.h>
#include <hwCommunicatorUARThelper.h>
#include <hwCommunicatorSPIhelper.h>
#include <communicator.h>
#include <led.h>
#include <adc.h>
#include <messageProcessor.h>
#include <messageProcessorSMOK.h>
#include <cc85xx_common.h>
#include <cc85xx_control_slv.h>
#include <uit_monitor.h>
#include <cfg.h>
#include <watchdog.h>
#include <crc.h>
#include <m25p20.h>


typedef struct
{
  uint8_t volume;
  uint8_t cSharc;
  uint8_t cPWM;
} spkSet_t;
static spkSet_t spkSet;

static void sendKACC(void);

#define MM_LEFT  0
#define MM_RIGHT 1
#define MM_PIPE0 0
#define MM_PIPE1 1
#define MM_ONEO  0
#define MM_TWOO  1
const char masterMap[2][2][2] =
{
  [MM_LEFT][MM_PIPE0][MM_ONEO] = 0,
  [MM_LEFT][MM_PIPE0][MM_TWOO] = 0,
  [MM_LEFT][MM_PIPE1][MM_ONEO] = 1,
  [MM_LEFT][MM_PIPE1][MM_TWOO] = 1,
  [MM_RIGHT][MM_PIPE0][MM_ONEO] = 0,
  [MM_RIGHT][MM_PIPE0][MM_TWOO] = 1,
  [MM_RIGHT][MM_PIPE1][MM_ONEO] = 1,
  [MM_RIGHT][MM_PIPE1][MM_TWOO] = 0,
};

static int signalType = -1;
static int outputType = -1;
int myPipeNum = 0;
int amIStereo = 0;
#define isHD (signalType >= 0xA)
static int msgACK(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  if(DIRECTION(portNum))
    {
      if(!isOn())
        {
          //If not ON state then respond on the message for PWM and SHARC
          *forwardMask = 0;
          return CB_RET_ACK;
        }
      return 0;
    }
  return CB_RET_ACK;
}

static int msgnACK(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  if(DIRECTION(portNum)) return 0;
  return CB_RET_nACK;
}

static int msgACKamp58(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_58_t *data = (void*)bBuffer->data;
  if(DIRECTION(portNum))
    {
      if(!isOn())
        {
          //If not ON state then respond on the message for PWM and SHARC
          *forwardMask = 0;
          return CB_RET_ACK;
        }
      if(data->guiTp != 0)
        ampEnable(0);
      return 0;
    }
  if(isOn() && data->guiTp != 0)
    {
      mdelay(cfg.ampEnableDelay58);
      ampEnable(1);
    }
  return CB_RET_ACK;
}

static int msgACKamp5A(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  if(DIRECTION(portNum))
    {
      if(!isOn())
        {
          //If not ON state then respond on the message for PWM and SHARC
          *forwardMask = 0;
          return CB_RET_ACK;
        }
      ampEnable(0);
      return 0;
    }
  if(isOn())
    {
      mdelay(cfg.ampEnableDelay58);
      ampEnable(1);
    }
  return CB_RET_ACK;
}

static int msgnACKamp(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  if(DIRECTION(portNum)) return 0;
  if(isOn())
    {
      mdelay(cfg.ampEnableDelay58);
      ampEnable(1);
    }
  return CB_RET_nACK;
}

static int msg585Atimeout(portNum_t portNum, comm_t *comm)
{
  if(isOn())
    {
      mdelay(cfg.ampEnableDelay58);
      ampEnable(1);
    }
  return CB_RET_nACK;
}


static int msgOn(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_50_t *data = (void*)bBuffer->data;
  if(DIRECTION(portNum))
    {
      //Already ON?
      if(isOn())
        return CB_RET_ACK;

      signalType = -1;
      outputType = -1;
      dprintf(LL_INFO, "Wlaczam zasilanie kond %x shark %x PWM %x\n", data->kondPom, data->cSharc, data->cPWM);
      spkSet.cSharc = data->cSharc;
      spkSet.cPWM = data->cPWM;

      powerON1();
      powerON2();
      if(!Check_power2())
        return CB_RET_nACK;

      //Wait before power on
      mdelay(100);
      powerON3();

      mdelay(200);
      if(!Check_power1())
        {
          poweroffSeq();
          return CB_RET_nACK;
        }

      mdelay(cfg.powerOnDelay);

      if(pll_set_retry(0x01, 1, cfg.pllRetryNum) <= 0 && !(cfg.disable_alerts&0x1000))
        {
          poweroffSeq();
          return CB_RET_nACK;
        }


      /* if(!Check_power3()) */
      /* 	return CB_RET_nACK; */

      kaCCEnable(1);
      sendKACC();

      //Przekaz komende dalej i czekaj na potwierdzenia
      return 0;
    }
  //  LEDset(0, ~0, 0);
  mdelay(cfg.ampEnableDelay50);
  powerON4();
  sendKACC();
  mdelay(300);
  if(!Check_power4())
    {
      kaCCEnable(0);
      poweroffSeq();
      return CB_RET_nACK;
    }

  if(SmokChangeState(Smok_ON, "0x50 ACK")<0)
    {
      kaCCEnable(0);
      return CB_RET_nACK;
    }

  SmokLEDsState(1, -1, SET_NONE, isHD);
  return CB_RET_ACK;
}

static int msgNegOn(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  //  msg_51_t *data = (void *)bBuffer->data;
  if(cfg.ack50)
    {
      return CB_RET_nACK;
    }
  //Send ACK instead of nACK
  return CB_RET_ACK;
}

void sendPWMoffMsg(void)
{

  bBuffer_t *buf = bAlloc(MSG_LEN);
  massert(buf);
  buf->size = sizeof(msg_56_t);
  msg_56_t *msg = (void*)buf->data;
  memset(msg, 0, sizeof(msg_56_t));
  msg->kod = 0x56;

  messageTx(PER2_PORT, (void*)buf->data, buf->size, (uint32_t)buf);

}


static int onFailed(portNum_t portNum, comm_t *comm)
{
  //Power off when Shark or PWM did not respond or responded with nACK
  if(cfg.ack50)
    {
      kaCCEnable(0);
      poweroffSeq();
    }
  else
    {
      powerON4();
      sendKACC();
      mdelay(300);
      Check_power4();
      SmokChangeState(Smok_ON, "0x50 timeout - ack50=0");
      SmokLEDsState(1, -1, SET_NONE, isHD);
      return CB_RET_ACK;//Send ACK emulating proper poweron
    }
  return CB_RET_nACK;
}

static int msgOff(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  if(DIRECTION(portNum))
    {
      //Already OFF?
      if(!isOn())
        return CB_RET_ACK;

      int ret = SmokChangeState(Smok_OFF, "52 received");
      SmokLEDsState(0, -1, SET_NONE, -1);
      /* LEDset(0, ~0, 0); */
      /* LEDset(0, LED_GREEN, 1); */

      if(ret >= 0)
        {
          dprintf(LL_INFO, "Wylaczam zasilanie\n");
          return CB_RET_ACK;
        }
      return CB_RET_nACK;
    }

  return CB_RET_nACK;
}

static int msgKeepAlive(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  if(!DIRECTION(portNum)) return 0;
  msg_55_t *data = (void*)bBuffer->data;
  portData[portNum].tsLastSeen = xTaskGetTickCount();
  portData[portNum].keepAlive = xTaskGetTickCount();
  portData[portNum].ka++;
  if(data->flags55&FLAGS55_CONNECTION_ACTIVE)
    SPIevent(portNum, SPI_EV_TYPE_LINK_ESTABLISHED);
  else
    SPIevent(portNum, SPI_EV_TYPE_DLINK_ERROR);

  //Get peer uptime
  uint32_t upTime;
  uint8_t *upTime8 = (uint8_t*) &upTime;
  upTime8[3] = data->uptime3;
  upTime8[2] = data->uptime2;
  upTime8[1] = data->uptime1;
  upTime8[0] = data->uptime0;
  portData[portNum].uptime = upTime;

  upTime = xTaskGetTickCount();
  data->uptime3 = upTime8[3];
  data->uptime2 = upTime8[2];
  data->uptime1 = upTime8[1];
  data->uptime0 = upTime8[0];
  if(isOn())
    {
      if((data->flags55&FLAGS55_SMOK_ON) == 0)
        {
          //Off request
          SmokChangeState(Smok_OFF, "0x55 reqOn=0 - MADO ordered all SMOK's off");
          SmokLEDsState(0, -1, SET_NONE, -1);
          data->flags55 &= ~FLAGS55_SMOK_ON;
        }
      else
        data->flags55 |= FLAGS55_SMOK_ON;
    }
  else
    {
      data->flags55 &= ~FLAGS55_SMOK_ON;
      if(cfg.flags & FLAGS_AUTO_ON_MASK)
        {
          if((data->flags55&FLAGS55_SMOK_ON) != 0)
            {
              //On request
              //TBD poweon sequence
              data->flags55 |= FLAGS55_SMOK_ON;
            }
          else
            data->flags55 &= ~FLAGS55_SMOK_ON;
        }
    }

  if(cfg.flags&FLAGS_PRIMARY_STREAM_MASK)
    data->flags55 |= FLAGS55_DEFAULT_PIPE;
  else
    data->flags55 &= ~FLAGS55_DEFAULT_PIPE;

  data->cSharc     = spkSet.cSharc;
  data->cPWM       = spkSet.cPWM;
  data->signalType = signalType;
  data->outputType = outputType;
  data->volume     = spkSet.volume;


  //  if(data->pipeNum != 0xFF)
  {
    if(portData[portNum].portType == PORT_TYPE_SPI)
      {
        int pipeNum = data->pipeNum?1:0;
        int twoOutputs =   ((data->flags55&FLAGS55_IS_TWO_OUTPUS) != 0);
        int channel = (cfg.flags & FLAGS_RIGHT_CHANNEL_MASK)?1:0;
        spiHelper_t *helper = (spiHelper_t *)portData[portNum].SPIhelper;
        int remoteNum = (int)masterMap[channel][pipeNum][twoOutputs];

        if(helper->currentRemote != remoteNum || amIStereo != ((data->flags55&FLAGS55_AM_I_STEREO) != 0))
          {
            helper->currentRemote = remoteNum;
            ehifStart(&(helper->radioModule->spi));
            slaveDisconnect();
            myPipeNum = pipeNum;
            amIStereo = ((data->flags55&FLAGS55_AM_I_STEREO) != 0);
            ehifStop();
          }
      }
  }

  return CB_RET_ACK;
}

int sendStatus(int MsgCode, int val1, int val2)
{
  if(cfg.do_not_send_status == 1)
    return 0;
  if(cfg.do_not_send_status != 0 && (cfg.do_not_send_status & 0xF0) != (MsgCode & 0xF0))
    return 0;
  bBuffer_t *bBuffer = bAlloc(MSG_LEN);
  bBuffer->size = sizeof(msg_B0_t);
  msg_B0_t *data = (void*)bBuffer->data;
  data->kod = 0xB0;
  data->kondPom = portData[CEN_PORT].kond_pom;
  data->urzadzenie = portData[CEN_PORT].urzadzenie;
  data->Smok_msgType = MsgCode;
  data->Smok_msg1 = val1;
  data->Smok_msg2 = val2;
  messageTx(CEN_PORT, (void*)bBuffer->data, bBuffer->size, (uint32_t)bBuffer);
  return 0;
}

int sendStatusLong(int MsgCode, int val1, int val2, int val3, int val4)
{
  if(cfg.do_not_send_status == 1)
    return 0;
  if(cfg.do_not_send_status != 0 && (cfg.do_not_send_status & 0xF0) != (MsgCode & 0xF0))
    return 0;
  bBuffer_t *bBuffer = bAlloc(MSG_LEN);
  bBuffer->size = sizeof(msg_B1_t);
  msg_B1_t *data = (void*)bBuffer->data;
  data->kod = 0xB1;
  data->kondPom = portData[CEN_PORT].kond_pom;
  data->urzadzenie = portData[CEN_PORT].urzadzenie;
  data->Smok_msgType = MsgCode;
  data->Smok_msgA1 = val1;
  data->Smok_msgA2 = val2;
  data->Smok_msgB1 = val3;
  data->Smok_msgB2 = val4;
  messageTx(CEN_PORT, (void*)bBuffer->data, bBuffer->size, (uint32_t)bBuffer);
  return 0;
}

static int kaOk(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  if(!DIRECTION(portNum))
    {
      uitOk(portNum);
      portData[portNum].keepAlive = xTaskGetTickCount();
    }
  return 0;
}

static int msgReconnect(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_92_t *data = (void*)bBuffer->data;
  if(!DIRECTION(portNum))
    {
      if(signalType != data->signalType || outputType != data->outputType)
        mdelay(cfg.ampEnableDelay92);
      ampEnable(1);
      //Remenber signal and output types
      signalType = data->signalType;
      outputType = data->outputType;
      if(isOn())
        SmokLEDsState(1, -1, SET_OTHER, isHD);

      return CB_RET_ACK;
    }

  if(signalType != data->signalType || outputType != data->outputType)
    ampEnable(0);

  if(isOn())
    {
      //set PLL (only when ON)
      pll_set_retry(data->signalType, 0, 2);
    }
  else
    {
      //If not ON state then respond on the message for PWM and SHARC
      *forwardMask = 0;
      return CB_RET_ACK;
    }
  return 0;
}

static int msgProgram(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_9C_t *data = (void*)bBuffer->data;
  if(DIRECTION(portNum))
    {
      cfg.manId = (((uint32_t)data->man_id3) << 24) | (((uint32_t)data->man_id2) << 16) | (((uint32_t)data->man_id1) << 8);
      cfg.kondPom = data->kondPom;
      cfg.flags = (((data->flags&MSG9C_FLAGS_PIPE_MASK)?1:0)<<2) | (((data->flags&MSG9C_FLAGS_LEFT_MASK)?1:0)<<0) | (1<<1);
      //Program master
      cfgFlush();
      watchdogReset();//Delayed reset
      return CB_RET_ACK;
    }
  return 0;
}

static int msgFlash(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_A2_t *data = (void*)bBuffer->data;
  if(DIRECTION(portNum))
    {
      A2Data_t *buf = (A2Data_t *)data;
      bBuffer->size = sizeof(msg_A2r_t);
      *forwardMask = 0;
      switch(buf->subCode)
        {
        case FILE_INIT:
        {
          dprintf(LL_INFO, "Inicjalizacja flashowania \n");
          lockNonFlashMessages(1);

          //Erase FLASH
          FLASH_ClearFlag(FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );
          FLASH_Unlock();
          IWDG_ReloadCounter();
          FLASH_EraseSector(FLASH_Sector_8, VoltageRange_3);
          IWDG_ReloadCounter();
          FLASH_EraseSector(FLASH_Sector_9, VoltageRange_3);
          IWDG_ReloadCounter();
          FLASH_EraseSector(FLASH_Sector_10, VoltageRange_3);
          IWDG_ReloadCounter();
          FLASH_EraseSector(FLASH_Sector_11, VoltageRange_3);
          IWDG_ReloadCounter();
          //Send ACK od nACK
          return CB_RET_ACK;
        }
        case FILE_BUFFER:
        {
          int crc = buf->cmd_buffer.crc;
          buf->cmd_buffer.crc = 0;
          if(crc != crc16(0, buf, sizeof(A2Data_t)))
            return CB_RET_nACK;

          //Store in flash
          int offset = buf->cmd_buffer.chunkIdx*sizeof(buf->cmd_buffer.data);
          uint32_t *src = (uint32_t*)buf->cmd_buffer.data;
          uint32_t addr = (uint32_t)(&flashStorage[offset]);
          int num =  sizeof(buf->cmd_buffer.data)/4;
          while(num--)
            {
              if(FLASH_ProgramWord(addr, *(src++)) != FLASH_COMPLETE)
                return CB_RET_nACK;
              addr+=4;
            }

          //Send ACK
          return CB_RET_ACK;
        }
        case FILE_STM:
        {
          dprintf(LL_INFO, "Flashowanie STMa \n");
          //Calculate checksum and prepare for update
          uint32_t crc = crc32(0, flashStorage, buf->cmd_finish.size);
          if(crc != buf->cmd_finish.crc32)
            return CB_RET_nACK;

          //Update by bootloader
          cfg.fSize = buf->cmd_finish.size;
          cfg.fCRC = buf->cmd_finish.crc32;
          cfgFlush();
          watchdogReset();//Delayed reset

          //Send ACK od nACK
          return CB_RET_ACK;
        }
        case FILE_SHARC:
        {
          dprintf(LL_INFO, "Flashowanie SHARCa\n");

          //Calculate checksum and update SHARC
          uint32_t crc = crc32(0, flashStorage, buf->cmd_finish.size);
          if(crc != buf->cmd_finish.crc32)
            return CB_RET_nACK;

          //SPK SW
          GPIO_WriteBit(GPIOA, GPIO_Pin_9, 0);
          //RST_AMP
          GPIO_WriteBit(GPIOA, GPIO_Pin_11, 0);

          //wait after setting shark
          mdelay(10);

          //Initialize SPI for sharc
          sharc_spi_init();
          mdelay(10);

          int programRetry = cfg.sharcProgramRetry;
          while(programRetry--)
            {
              if(m25p20Program(flashStorage, buf->cmd_finish.size) == 0)
                {
                  watchdogReset();//Delayed reset
                  //Send ACK
                  return CB_RET_ACK;
                }
#ifdef WATCHDOG_COMM
              wdogFeed(WATCHDOG_COMM(0));
#endif
            }
          dprintf(LL_INFO, "Flashowanie SHARCa zakonczone\n");

          watchdogReset();//Delayed reset
          //Send nACK
          return CB_RET_nACK;
        }
        default:
          return CB_RET_nACK;
        }
    }
  return 0;
}


static int msgVolume(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_80_t *data = (void *)bBuffer->data;
  if(DIRECTION(portNum))
    {
      spkSet.volume = data->volume;
      if(!isOn())
        {
          //If not ON state then respond on the message for PWM and SHARC
          *forwardMask = 0;
          return CB_RET_ACK;
        }
      return 0;
    }
  return CB_RET_ACK;
}

comm_t MSG_TYPES_TAB[MSG_TYPES_QTY] =
{
  COMM_TBL_SET(50, 51, CONF_TIMEOUT,     3,  C_TIMEOUT_50, msgOn,        onFailed),
  COMM_TBL_SET(51, 50, NEG_CONFIRMATION, 0,    0, msgNegOn,     NULL),
  COMM_TBL_SET(52, 53, MA_SMOK,          0,    0, msgOff,       NULL),
  COMM_TBL_SET(55, 55, MA_SMOK,          0,    0, msgKeepAlive, NULL),
  COMM_TBL_SET(56, 00, SMOK_GEN,         0,    0, NULL,         NULL),
  COMM_TBL_SET(58, 59, CONF_TIMEOUT,     1,  200, msgACKamp58,  msg585Atimeout),
  COMM_TBL_SET(59, 58, NEG_CONFIRMATION, 0,    0, msgnACKamp,   NULL),
  COMM_TBL_SET(5A, 5B, CONF_TIMEOUT,     2,  200, msgACKamp5A,  msg585Atimeout),
  COMM_TBL_SET(5B, 5A, NEG_CONFIRMATION, 0,    0, msgnACKamp,   NULL),
  COMM_TBL_SET(60, 00, PER_GEN,          0,    0, msgACK,       NULL),
  COMM_TBL_SET(70, 00, PER_GEN,          0,    0, msgACK,       NULL),
  COMM_TBL_SET(80, 81, CONF_TIMEOUT,     3,  200, msgVolume,    NULL),
  COMM_TBL_SET(81, 80, NEG_CONFIRMATION, 0,    0, msgnACK,      NULL),
  COMM_TBL_SET(92, 93, CONF_TIMEOUT,     3, 1000, msgReconnect, NULL),
  COMM_TBL_SET(93, 92, NEG_CONFIRMATION, 0,    0, msgnACK,      NULL),
  COMM_TBL_SET(9C, 9D, MA_SMOK,          0,    0, msgProgram,   NULL),
  COMM_TBL_SET(C1, 00, SMOK_GEN,         0,    0, kaOk,         NULL),
  COMM_TBL_SET(C2, 00, SMOK_GEN,         0,    0, kaOk,         NULL),
  COMM_TBL_SET(CC, 00, SMOK_GEN,         0,    0, kaOk,         NULL),
  COMM_TBL_SET(A2, A3, MA_SMOK,          0,    0, msgFlash,     NULL),
  COMM_TBL_SET(B0, 00, SMOK_GEN,         0,    0, msgACK,       NULL),
  COMM_TBL_SET(B1, 00, SMOK_GEN,         0,    0, msgACK,       NULL),
};


void Timeout_Data(portNum_t portNum, char *buffer, int bufferSize)
{
}


static uint32_t timestamp = 0xFFFFFFFF;
void kaCCEnable(int enable)
{
  if(enable)
    {
      timestamp = xTaskGetTickCount() ;
      portData[PER1_PORT].keepAlive = xTaskGetTickCount();
      portData[PER2_PORT].keepAlive = xTaskGetTickCount();
    }
  else
    {
      timestamp = 0xFFFFFFFF;
    }
}

static void sendKACC(void)
{
  //Keep alive to PWM and Sharc
  bBuffer_t *buf = bAlloc(MSG_LEN);
  massert(buf);
  buf->size = sizeof(msg_CC_t);
  msg_CC_t *msg = (void*)buf->data;
  memset(msg, 0, sizeof(msg_CC_t));
  msg->kod = 0xC0 + portData[CEN_PORT].urzadzenie;

  bRef(buf);
  messageTx(PER1_PORT, (void*)buf->data, buf->size, (uint32_t)buf);
  messageTx(PER2_PORT, (void*)buf->data, buf->size, (uint32_t)buf);
}

void communicatorPeriodicPrivate(void)
{
  if(timestamp < xTaskGetTickCount())
    {
      sendKACC();
      timestamp = xTaskGetTickCount() + cfg.ka_period;
    }

  portNum_t portNum;
  for(portNum = PER1_PORT; portNum <= PER2_PORT; portNum++)
    if((int)xTaskGetTickCount() - (int)portData[portNum].keepAlive > cfg.ka_timeout)
      {
        uitTimeout(portNum);
        portData[portNum].keepAlive = xTaskGetTickCount() + cfg.ka_period;
      }

  if(abs(portData[CEN_PORT].keepAlive - xTaskGetTickCount()) > cfg.ka_timeout)
    {
      portData[CEN_PORT].keepAlive = xTaskGetTickCount();
#if 0
      if(portData[CEN_PORT].portType == PORT_TYPE_SPI)
        {
          spiHelper_t *helper = (spiHelper_t *)portData[CEN_PORT].SPIhelper;
          if(helper->radioModule && helper->radioModule->spi.spi)
            {
              ehifStart(&(helper->radioModule->spi));
              slaveDisconnect();
              ehifStop();
            }
        }
#else
      SPIevent(CEN_PORT, SPI_EV_TYPE_DLINK_ERROR);
#endif
    }
}

static netState_e prevState = NET_NC;
void connectionState(netState_e state)
{
  if(state == prevState) return;//No change
  signalType = -1;
  outputType = -1;
  switch(state)
    {
    case NET_DATAGRAM:
      SmokChangeState(Smok_OFF, "acquired datagram connection");
      SmokLEDsState(-1, 1, SET_NONE, -1);
      break;
    case NET_PARTIAL:
      if(isOn())
        SmokLEDsState(0, 0, SET_CONN, -1);
      SmokChangeState(Smok_NC, "lost datagram connection");
      power_SHD_ERR_P(DATAGRAM_CONN_LOST);
      break;
    case NET_NC:
      if(isOn())
        SmokLEDsState(0, 0, SET_CONN, -1);
      SmokChangeState(Smok_NC, "lost radio connection");
      power_SHD_ERR_P(CONN_LOST);
      break;
    }
  prevState = state;
}
