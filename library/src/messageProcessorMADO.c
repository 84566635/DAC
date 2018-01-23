#include <project.h>
#include <common.h>
#include <string.h>

#include STMINCP()
#include STMINCS(_flash)
#include STMINCS(_iwdg)
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
#include <cc85xx_common.h>
#include <convert.h>
#include <messageProcessorMADO.h>
#include <wm8805.h>
#include <cfg.h>
#include <watchdog.h>
#include <crc.h>
#include <debug.h>


struct tpGui_s;
typedef struct tpGui_s tpGui_t;

struct tpGui_s
{
  tpGui_t *next;
  int      id;
  uint8_t kond_pom;
};

static tpGui_t *tp_gui = NULL;

typedef struct
{
  uint8_t volume[2];
  uint8_t cSharc;
  uint8_t cPWM;
  uint8_t reqOn:1;
  uint8_t isOn:1;
  uint8_t signalType;
  uint8_t outputType;
} spkSet_t;
static spkSet_t spkSet[4];
static spkSet_t spkSetSMOKs[4][2];

void showSpkSets(int *bufSize, char *bufPtr)
{
  int i;
  for(i = 0; i < 4; i++)
    {
      DPRINTF("%d) isOn:%d/%d/%d reqOn:%d cSharc:%x/%x/%x cPWM:%x/%x/%x signalType %x/%x outputType %x/%x volume %d/%d:%d/%d"CLEAR_LINE"\n", i,
              spkSet[i].isOn, spkSetSMOKs[i][0].isOn, spkSetSMOKs[i][1].isOn,
              spkSet[i].reqOn,
              spkSet[i].cSharc, spkSetSMOKs[i][0].cSharc, spkSetSMOKs[i][1].cSharc,
              spkSet[i].cPWM,   spkSetSMOKs[i][0].cPWM, spkSetSMOKs[i][1].cPWM,
              spkSet[i].signalType, spkSetSMOKs[i][0].signalType,
              spkSet[i].outputType, spkSetSMOKs[i][0].outputType,
              spkSet[i].volume[0], spkSetSMOKs[i][0].volume[0],
              spkSet[i].volume[1], spkSetSMOKs[i][1].volume[0]);
    }
}

void updateAllSMOKs(void)
{
  portNum_t p;
  for(p = PER1_PORT; p < PORTS_NUM; p+=1)
    sendKANow(p);
}

static int onOffPending = 0;
static int sendVolume(int kondPom);
static int sendSHARK(int kondPom, int msg60);

static int kondPom2SpkNum(int kondPom)
{
  portNum_t p = getPortNum(kondPom, 0);
  if(p < PER1_PORT || p >= PORTS_NUM)
    return -1;
  int spkNum = (p - PER1_PORT)>>1;
  return spkNum;
}

static int sendBroadcast(bBuffer_t *buffer, uint8_t kondPom, uint8_t tpGuiOffset)
{
  int ret = CB_RET_ACK;//In case TP is not registered
  tpGui_t *gui = tp_gui;
  int noneFound = 1;
  while (gui)
    {
      if (gui->kond_pom == kondPom)
        {
          if (buffer->data[tpGuiOffset] == gui->id) ret = 0; //Do not duplicate message guiTp
          //Send broadcast
          bBuffer_t *new = bCopy(buffer);
          new->data[tpGuiOffset] = gui->id;
          messageTx(CEN_PORT, (void *)new->data, new->size, (uint32_t)new);
          noneFound = 0;
        }
      gui = gui->next;
    }
  if(noneFound)
    {
      //Send broadcast with GUI = 0
      bBuffer_t *new = bCopy(buffer);
      new->data[tpGuiOffset] = 0x00;
      messageTx(CEN_PORT, (void *)new->data, new->size, (uint32_t)new);
      noneFound = 0;
    }
  return ret;
}

static int msgSharcCmd(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  bBuffer->size = sizeof(msg_58_t);
  msg_58_t *data = (void *)bBuffer->data;
  int pipeNum = getPipeNum(data->kondPom);
  if(pipeNum < 0)
    {
      return 0;
    }

  int spkNum =  kondPom2SpkNum(data->kondPom);
  if(spkNum < 0) return 0;
  if (DIRECTION(portNum))
    {
      *forwardMask <<= spkNum*2;
      return 0;
    }
  spkSet[spkNum].cSharc = data->Sharc_komenda;
  // send ACK broadcast
  return sendBroadcast(bBuffer, data->kondPom, offsetof(msg_58_t, guiTp));
}

static int msgPWMCmd(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  bBuffer->size = sizeof(msg_5A_t);
  msg_5A_t *data = (void *)bBuffer->data;
  int pipeNum = getPipeNum(data->kondPom);
  if(pipeNum < 0)
    {
      return 0;
    }

  int spkNum =  kondPom2SpkNum(data->kondPom);
  if(spkNum < 0) return 0;
  if (DIRECTION(portNum))
    {
      *forwardMask <<= spkNum*2;
      return 0;
    }
  spkSet[spkNum].cPWM = data->PWM_komenda;

  // send ACK broadcast
  return sendBroadcast(bBuffer, data->kondPom, offsetof(msg_5A_t, guiTp));
}


#define MSGACK(arg_code)						\
  static int msgACK##arg_code(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)	\
  {									\
    msg_##arg_code##_t *data = (void *)bBuffer->data;			\
      int spkNum =  kondPom2SpkNum(data->kondPom);			\
      if(spkNum < 0) return 0;						\
      *forwardMask <<= spkNum*2;					\
      if (DIRECTION(portNum)) return 0;					\
      return CB_RET_ACK;						\
  }

/* MSGACK(58); */
/* MSGACK(5A); */
static int msgnACK(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  if (DIRECTION(portNum)) return 0;
  return 0;
}

/* static int isOn(int kondPom) */
/* { */
/*   portNum_t p = getPortNum(kondPom, 0); */
/*   if(p < PER1_PORT || p >= PORTS_NUM) */
/*     return 0; */
/*   int spkNum = (p - PER1_PORT)>>1; */
/*   return spkSet[spkNum].isOn; */
/* } */

int sendOnOff(portNum_t portNum, int on)
{
  if(!onOffPending)
    {
      int kondPom = portData[portNum].kond_pom;
      int spkNum =  kondPom2SpkNum(kondPom);
      if(spkNum < 0) return 0;
      if(on == -1) on = spkSet[spkNum].isOn;
      if(on)
        {
          //Send 50
          bBuffer_t *msg = bAlloc(MSG_LEN);
          msg->size = sizeof(msg_50_t);
          msg_50_t *data = (void *)msg->data;
          memset(data, 0, MSG_LEN);
          data->kod = 0x50;
          data->kondPom = kondPom;
          data->cSharc = spkSet[spkNum].cSharc;
          data->cPWM = spkSet[spkNum].cPWM;
          data->guiTp = 0;
          msgGenSend(msg);
        }
      else
        {
          //Send 52
          bBuffer_t *msg = bAlloc(MSG_LEN);
          msg->size = sizeof(msg_52_t);
          msg_52_t *data = (void *)msg->data;
          memset(data, 0, MSG_LEN);
          data->kod = 0x52;
          data->kondPom = kondPom;
          data->guiTp = 0;
          msgGenSend(msg);
        }
    }
  return 0;
}



static int msgOn(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_50_t *data = (void *)bBuffer->data;
  int spkNum =  kondPom2SpkNum(data->kondPom);
  if(spkNum < 0) return 0;
  int pipeNum = getPipeNum(data->kondPom);
  if(pipeNum < 0)
    {
      return 0;
    }
  *forwardMask <<= spkNum*2;
  if (DIRECTION(portNum))
    {
      onOffPending = 1;
      spkSet[spkNum].reqOn = 1;
      spkSet[spkNum].cSharc = data->cSharc;
      spkSet[spkNum].cPWM = data->cPWM;

      //Przekaz komende dalej i czekaj na potwierdzenia
      if(cfg.ack50)
        return 0;

      spkSet[spkNum].isOn = 1;
      sendStreamInfo(pipeNum);
      sendVolume(data->kondPom);

      return CB_RET_ACK;
    }

  if(cfg.ack50)
    {
      spkSet[spkNum].isOn = 1;
      sendStreamInfo(pipeNum);
      sendVolume(data->kondPom);
    }
  onOffPending = 0;
  // send ACK broadcast
  return sendBroadcast(bBuffer, data->kondPom, offsetof(msg_50_t, guiTp));
}

static int msgNegOn(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  //  msg_51_t *data = (void *)bBuffer->data;
  onOffPending = 0;
  if(cfg.ack50)
    {
      int spkNum = (portNum - PER1_PORT)>>1;
      spkSet[spkNum].reqOn = 0;
      updateAllSMOKs();
    }
  return 0;
}

static int msgOff(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_52_t *data = (void *)bBuffer->data;
  int spkNum =  kondPom2SpkNum(data->kondPom);
  if(spkNum < 0) return 0;
  int pipeNum = getPipeNum(data->kondPom);
  if(pipeNum < 0)
    {
      return 0;
    }

  *forwardMask <<= spkNum*2;
  if (DIRECTION(portNum))
    {
      //Przekaz komende dalej i czekaj na potwierdzenia
      onOffPending = 1;
      spkSet[spkNum].reqOn = 0;
      updateAllSMOKs();
      if(cfg.ack50)
        return 0;
      return CB_RET_ACK;
    }
  onOffPending = 0;
  spkSet[spkNum].isOn = 0;
  return sendBroadcast(bBuffer, data->kondPom, offsetof(msg_52_t, guiTp));
}


static int onOffFailed(portNum_t portNum, comm_t *comm)
{
  onOffPending = 0;
  int spkNum = (portNum - PER1_PORT)>>1;
  if(cfg.ack50)
    {
      spkSet[spkNum].reqOn = 0;
    }
  return CB_RET_nACK;
}

static void syncSMOKState(portNum_t portNum)
{
  if(!onOffPending)
    {
      int spkNum = (portNum - PER1_PORT)>>1;
      //Turn off both speakers if one is off

      if((cfg.smok_mask & (1<<(portData[portNum].urzadzenie-1))) && !spkSetSMOKs[spkNum][portData[portNum].urzadzenie-1].isOn && spkSet[spkNum].reqOn)
        {
          spkSet[spkNum].reqOn = 0;
          spkSet[spkNum].isOn = 0;
          //Send 52 to GUI
          bBuffer_t *msg = bAlloc(MSG_LEN);
          msg->size = sizeof(msg_52_t);
          msg_52_t *data = (void *)msg->data;
          memset(data, 0, MSG_LEN);
          data->kod = 0x52;
          data->kondPom = portData[portNum].kond_pom;
          sendBroadcast(msg, data->kondPom, offsetof(msg_52_t, guiTp));
        }
    }
}

static int msgKeepAlive(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_55_t *data = (void *)bBuffer->data;
  int pipeNum = getPipeNum(portData[portNum].kond_pom);
  if(pipeNum < 0)
    {
      //Registerinto default pipe
      pipeNum = (data->flags55&FLAGS55_DEFAULT_PIPE)?1:0;
      pipeSetSpkSet(pipeNum, portData[portNum].kond_pom, (portNum-PER1_PORT)>>1);
    }

  portData[portNum].tsLastSeen = xTaskGetTickCount();
  portData[portNum].keepAlive = xTaskGetTickCount();

  //Get peer uptime
  uint32_t upTime;
  uint8_t *upTime8 = (uint8_t*) &upTime;
  upTime8[3] = data->uptime3;
  upTime8[2] = data->uptime2;
  upTime8[1] = data->uptime1;
  upTime8[0] = data->uptime0;
  portData[portNum].uptime = upTime;

  //Get actual SMOK state
  int spkNum = (portNum - PER1_PORT)>>1;
  spkSetSMOKs[spkNum][portData[portNum].urzadzenie-1].cSharc     = data->cSharc;
  spkSetSMOKs[spkNum][portData[portNum].urzadzenie-1].cPWM       = data->cPWM;
  spkSetSMOKs[spkNum][portData[portNum].urzadzenie-1].signalType = data->signalType;
  spkSetSMOKs[spkNum][portData[portNum].urzadzenie-1].outputType = data->outputType;
  spkSetSMOKs[spkNum][portData[portNum].urzadzenie-1].volume[0]  = data->volume;
  spkSetSMOKs[spkNum][portData[portNum].urzadzenie-1].isOn       = ((data->flags55&FLAGS55_SMOK_ON)!=0);


  syncSMOKState(portNum);

  portData[portNum].ka++;
  SPIevent(portNum, SPI_EV_TYPE_LINK_ESTABLISHED);

  // send ACK broadcast
  return sendBroadcast(bBuffer, data->kondPom, offsetof(msg_55_t, guiTp));
  return 0;
}

static int msgSharcInfo(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  bBuffer->size = sizeof(msg_60_t);
  msg_60_t *data = (void *)bBuffer->data;

  int spkNum =  kondPom2SpkNum(data->kondPom);
  if(spkNum >= 0)
    {
      switch(data->Sharc_msg2)
        {
        case 0x58:
        {
          sendSHARK(data->kondPom, data->Sharc_msg1);
        }
        break;
        case 0x80:
        {
          int thisSpk = data->urzadzenie-1;
          int complementarySpk = 1 - thisSpk;
          int newThisVolume = data->Sharc_msg1;
          int newComplementaryVolume = (int)spkSet[spkNum].volume[complementarySpk] +  newThisVolume - (int)spkSet[spkNum].volume[thisSpk];
          if(newComplementaryVolume < 0) newComplementaryVolume = 0;
          spkSet[spkNum].volume[thisSpk] = newThisVolume;
          spkSet[spkNum].volume[complementarySpk] = newComplementaryVolume;
          //Send new volumes
          sendVolume(data->kondPom);
        }
        break;
        }
    }
  // send ACK broadcast
  return sendBroadcast(bBuffer, data->kondPom, offsetof(msg_60_t, guiTp));
}

static int msgPWMInfo(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  bBuffer->size = sizeof(msg_70_t);
  msg_70_t *data = (void *)bBuffer->data;
  // send ACK broadcast
  return sendBroadcast(bBuffer, data->kondPom, offsetof(msg_70_t, guiTp));
}

static int radioReinitOnACK92 = 0;
static int kaNow[PORTS_NUM] = {0};
void sendKANow(portNum_t portNum)
{
  taskENTER_CRITICAL();
  kaNow[portNum]=1;
  messageCommNULL();
  taskEXIT_CRITICAL();
}

#define CHANGE_PIPE(arg_spkSet, arg_oldPipe)				\
  if(arg_spkSet){							\
    int pipeNum = getPipeNum(arg_spkSet);				\
    if(pipeNum == (arg_oldPipe))					\
      {									\
	/*Change pipe*/							\
	portNum_t p;							\
	for(p = PER1_PORT; p < PORTS_NUM; p+=1)				\
	  if(portData[p].kond_pom == arg_spkSet)			\
	    portData[p].reqPipe = (arg_oldPipe)?0:1;			\
	pipeSetSpkSet(-1, arg_spkSet, -1/*Find*/);			\
      }									\
  }

static int msPipeNum[PORTS_NUM] = {0};
static int msStSpkSet = {-1};
static void fillMsgActiveTransmiters(msg_90_t *data)
{
  portNum_t p;
  data->kod = 0x90;
  data->kondPomMADO = portData[CEN_PORT].kond_pom;
  if(data->konfig_ZG)return;
  data->ZkondPomA1 = 0;
  data->ZkondPomA2 = 0;
  data->ZkondPomB1 = 0;
  data->ZkondPomB2 = 0;

  //Assume multislave
  switch(rMode)
    {
    case RM_SINGLESLAVE:
      data->konfig_ZG = 4;
      data->ZkondPomA1 = portData[PER1_PORT].kond_pom;
      break;
    case RM_MULTISLAVE_STEREO:
      data->ZkondPomA1 = msStSpkSet;
      data->konfig_ZG = 3;
      break;
    case RM_MULTISLAVE:
      data->konfig_ZG = isHD()?2:1;
      for(p = PER1_PORT; p < PORTS_NUM; p+=2)
        {
          switch(msPipeNum[p])
            {
            case 0:
            {
              if(data->ZkondPomA1 == 0) data->ZkondPomA1 = portData[p].kond_pom;
              else                      data->ZkondPomA2 = portData[p].kond_pom;
            }
            break;
            case 1:
            {
              if(data->ZkondPomB1 == 0) data->ZkondPomB1 = portData[p].kond_pom;
              else                      data->ZkondPomB2 = portData[p].kond_pom;
            }
            break;
            }
        }
      break;
    default:
      data->konfig_ZG = 0;
      break;
    }
}
static const radioMode_e radioModeMap[] =
{
  [1] = RM_MULTISLAVE,
  [2] = RM_MULTISLAVE,
  [3] = RM_MULTISLAVE_STEREO,
  [4] = RM_SINGLESLAVE,
};

static int msgActiveTransmiters(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_90_t *data = (void *)bBuffer->data;
  portNum_t p;
  switch(rMode)
    {
    case RM_MULTISLAVE:
    {
      portNum_t p;
      for(p = PER1_PORT; p < PORTS_NUM; p++)
        {
          msPipeNum[p] = getPipeNum(portData[p].kond_pom);
        }
    }
    break;
    default:
      break;
    }

  if(rMode !=  RM_SINGLESLAVE && data->konfig_ZG >= 1 && data->konfig_ZG <= 3)
    {
      radioMode_e newRadioMode = radioModeMap[data->konfig_ZG];
      if(newRadioMode != rMode)
        {
          /*Change pipe*/

          if(newRadioMode == RM_MULTISLAVE_STEREO)
            {
              msStSpkSet = data->ZkondPomA1;
              rmChange(RM_MULTISLAVE_STEREO, msStSpkSet, 1);
              //Clear all spkSets
              pipeSetSpkSet(-1, -1, 0);
              pipeSetSpkSet(-1, -1, 1);
              pipeSetSpkSet(-1, -1, 2);
              pipeSetSpkSet(-1, -1, 3);
              //Set new spkSet
              pipeSetSpkSet(0, msStSpkSet, -1/*Find*/);
              //sendStreamInfoSpk(msStSpkSet);

            }
          else
            {
              rmChange(RM_MULTISLAVE, msStSpkSet, 1);
              /* pipeSetSpkSet(-1, -1, 0); */
              /* pipeSetSpkSet(-1, -1, 1); */
              /* pipeSetSpkSet(-1, -1, 2); */
              /* pipeSetSpkSet(-1, -1, 3); */
              //              pipeSetSpkSet(-1, -1, 0);
              for(p = PER1_PORT; p < PORTS_NUM; p+=2)
                if(portData[p].kond_pom > 0)
                  pipeSetSpkSet(msPipeNum[p], portData[p].kond_pom, (p-PER1_PORT)>>1);
            }

          if(newRadioMode == RM_MULTISLAVE_STEREO)
            radioReinitOnACK92 = (cfg.smok_mask == 0x3)?2:1;
          else
            radioReinit();
        }
      else
        {
          CHANGE_PIPE(data->ZkondPomA1, 1);
          CHANGE_PIPE(data->ZkondPomA2, 1);
          CHANGE_PIPE(data->ZkondPomB1, 0);
          CHANGE_PIPE(data->ZkondPomB2, 0);
        }
    }

  for(p = PER1_PORT; p < PORTS_NUM; p+=1)
    sendKANow(p);

  //Fill
  fillMsgActiveTransmiters(data);
  return CB_RET_ACK;
}

static int msgChangeFreq(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_92_t *data = (void *)bBuffer->data;

  int pipeNum = getPipeNum(data->kondPom);
  if (DIRECTION(portNum))
    {
      //Forward
      if(pipeNum < 0)
        {
          return 0;
        }
      int spkNum =  kondPom2SpkNum(data->kondPom);
      if(spkNum < 0) return 0;
      *forwardMask <<= spkNum*2;
      return 0;
    }

  if(pipeNum >= 0)
    {
      if(radioReinitOnACK92)
        {
          if((--radioReinitOnACK92) == 0)
            radioReinit();
        }
      unMute(pipeNum);
    }

  // send ACK broadcast
  sendBroadcast(bBuffer, data->kondPom, offsetof(msg_70_t, guiTp));
  return 0;
}

static void setSPDIF(int pipeNum, int konfigIn, uint8_t spkSet, outputMode_e outputMode)
{
  int source = 0;
  switch(konfigIn)
    {
    case 0x31:
      source = WM88XX_RECV_CHANNEL0;
      break;
    case 0x32:
      source = (cfg.proto & 0x04)?WM88XX_RECV_CHANNEL5:WM88XX_RECV_CHANNEL1;
      break;
    case 0x33:
      source = (cfg.proto & 0x04)?WM88XX_RECV_CHANNEL6:WM88XX_RECV_CHANNEL2;
      break;
    case 0x34:
      source = (cfg.proto & 0x04)?WM88XX_RECV_CHANNEL7:WM88XX_RECV_CHANNEL3;
      break;
    case 0x41:
    case 0x21:
      source = WM88XX_RECV_CHANNEL4;
      break;

    }
  if(pipeNum == 1)
    {
      inputChange(INPUT_SPDIF1, source, spkSet, outputMode);
    }
  else
    {
      inputChange(INPUT_SPDIF2, source, spkSet, outputMode);
    }
}
static int testPtrn = 0;
static int msgInputSet(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_94_t *data = (void *)bBuffer->data;
  int pipeNum = getPipeNum(data->kondPom);
  if(pipeNum < 0)
    return 0;
  outputMode_e outputMode;
  switch (data->msg)
    {
    case 0x1:
      outputMode = (cfg.proto & 0x08)?OM_CABLE:OM_RADIO;
      break;
    case 0x2:
      outputMode = (cfg.proto & 0x08)?OM_RADIO:OM_CABLE;
      break;
    default:
      outputMode = OM_NO_CHANGE;
      break;
    }
  switch (data->konfigWejsc)
    {
    case 0x11:
    case 0x12:
      if(pipeNum == 1)
        {
          inputChange(INPUT_SPI2, 0, data->kondPom, outputMode);
        }
      else
        {
          inputChange(INPUT_SPI1, 0, data->kondPom, outputMode);
        }
      break;
    case 0x21:
      if(cfg.proto & 0x20)
        {
          //Start stream capture
          taskENTER_CRITICAL();
          streamCaptureBuffer = bAlloc(STREAM_CAPTURE_BUFFER_SIZE);
          if(streamCaptureBuffer)
            {
              streamCaptureBuffer->offset = 0;
              streamCaptureBuffer->size = STREAM_CAPTURE_BUFFER_SIZE;
            }
          taskEXIT_CRITICAL();
          if(!streamCaptureBuffer)
            {
              xprintf("capture buffer not allocated!!!!\n");
            }
          break;
        }

      if(cfg.proto & 0x04)
        {
          //Use USB input as test patterns
          inputChange(INPUT_TEST,testPtrn , data->kondPom, outputMode);
          if (testInputPatternChange)
            testPtrn = testInputPatternChange(testPtrn);
          break;
        }

      if(cfg.proto & 0x10)
        {
	  if(pipeNum == 1)
	    inputChange(INPUT_USB2, 0, data->kondPom, outputMode);
	  else
	    inputChange(INPUT_USB1, 0, data->kondPom, outputMode);
	    
          break;
        }
    case 0x31 ... 0x34:
    case 0x41:
      setSPDIF(pipeNum, data->konfigWejsc, data->kondPom, outputMode);
      break;
    case 0x99 ... 0x99+TEST_PATTERNS_NUM-1:
      inputChange(INPUT_TEST, data->konfigWejsc - 0x99, data->kondPom, outputMode);
      if (testInputPatternChange)
        testInputPatternChange(data->konfigWejsc - 0x99);
      break;
    }

  //Send ACK broadcast
  sendBroadcast(bBuffer, data->kondPom, offsetof(msg_94_t, guiTp));
  return 0;
}

static int msgConnect(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_96_t *data     = (void *)bBuffer->data;

  int pipeNum = getPipeNum(data->kondPom);
  if(pipeNum < 0)
    {
      dprintf(LL_WARNING, "MSG96: pipe number for konPom %02x not found\n", data->kondPom);
      return 0;
    }
  int spkNum =  kondPom2SpkNum(data->kondPom);
  if(spkNum < 0) return 0;

  //  msg_96r_t *dataRet = (void*)bBuffer->data;
  int tpGui = data->guiTp;

  //Check if already registered
  tpGui_t *gui = tp_gui;
  while (gui)
    {
      if (gui->id == tpGui && gui->kond_pom == data->kondPom)
        break;
      gui = gui->next;
    }
  if (gui == NULL)
    {
      dprintf(LL_INFO, "Registered GUI/TP %d\n", tpGui);
      tpGui_t *gui = dAlloc(sizeof(tpGui_t));
      gui->next = tp_gui;
      tp_gui = gui;
      gui->kond_pom = data->kondPom;
      gui->id = tpGui;
    }
  else
    {
      dprintf(LL_WARNING, "Already registered GUI/TP %d\n", tpGui);
    }

  //Fill status
  data->gStatus = spkSet[spkNum].isOn;
  data->cSharc = spkSet[spkNum].cSharc;
  data->cPWM = spkSet[spkNum].cPWM;
  data->VolumeL = spkSet[spkNum].volume[0];
  data->VolumeR = spkSet[spkNum].volume[1];
  return CB_RET_ACK;
}

static int msgDisconnect(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_98_t *data = (void *)bBuffer->data;
  int tpGui = data->guiTp;
  tpGui_t *gui = tp_gui;
  tpGui_t *prev = NULL;
  //Check if already registered
  while (gui)
    {
      if (gui->id == tpGui && gui->kond_pom == data->kondPom)
        break;
      prev = gui;
      gui = gui->next;
    }
  if (gui != NULL)
    {
      dprintf(LL_INFO, "Deregistered GUI/TP %d\n", tpGui);
      if (prev)
        prev = gui->next;
      else
        tp_gui = gui->next;
      dFree(gui);
    }
  return CB_RET_ACK;
}

static int msgStatus(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  bBuffer->size = sizeof(msg_B0_t);
  msg_B0_t *data = (void *)bBuffer->data;
  if(!cfg.disable_alerts)
    if(data->Smok_msgType >= 0xB0 && data->Smok_msgType <= 0xBF)
      {
        int spkNum =  kondPom2SpkNum(data->kondPom);
        if(spkNum >=0)
          {
            spkSet[spkNum].reqOn = 0;
            if(spkSet[spkNum].isOn)
              {
                portNum_t p = getSecondPort(portNum);
                //Send off to the other SMOK
                if(p != CEN_PORT)
                  sendOnOff(p, 0);
              }
            updateAllSMOKs();
          }
      }

  // send ACK broadcast
  sendBroadcast(bBuffer, data->kondPom, offsetof(msg_B0_t, guiTp));
  return 0;
}

static int msgStatusLong(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  bBuffer->size = sizeof(msg_B1_t);
  msg_B1_t *data = (void *)bBuffer->data;

  if(!cfg.disable_alerts)
    if(data->Smok_msgType >= 0xB0 && data->Smok_msgType <= 0xBF)
      {
        int spkNum =  kondPom2SpkNum(data->kondPom);
        if(spkNum >=0)
          {
            spkSet[spkNum].reqOn = 0;
            if(spkSet[spkNum].isOn)
              {
                portNum_t p = getSecondPort(portNum);
                //Send off to the other SMOK
                if(p != CEN_PORT)
                  sendOnOff(p, 0);
              }
            updateAllSMOKs();
          }
      }

  // send ACK broadcast
  sendBroadcast(bBuffer, data->kondPom, offsetof(msg_B1_t, guiTp));
  return 0;
}

static int sendVolume(int kondPom)
{
  int spkNum =  kondPom2SpkNum(kondPom);
  if(spkNum < 0) return -1;
  int i = 0;
  for(i = 0; i < 2; i++)
    {
      if(cfg.smok_mask & (1<<i))
        {

          bBuffer_t *msg = bAlloc(MSG_LEN);
          msg->size = sizeof(msg_80_t);
          msg_80_t *data = (void *)msg->data;
          memset(data, 0, MSG_LEN);
          data->kod = 0x80;
          data->kondPom = kondPom;
          data->urzadzenie = 1 + i;
          data->volume = spkSet[spkNum].volume[i];
          data->guiTp = 0;
          msgGenSend(msg);
        }
    }
  return 0;
}

static int sendSHARK(int kondPom, int msg60)
{
  int spkNum =  kondPom2SpkNum(kondPom);
  if(spkNum < 0) return -1;
  bBuffer_t *msg = bAlloc(MSG_LEN);
  msg->size = sizeof(msg_58_t);
  msg_58_t *data = (void *)msg->data;
  memset(data, 0, MSG_LEN);
  data->kod = 0x58;
  data->kondPom = kondPom;
  data->urzadzenie = 0;
  data->msg60 = msg60;
  data->Sharc_komenda = spkSet[spkNum].cSharc;
  data->guiTp = 0;
  msgGenSend(msg);
  return 0;
}
static int msgVolume(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_80_t *data = (void *)bBuffer->data;
  int pipeNum = getPipeNum(data->kondPom);
  if(pipeNum < 0)
    {
      return 0;
    }
  int spkNum =  kondPom2SpkNum(data->kondPom);
  if(spkNum < 0) return 0;
  if (DIRECTION(portNum))
    {
      switch(data->urzadzenie)
        {
        case 0:
          break;
        case 1:
          *forwardMask = 0x01;
          break;
        case 2:
          *forwardMask = 0x02;
          break;
        }
      *forwardMask <<= 2*spkNum;
      return 0;
    }
  switch(data->urzadzenie)
    {
    case 0:
      spkSet[spkNum].volume[0] = data->volume;
      spkSet[spkNum].volume[1] = data->volume;
      break;
    case 1:
      spkSet[spkNum].volume[0] = data->volume;
      break;
    case 2:
      spkSet[spkNum].volume[1] = data->volume;
      break;
    }

  return sendBroadcast(bBuffer, data->kondPom, offsetof(msg_80_t, guiTp));
}

static int spk = -1;
static int msgProgram(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_9C_t *data = (void*)bBuffer->data;
  if(DIRECTION(portNum))
    {
      if(data->flags&MSG9C_FLAGS_MASTER_MASK)
        {
          *forwardMask = 0;//Do not send to slaves
          cfg.manId = (((uint32_t)data->man_id3) << 24) | (((uint32_t)data->man_id2) << 16) | (((uint32_t)data->man_id1) << 8);
          cfg.kondPom = data->kondPom;
          //Program master
          cfgFlush();
          watchdogReset();
          return CB_RET_ACK;
        }
      spk = -1;
      //if(cable) spk = 3;
      if(spk == -1 && cfg.kondPomSPK[1] == 0) spk = 0;
      if(spk == -1 && cfg.kondPomSPK[2] == 0) spk = 1;
      if(spk == -1 && cfg.kondPomSPK[3] == 0) spk = 3;
      if(spk == -1 && cfg.kondPomSPK[0] == 0) spk = 2;

      if(spk == -1)
        {
          //No free slot for slave
          *forwardMask = 0;//Do not send to slaves
          return CB_RET_nACK;
        }

      //Program slave
      return 0;
    }

  //Slave programmed. Remember in own config
  if(spk >= 0 && spk <= 3)
    {
      cfg.kondPomSPK[spk] = data->kondPom;
      cfgFlush();
    }
  return CB_RET_ACK;
}

static int msgFlash(portNum_t portNum, comm_t *comm, bBuffer_t *bBuffer, uint8_t *forwardMask)
{
  msg_A2_t *data = (void*)bBuffer->data;
  if(DIRECTION(portNum))
    {
      A2Data_t *buf = (A2Data_t *)data;
      *forwardMask = buf->mask;
      if(buf->mask == 0)
        {
          bBuffer->size = sizeof(msg_A2r_t);
          //I am the destination
          switch(buf->subCode)
            {
            case FILE_INIT:
            {
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
              //Store in flash
              int crc = buf->cmd_buffer.crc;
              buf->cmd_buffer.crc = 0;
              if(crc != crc16(0, buf, sizeof(A2Data_t)))
                return CB_RET_nACK;

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

              dprintf(LL_DEBUG, "Verifying flash image\n");
              //Calculate checksum and prepare for update
              uint32_t crc = crc32(0, flashStorage, buf->cmd_finish.size);
              if(crc != buf->cmd_finish.crc32)
                {
                  dprintf(LL_DEBUG, "Flash image NOT Ok\n");
                  return CB_RET_nACK;
                }

              dprintf(LL_DEBUG, "Flash image Ok\n");
              //Update by bootloader
              cfg.fSize = buf->cmd_finish.size;
              cfg.fCRC = buf->cmd_finish.crc32;
              cfgFlush();
              watchdogReset();//Delayed reset

              //Send ACK od nACK
              return CB_RET_ACK;
            }
            default:
              return CB_RET_nACK;
            }
        }
      else
        {
          //Forward message
          switch(buf->subCode)
            {
            case FILE_INIT:
              lockNonFlashMessages(1);
              break;
            case FILE_BUFFER:
              break;
            case FILE_STM:
            case FILE_SHARC:
              lockNonFlashMessages(0);
              break;
            default:
              break;
            }
          //Send further
          return 0;
        }
    }
  return CB_RET_ACK;
}


#define DST C_SMOK_MASK

comm_t MSG_TYPES_TAB[MSG_TYPES_QTY] =
{
  COMM_TBL_SET(50, 51, CONF_TIMEOUT,     DST, C_TIMEOUT_50, msgOn,        onOffFailed),
  COMM_TBL_SET(51, 50, NEG_CONFIRMATION,   0,    0, msgNegOn,             NULL),
  COMM_TBL_SET(52, 53, CONF_TIMEOUT,     DST, 1000, msgOff,               onOffFailed),
  COMM_TBL_SET(53, 52, NEG_CONFIRMATION,   0,    0, msgnACK,              NULL),
  COMM_TBL_SET(55, 00, MA_GEN,             0,    0, msgKeepAlive,         NULL),
  COMM_TBL_SET(58, 59, CONF_TIMEOUT,     DST, 1800, msgSharcCmd,          NULL),
  COMM_TBL_SET(59, 58, NEG_CONFIRMATION,   0,    0, NULL,                 NULL),
  COMM_TBL_SET(5A, 5B, CONF_TIMEOUT,     DST, 1800, msgPWMCmd,            NULL),
  COMM_TBL_SET(5B, 5A, NEG_CONFIRMATION,   0,    0, NULL,                 NULL),
  COMM_TBL_SET(60, 00, PER_GEN,            0,    0, msgSharcInfo,         NULL),
  COMM_TBL_SET(70, 00, PER_GEN,            0,    0, msgPWMInfo,           NULL),
  COMM_TBL_SET(80, 81, CONF_TIMEOUT,     DST,  400, msgVolume,            NULL),
  COMM_TBL_SET(81, 80, NEG_CONFIRMATION,   0,    0, msgnACK,              NULL),
  COMM_TBL_SET(90, 00, MA_GEN,             0,    0, msgActiveTransmiters, NULL),
  COMM_TBL_SET(92, 93, CONF_TIMEOUT,     DST, 3200, msgChangeFreq,        NULL),
  COMM_TBL_SET(93, 92, NEG_CONFIRMATION,   0,    0, msgnACK,              NULL),
  COMM_TBL_SET(94, 00, TP_MA,              0,    0, msgInputSet,          NULL),
  COMM_TBL_SET(96, 00, TP_MA,              0,    0, msgConnect,           NULL),
  COMM_TBL_SET(98, 00, TP_MA,              0,    0, msgDisconnect,        NULL),
  COMM_TBL_SET(9C, 9D, CONF_TIMEOUT,       1, 2100, msgProgram,           NULL),
  COMM_TBL_SET(9D, 9C, NEG_CONFIRMATION,   0,    0, NULL,                 NULL),
  COMM_TBL_SET(A0, 00, MA_GEN,             0,    0, NULL,                 NULL),
  COMM_TBL_SET(A2, A3, CONF_TIMEOUT,       0,60000, msgFlash,             NULL),
  COMM_TBL_SET(A3, A2, NEG_CONFIRMATION,   0,    0, NULL,                 NULL),
  COMM_TBL_SET(B0, 00, PER_GEN,            0,    0, msgStatus,            NULL),
  COMM_TBL_SET(B1, 00, PER_GEN,            0,    0, msgStatusLong,        NULL),
};

static int sendNetworkStatusB0(portNum_t portNum)
{
  if(portData[portNum].kond_pom == 0)
    return -2;

  uint8_t subMsgC = 0;
  uint8_t subMsgD = 0;
  switch(portData[portNum].connState)
    {
    case DEV_CONN:
      //Main connection restored
      subMsgC = 0xC3;
      //datagram restored
      subMsgD = 0xD3;
      break;
    case DEV_PCONN:
      //Main connection restored
      subMsgC = 0xC3;
      //datagram lost
      subMsgD = 0xD1;
      break;
    case DEV_NCONN:
      //Main connection lost
      subMsgC = 0xC1;
      //datagram lost
      subMsgD = 0xD1;
      break;
    default:
      return -1;
    }

  {
    //Send B0 Cx
    bBuffer_t *bBuffer = bAlloc(MSG_LEN);
    bBuffer->size = sizeof(msg_B0_t);
    msg_B0_t *data = (void*)bBuffer->data;
    data->kod = 0xB0;
    data->kondPom = portData[portNum].kond_pom;
    data->urzadzenie = portData[portNum].urzadzenie;
    data->Smok_msgType = 0;
    data->Smok_msg1 = 0;
    data->Smok_msg2 = 0;
    data->Smok_msgType = subMsgC;
    sendBroadcast(bBuffer, data->kondPom, offsetof(msg_B0_t, guiTp));
    bFree(bBuffer);
  }
  {
    //Send B0 Dx
    bBuffer_t *bBuffer = bAlloc(MSG_LEN);
    bBuffer->size = sizeof(msg_B0_t);
    msg_B0_t *data = (void*)bBuffer->data;
    data->kod = 0xB0;
    data->kondPom = portData[portNum].kond_pom;
    data->urzadzenie = portData[portNum].urzadzenie;
    data->Smok_msgType = 0;
    data->Smok_msg1 = 0;
    data->Smok_msg2 = 0;
    data->Smok_msgType = subMsgD;
    sendBroadcast(bBuffer, data->kondPom, offsetof(msg_B0_t, guiTp));
    bFree(bBuffer);
  }
  return 0;
}


int sendNetworkStatus(portNum_t portNum, netStatus_e status, uint8_t lastByte)
{
  return 0;
  //Send A0
  bBuffer_t *bBuffer = bAlloc(MSG_LEN);
  bBuffer->size = sizeof(msg_A0_t);
  msg_A0_t *data = (void *)bBuffer->data;
  data->kod = 0xA0;
  data->kondPom = portData[portNum].kond_pom;
  data->urzadzenie = portData[portNum].urzadzenie;
  data->radioMsg1 = status;
  data->radioMsg2 = lastByte;

  sendBroadcast(bBuffer, data->kondPom, offsetof(msg_A0_t, guiTp));
  bFree(bBuffer);

  sendNetworkStatusB0(portNum);
  return 0;
}

void Timeout_Data(portNum_t portNum, char *buffer, int bufferSize)
{
}

static volatile uint32_t timestamp[PORTS_NUM] =
{
  1000,
  1000+100,
  1000+200,
  1000+300,
  1000+400,
  1000+500,
  1000+600,
  1000+700,
  1000+800,
};
void sendKA(portNum_t sendPort)
{
  bBuffer_t *buf = bAlloc(MSG_LEN);
  massert(buf);
  int spkNum = (sendPort - PER1_PORT)>>1;
  int pipeNum = getPipeNum(portData[sendPort].kond_pom);

  buf->size = sizeof(msg_55_t);
  msg_55_t *msg = (void *)buf->data;
  memset(msg, 0, sizeof(msg_55_t));
  msg->kod = 0x55;

  uint32_t upTime = xTaskGetTickCount();
  uint8_t *upTime8 = (uint8_t*) &upTime;
  msg->uptime3 = upTime8[3];
  msg->uptime2 = upTime8[2];
  msg->uptime1 = upTime8[1];
  msg->uptime0 = upTime8[0];

  msg->kondPom = portData[sendPort].kond_pom;
  if(portData[sendPort].connState == DEV_CONN || portData[sendPort].connState == DEV_UCONN)
    msg->flags55 |= FLAGS55_CONNECTION_ACTIVE;
  else
    msg->flags55 &= ~(FLAGS55_CONNECTION_ACTIVE);

  if(spkSet[spkNum].reqOn)
    msg->flags55 |= FLAGS55_SMOK_ON;
  else
    msg->flags55 &= ~(FLAGS55_SMOK_ON);

  msg->cSharc     = spkSet[spkNum].cSharc;
  msg->cPWM       = spkSet[spkNum].cPWM;
  msg->signalType = spkSet[spkNum].signalType = getOutputType(pipeNum);
  msg->outputType = spkSet[spkNum].outputType = getOutputMode(pipeNum);
  msg->volume     = spkSet[spkNum].volume[portData[sendPort].urzadzenie-1];

  if(portData[sendPort].kond_pom)
    {
      if(portData[sendPort].reqPipe >= 0)
        msg->pipeNum = portData[sendPort].reqPipe;
      else
        msg->pipeNum = getPipeNum(portData[sendPort].kond_pom);

      if(twoOutputs) msg->flags55 |= FLAGS55_IS_TWO_OUTPUS;
      if(stereoSpkSet == portData[sendPort].kond_pom) msg->flags55 |= FLAGS55_AM_I_STEREO;
    }
  else
    msg->pipeNum = 0xFF;

  msg->urzadzenie = portData[sendPort].urzadzenie;
  messageTx(sendPort, (void *)buf->data, buf->size, (uint32_t)buf);
}

int tur = 0;
void communicatorPeriodicPrivate(void)
{
  
  portNum_t sendPort;
  for (sendPort = PER1_PORT; sendPort < PORTS_NUM; sendPort++)
    if (kaNow[sendPort] || timestamp[sendPort] < xTaskGetTickCount())
      {
        sendKA(sendPort);
        timestamp[sendPort] = xTaskGetTickCount() + cfg.ka_period;
        timestamp[sendPort] /= cfg.ka_period;
        timestamp[sendPort] *= cfg.ka_period;
        timestamp[sendPort] += 100*sendPort;
        taskENTER_CRITICAL();
        if(kaNow[sendPort])
          kaNow[sendPort]--;
        taskEXIT_CRITICAL();
        sendNetworkStatusB0(sendPort);
      }

  //Check link
  portNum_t portNum;
  for (portNum = PER1_PORT; portNum < PORTS_NUM; portNum++)
    {
      if (abs(portData[portNum].keepAlive - xTaskGetTickCount()) > cfg.ka_timeout)
        {
          portData[portNum].keepAlive = xTaskGetTickCount();
          if(cfg.smok_mask & (1<<(portData[portNum].urzadzenie-1)))
            {
              SPIevent(portNum, SPI_EV_TYPE_DLINK_ERROR);

              int spkNum = (portNum - PER1_PORT)>>1;
              //assume SMOK is off.
              spkSetSMOKs[spkNum][portData[portNum].urzadzenie-1].isOn = 0;
              syncSMOKState(portNum);
            }
        }
    }
}
