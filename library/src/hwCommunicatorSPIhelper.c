#include <project.h>
#include <stdint.h>
#include <common.h>
#include STMINCP()
#include STMINCS(_spi)
#include STMINCS(_gpio)
#include <bqueue.h>
#include <moduleCommon.h>
#include <hwCommunicator.h>
#include <hwCommunicatorUARThelper.h>
#include <hwCommunicatorSPIhelper.h>
#include <messageProcessor.h>
#include "FreeRTOS.h"
#include <semphr.h>
#include <task.h>
#include STMINCS(_hwInit)
#include <string.h>
#include <cc85xx_common.h>
#ifdef SMOK
#include <cc85xx_control_slv.h>
#else
#include <cc85xx_control_mst.h>
#endif
#include <cc85xx_ehif_bootloader.h>
#include <led.h>
#ifdef MADO
#include <convert.h>
#endif
#include <watchdog.h>
#ifdef RADIO_IMAGE
#include <radio_image.h>
#endif
#include <cfg.h>
#ifdef SMOK
#include <messageProcessorSMOK.h>
#include <uit_monitor.h>
#endif
#ifdef MADO
#include <messageProcessorMADO.h>
#endif
#include <crc.h>

radioModule_t radioModule[RADIO_MODULES];

enum
{
  CMD_EVENT,
  CMD_SEND_BUFFER,
  CMD_RADIO_REINIT,
};

static void hwCommunicatorRadioInit(void);
static void SPIInternalEvent(void *data);
static void sendBuffer(void *data);
static void doAlways(void *data);
static void radioReinitMsg(void *data);
static void checkDataReady(void);
static int SPI_Check(radioModule_t *rm, int eventType);
static int radioSend(portNum_t portNum, char *txBuffer, int transferSize);
static int radioReceive(radioModule_t *rm, portNum_t *portNum, char *txBuffer);
static void hwCommunicatorSPIInit(void *data);
static void checkConnection(radioModule_t *rm);

static const action_t commands[] =
{
  [CMD_EVENT] = SPIInternalEvent,
  [CMD_SEND_BUFFER]  = sendBuffer,
  [CMD_RADIO_REINIT] = radioReinitMsg,
};

static moduleModule_t moduleSPI =
{
  .queueLength = 128,
  .timeout = SPI_CHECK_FREQ,
  .commandsNum = sizeof(commands) / sizeof(commands[0]),
  .initCallback = hwCommunicatorSPIInit,
  .commandCallback = commands,
  .timeoutCallback = NULL,
  .alwaysCallback = doAlways,
  .priority = 2,
  .stackSize = 1024,
  .name = "HWCommSPIHelper",
};
static xSemaphoreHandle mutex;

/////////////////////////////////////////////////////////////////////
typedef struct
{
  uint8_t res;
} messageRadioReinit_t;

int radioReinit(void)
{
  //Allocate resources for data transfer
  messageRadioReinit_t *message = dAlloc(sizeof(messageRadioReinit_t));

  if (message == NULL)
    return -1;

  //Fill message

  //Send the message to communicator
  if (moduleSendCommand(&moduleSPI, CMD_RADIO_REINIT, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  return 0;
}

static void radioReinitMsg(void *data)
{
  messageRadioReinit_t *message = data;
  //Disconnect all ports
  portNum_t p;
  for (p = PER1_PORT; p < PORTS_NUM; p++)
    {
      portData[p].enabled = 0;

      spiHelper_t *helper = (spiHelper_t *)portData[p].SPIhelper;
      if(!helper) continue;
      if(portData[p].portType == PORT_TYPE_SPI)
        {
          if (portData[p].connState != DEV_NCONN)
            {
              portData[p].prevConnState = portData[p].connState;
              portData[p].connState = DEV_NCONN;
              connectionState(NET_NC);
              dprintf(LL_INFO, "Disconnecting peer (%08x:%08x:%08x).\n",
                      helper->connDev.devID,
                      helper->connDev.manID,
                      helper->connDev.prodID);
              switchPortHelper(p, PORT_TYPE_USART);
#ifdef MADO
              helper->connDev.devID = 0;
              helper->connDev.manID = 0;
              helper->connDev.prodID = 0;
              helper->radioModule = NULL;
              helper->connDev.audioConnState = 0;
#endif

              sendNetworkStatus(p, NET_DROPPED, (uint8_t)helper->connDev.devID);
            }
        }
    }
  //Reinit radio modules
  hwCommunicatorRadioInit();

  dFree(message);
}


typedef union
{
  struct
  {
    uint8_t portNum;
    uint8_t evType;
  } __attribute__((packed));
  uint32_t raw;
} __attribute__((packed)) ev_t;

int SPIevent(portNum_t portNum, uint8_t evType)
{
  ev_t ev = {.portNum = portNum, .evType = evType};
  if (moduleSendCommand(&moduleSPI, CMD_EVENT, (void *)ev.raw) < 0)
    {
      //Sending failed
      return -1;
    }
  return 0;
}

static int SPIeventNOW(portNum_t portNum, uint8_t evType)
{
  ev_t ev = {.portNum = portNum, .evType = evType};
  SPIInternalEvent((void *)ev.raw);
  return 0;
}

static int eventType = 1;//Timeout by default
static void SPIInternalEvent(void *data)
{
  ev_t ev = {.raw = (uint32_t)data};
  switch (ev.evType)
    {
    case SPI_EV_TYPE_DATA_READY:
      //Nothing to be done. Just don't wait for timeout to check for incoming data
      eventType = 2;
      break;
    case SPI_EV_TYPE_LINK_ERROR:
    {
      //Degrade connection status as link connection error was detected
      spiHelper_t *helper = (spiHelper_t *)portData[ev.portNum].SPIhelper;
      if(!helper) break;
      if(portData[ev.portNum].portType == PORT_TYPE_SPI)
        {
          if (portData[ev.portNum].connState != DEV_NCONN)
            {
              portData[ev.portNum].prevConnState = portData[ev.portNum].connState;
              portData[ev.portNum].connState = DEV_NCONN;
              connectionState(NET_NC);
              dprintf(LL_INFO, "Connection with peer (%08x:%08x:%08x) lost.\n",
                      helper->connDev.devID,
                      helper->connDev.manID,
                      helper->connDev.prodID);
              switchPortHelper(ev.portNum, PORT_TYPE_USART);
#ifdef MADO
              helper->connDev.devID = 0;
              helper->connDev.manID = 0;
              helper->connDev.prodID = 0;
              helper->radioModule = NULL;
              helper->connDev.audioConnState = 0;

              if(portData[portCompl[ev.portNum]].connState == DEV_NCONN)
                {
                  //Deregister speakerset from a stream
                  pipeSetSpkSet(0, -1, (ev.portNum-PER1_PORT)>>1);
                }
#else
              SmokLEDsState(-1, 0, SET_NONE, -1);
#endif

              sendNetworkStatus(ev.portNum, NET_DROPPED, (uint8_t)helper->connDev.devID);
            }
        }
      else
        {
          if (portData[ev.portNum].connState == DEV_UCONN)
            {
              portData[ev.portNum].prevConnState = portData[ev.portNum].connState;
              portData[ev.portNum].connState = DEV_NCONN;
              connectionState(NET_NC);
              dprintf(LL_INFO, "Cable connection with peer (%08x:%08x:%08x) lost.\n",
                      helper->connDev.devID,
                      helper->connDev.manID,
                      helper->connDev.prodID);
            }
        }
    }
    break;
    case SPI_EV_TYPE_DLINK_ERROR:
    {
      //Degrade connection status as link connection error was detected
      spiHelper_t *helper = (spiHelper_t *)portData[ev.portNum].SPIhelper;
      if(!helper) break;
      if(portData[ev.portNum].portType == PORT_TYPE_SPI)
        {
          if (portData[ev.portNum].connState == DEV_CONN)
            {
              portData[ev.portNum].prevConnState = portData[ev.portNum].connState;
              portData[ev.portNum].connState = DEV_PCONN;
              connectionState(NET_PARTIAL);
              dprintf(LL_INFO, "Datagram connection with peer (%08x:%08x:%08x) lost.\n",
                      helper->connDev.devID,
                      helper->connDev.manID,
                      helper->connDev.prodID);
              sendNetworkStatus(ev.portNum, NET_DATAGRAM_LOST, (uint8_t)helper->connDev.devID);

            }
        }
      else
        {
          if (portData[ev.portNum].connState == DEV_UCONN)
            {
              portData[ev.portNum].prevConnState = portData[ev.portNum].connState;
              portData[ev.portNum].connState = DEV_NCONN;
              connectionState(NET_NC);
              dprintf(LL_INFO, "Cable connection with peer (%08x:%08x:%08x) lost.\n",
                      helper->connDev.devID,
                      helper->connDev.manID,
                      helper->connDev.prodID);
            }
        }
    }
    break;
    case SPI_EV_TYPE_LINK_ESTABLISHED:
    {
      spiHelper_t *helper = (spiHelper_t *)portData[ev.portNum].SPIhelper;

      int changedState = DEV_NCONN;
      if(helper && portData[ev.portNum].portType == PORT_TYPE_SPI && portData[ev.portNum].connState == DEV_PCONN)
        changedState = DEV_CONN;//Radio full connection established
      else if ((!helper || portData[ev.portNum].portType != PORT_TYPE_SPI) && portData[ev.portNum].connState == DEV_NCONN)
        changedState = DEV_UCONN;//UART connection established

      if(changedState != DEV_NCONN)
        {
          if(changedState == DEV_CONN)
            {
              dprintf(LL_INFO, "%s: Datagram connection with peer (%08x:%08x:%08x) established.\n", portStr[ev.portNum],
                      helper->connDev.devID,
                      helper->connDev.manID,
                      helper->connDev.prodID);
            }
          else
            {
              dprintf(LL_INFO, "%s: Cable connection with peer  established.\n", portStr[ev.portNum]);
            }

          portData[ev.portNum].prevConnState = portData[ev.portNum].connState;
          portData[ev.portNum].connState = changedState;;
          portData[ev.portNum].tsConnectedDatagram = xTaskGetTickCount();
          connectionState(NET_DATAGRAM);
          sendNetworkStatus(ev.portNum, NET_DATAGRAM_CONNECTED, (uint8_t)helper->connDev.devID);
#ifdef MADO
          int pipeNum = getPipeNum(portData[ev.portNum].kond_pom);
          if(pipeNum >= 0)
            {
              pipeSync(pipeNum);
            }
#endif
        }
    }
    break;
    }
}

/////////////////////////////////////////////////////////////////////
typedef struct
{
  char *buffer;
  int bufLen;
  portNum_t portNum;
} messageSendBuffer_t;

static int SPISendBuffer(portNum_t portNum, char *buffer, int bufferLen)
{
  //Allocate resources for data transfer
  messageSendBuffer_t *message = dAlloc(sizeof(messageSendBuffer_t));

  if (message == NULL)
    return -1;

  //Fill message
  message->buffer = buffer;
  message->bufLen = bufferLen;
  message->portNum = portNum;

  //Send the message to communicator
  if (moduleSendCommand(&moduleSPI, CMD_SEND_BUFFER, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  return 0;
}

static int resetRadio(void);
static void sendBuffer(void *data)
{
  messageSendBuffer_t *message = data;
  spiHelper_t *helper = (spiHelper_t *)portData[message->portNum].SPIhelper;
  int status = 0;
  int retry = 100;
  do
    {
      if(helper->radioModule && helper->radioModule->spi.spi)
        if((status = radioSend(message->portNum, message->buffer, message->bufLen)) == 1)
          {
            retry--;
            if(retry == 0)
              {
                //ehifStart(&(helper->radioModule->spi));
                //		resetRadio();
                //ehifStop();
                checkConnection(helper->radioModule);
                portData[message->portNum].txFullDrop++;
              }
            checkDataReady();
            mdelay(1);
          }
    }
  while(status == 1 && retry != 0);

  //Send is comlete or failed.

  massert(portDataTxRxComplete(message->portNum, PORT_TYPE_SPI, RXTX_COMPLETE_TX) >= 0);
  dFree(message);
}
static void checkDataReady(void)
{
  int radioNum;
  for (radioNum = 0 ; radioNum < RADIO_MODULES; radioNum++)
    {
      radioModule_t *rm = &(radioModule[radioNum]);
      if(rm->spi.spi)
        {
          //Check for data
          while (SPI_Check(rm, eventType))
            {
              char buffer[MAX_DATA_SIZE];
              int size;
              portNum_t p = 0;
              if ((size = radioReceive(rm, &p, buffer)))
                {
                  int idx = 0;
                  spiHelper_t *helper = (spiHelper_t *)portData[p].SPIhelper;
                  while (size--) rrPut(&(helper->rr), buffer[idx++]);
                  massert(portDataTxRxComplete(p, PORT_TYPE_SPI, RXTX_COMPLETE_RX) >= 0);
                }
            }
        }
    }
}

static void doAlways(void *data)
{
#ifdef WATCHDOG_HWCOMMSPI
  wdogFeed(WATCHDOG_HWCOMMSPI(moduleNum));
#endif
  int radioNum;
  for (radioNum = 0 ; radioNum < RADIO_MODULES; radioNum++)
    {
      radioModule_t *rm = &(radioModule[radioNum]);
      if(rm->spi.spi)
        {
          //Check connection
          checkConnection(rm);
        }
    }

  checkDataReady();

  eventType = 1;//Timeout by default
}

int SPIreceiveData(portNum_t portNum, char *buffer, int maxSize)
{
  int getNum = 0;
  spiHelper_t *helper = (spiHelper_t *)portData[portNum].SPIhelper;
  while (maxSize--)
    {
      int data = rrGet(&(helper->rr));
      if (data >= 0)
        {
          *(buffer++) = data;
          getNum++;
        }
    }
  if (helper->rr.num > 0)
    massert(portDataTxRxComplete(portNum, PORT_TYPE_SPI, RXTX_COMPLETE_RX) >= 0);
  return getNum;
}

int SPIsendData (portNum_t portNum, char *buffer, int size)
{
  int putNum = 0;
  if (SPISendBuffer(portNum, buffer, size) >= 0 )
    putNum = size;
  return putNum;
}

void spiHelperInit(void)
{
  massert((mutex = xSemaphoreCreateMutex()));
  moduleSPI.privateData = &moduleSPI;
  moduleInit(&(moduleSPI));
}

static spi_t *EHIF_SPI_HOOK = NULL;

int ehifStart(spi_t *spi)
{
  massert(spi);
  if( xSemaphoreTake( mutex, portMAX_DELAY) == pdTRUE )
    {
      EHIF_SPI_HOOK = spi;
      return 0;
    }
  return -1;
}

void ehifStop(void)
{
  if(EHIF_SPI_HOOK)
    {
      EHIF_SPI_HOOK = NULL;
      xSemaphoreGive(mutex);
    }
}


#ifdef MADO
portNum_t getPortNum(int kondPom, int device)
{
  portNum_t p;
  for (p = PER1_PORT; p < PORTS_NUM; p++)
    {
      //Chek if port matching
      if(portData[p].kond_pom == kondPom)
        {
          if(portData[p].urzadzenie == device || device == 0)
            return p;
        }
    }
  return PORT_INVALID;
}
static int masterCheckLink(radioModule_t *rm)
{
  /* if(!rm->nwkStateChange) */
  /*   return 0; */
  /* rm->nwkStateChange--; */

  ehifStart(&rm->spi);
  uint16_t status = ehifGetStatus();
  uint32_t connectedMask = 0;
  if (status &  BV_EHIF_STAT_CONNECTED )  //network connection
    {
      EHIF_CMD_NWM_GET_STATUS_MASTER_DATA_T status_m;
      WAIT_BUSY;
      status_m = getStatus();
      rm->smplRate = 25*status_m.smplRate;
      rm->tsPeriod = 2000+status_m.tsPeriod*250;

      //Slave connected to master Status section:
      int idx;
      for (idx = 0; idx < status_m.wpsCount; idx++)
        {
          uint8_t kondPom = (status_m.pWpsInfo[idx].prodId& PROD_ID_KONDPOM_MASK) >> PROD_ID_KONDPOM_BIT ;
          uint8_t device =  (status_m.pWpsInfo[idx].prodId & PROD_ID_LR_MASK)>>PROD_ID_LR_BIT;
          portNum_t p = getPortNum(kondPom, device);
          if(p == PORT_INVALID)
            {
              //First time connection. Assign a stream. First if no UART connection detected. Second for UART detect??
              //For now streams are assigned
              p = PER1_PORT + device-1;
            }
          int pipeNum = portData[p].reqPipe;
          if(pipeNum < 0)
            pipeNum = getPipeNum(kondPom);
          if(pipeNum < 0)
            pipeNum = (status_m.pWpsInfo[idx].prodId & PROD_ID_PRIMARY_STREAM_MASK) >> PROD_ID_PRIMARY_STREAM_BIT;
          if(device < 1 || device > 8) continue;
          connectedMask |= (1<<p);
          {
            spiHelper_t *helper = (spiHelper_t *)portData[p].SPIhelper;
            //Check if a new connection is established
            if (portData[p].connState == DEV_NCONN || portData[p].connState == DEV_UCONN)
              {
                helper->radioModule = rm;
                portData[p].prevConnState = portData[p].connState;
                portData[p].connState = DEV_PCONN;
                portData[p].tsConnected = xTaskGetTickCount();
                connectionState(NET_PARTIAL);
                helper->connDev.devID = status_m.pWpsInfo[idx].devId;
                helper->connDev.manID = status_m.pWpsInfo[idx].mfctId;
                helper->connDev.prodID = status_m.pWpsInfo[idx].prodId;
                portData[p].kond_pom   = kondPom;
                dprintf(LL_INFO, "%s: Connection with slave (%08x:%08x:%08x)->(%08x:%08x:%08x) established.\n", portStr[p],
                        helper->radioModule->device.devID,
                        helper->radioModule->device.manID,
                        helper->radioModule->device.prodID,
                        helper->connDev.devID,
                        helper->connDev.manID,
                        helper->connDev.prodID);

                sendNetworkStatus(p, NET_JOINED, (uint8_t)helper->connDev.devID);
                switchPortHelper(p, PORT_TYPE_SPI);
                pipeSetSpkSet(pipeNum, kondPom, (p-PER1_PORT)>>1);
                sendKANow(p);
              }
            helper->connDev.audioConnState = status_m.pWpsInfo[idx].bvAchUsedByWps;
          }
        }
    }

  //Check for dropped connections
  portNum_t p;
  for (p = 0; p < PORTS_NUM; p++)
    {
      if(portData[p].portType == PORT_TYPE_SPI)
        {
          spiHelper_t *helper = (spiHelper_t *)portData[p].SPIhelper;
          if(!(connectedMask & (1<<p)) && helper->radioModule == rm)
            {
              if (portData[p].connState != DEV_NCONN)
                {
                  dprintf(LL_INFO, "%s: Lost connection with slave (%08x:%08x:%08x).\n", portStr[p],
                          helper->connDev.devID,
                          helper->connDev.manID,
                          helper->connDev.prodID);
                  SPIeventNOW(p, SPI_EV_TYPE_LINK_ERROR);
                }
            }
        }
    }
  ehifStop();
  return 0;
}
#endif
#ifdef SMOK
static int slaveSetAudioLink(spiHelper_t *helper, int channelsMask)
{
  uint8_t channelSetupTable[MAX_SLOT_NUM] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  int i;
  for(i = 0; i < 16; i++)
    if(channelsMask & (1<<i)) channelSetupTable[i] = i;

  WAIT_BUSY;
  EHIF_CMD_NWM_GET_STATUS_SLAVE_DATA_T slaveStat = getStatus();


  dprintf(LL_DEBUG, "Protocol master supports channels mask: %x\n", slaveStat.bvAchSupportedByWpm);

  activateChannels(channelSetupTable);

  return 1;
}

void slaveDisconnect(void)
{
  disconnectFromMaster();
}


static int slaveConnect(portNum_t port, remoteDev_t *device)
{
  spiHelper_t *helper = (spiHelper_t *)portData[port].SPIhelper;
  mdelay(1000);
  if (doJoin(device->devID, device->manID) )
    {
      helper->connDev = *device;
      dprintf(LL_INFO, "Connected to master (%08x:%08x:%08x). Switching to radio connection.\n",
              helper->connDev.devID,
              helper->connDev.manID,
              helper->connDev.prodID);

      switchPortHelper(port, PORT_TYPE_SPI);
      SmokLEDsState(-1, 1, SET_CONN, -1);

      slaveSetAudioLink(helper, (amIStereo == 1 || portData[CEN_PORT].kond_pom >= 0xEC)?0x7:0x3);
      portData[port].prevConnState = portData[port].connState;
      portData[port].connState = DEV_PCONN;
      sendKANow(port);
      return 0;
    }
  if(portData[port].connState != DEV_NCONN)
    portData[port].prevConnState = portData[port].connState;
  portData[port].connState = DEV_NCONN;
  return -1;
}
static int slaveCheckLink(radioModule_t *rm)
{
  ehifStart(&rm->spi);
  portNum_t p;
  for (p = 0; p < PORTS_NUM; p++)
    {
      if(portData[p].SPIhelper)
        {
          spiHelper_t *helper = (spiHelper_t *)portData[p].SPIhelper;
          uint16_t status = ehifGetStatus();
          if ( status & BV_EHIF_STAT_CONNECTED)
            {
              if(portData[p].connState == DEV_NCONN)
                {
                  portData[p].prevConnState = portData[p].connState;
                  portData[p].connState = DEV_PCONN;
                  sendKANow(p);
                }

              EHIF_CMD_NWM_GET_STATUS_SLAVE_DATA_T slaveStat = getStatus();
              helper->connDev.audioConnState = slaveStat.bvAchUsedByMe;
              rm->smplRate = 25*slaveStat.smplRate;
              rm->tsPeriod = 2000+slaveStat.tsPeriod*250;
            }
          else
            {
              if (portData[p].connState == DEV_PCONN || portData[p].connState == DEV_CONN)
                {
                  dprintf(LL_INFO, "Lost connection with master (%08x:%08x:%08x).",
                          helper->connDev.devID,
                          helper->connDev.manID,
                          helper->connDev.prodID);

                  /* if(switchPortHelper(p, PORT_TYPE_USART) == 0) */
                  /*   dprintf("Switching to cable connection.\n"); */
                  /* else */
                  dprintf(LL_INFO, "\n");
                  SPIeventNOW(p, SPI_EV_TYPE_LINK_ERROR);
                }
              else
                {
                  WAIT_BUSY;
                  EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T devInfo;
                  uint32_t masterManId[2][2] = {{0}};
                  masterManId[0][0] = (helper->radioModule->device.manID&(MAN_ID_UNIQUE_MASK | MAN_ID_STEREO_MASK));
                  masterManId[0][1] = (helper->radioModule->device.manID&(MAN_ID_UNIQUE_MASK | MAN_ID_STEREO_MASK)) | MAN_ID_LR_MASK;
                  masterManId[1][0] = (helper->radioModule->device.manID&(MAN_ID_UNIQUE_MASK | MAN_ID_STEREO_MASK)) | MAN_ID_STEREO_MASK;
                  masterManId[1][1] = (helper->radioModule->device.manID&(MAN_ID_UNIQUE_MASK | MAN_ID_STEREO_MASK)) | MAN_ID_LR_MASK | MAN_ID_STEREO_MASK;
                  int fm;
                  for(fm = 0; fm < 2; fm++)
                    {
                      int ms = (amIStereo == 1 || portData[CEN_PORT].kond_pom >= 0xEC)?1:0;
                      do
                        {
                          dprintf(LL_DEBUG, "Looking for %s %s  master with ManID %x: ",
                                  (fm==0)?"first":"second",( ms==0)?"MultiSlave":"SingleSlave", masterManId[ms][fm]);
                          devInfo = getNetworkID(masterManId[ms][fm], 1000);
                          helper->remote[fm].devID  = devInfo.deviceId;
                          helper->remote[fm].manID  = devInfo.mfctId;
                          helper->remote[fm].prodID = devInfo.prodId;
                          if(devInfo.deviceId)
                            {
                              dprintf(LL_DEBUG, "found (%08x:%08x:%08x) \n",
                                      helper->remote[fm].devID,
                                      helper->remote[fm].manID,
                                      helper->remote[fm].prodID);
                              //Do not look further
                              ms = -1;
                            }
                          else
                            {
                              dprintf(LL_DEBUG, "not found\n");
                              if(portData[CEN_PORT].kond_pom < 0xEC)
                                ms--;
                              else
                                ms = -1;
                            }
                        }
                      while(ms >= 0);
                    }

                  if (helper->remote[helper->currentRemote].devID)
                    {
                      dprintf(LL_DEBUG, "Trying to pair with master with manID = %x devId = %x \n",
                              helper->remote[helper->currentRemote].manID, helper->remote[helper->currentRemote].devID);
                      if(slaveConnect(p, &helper->remote[helper->currentRemote]) < 0)
                        {
                          dprintf(LL_WARNING, "Connection failed. Trying secondary as a fallback.\n");

                          if (helper->remote[1-helper->currentRemote].devID)
                            {
                              dprintf(LL_DEBUG, "Trying to pair with master with manID = %x devId = %x \n",
                                      helper->remote[1-helper->currentRemote].manID, helper->remote[1-helper->currentRemote].devID);
                              if(slaveConnect(p, &helper->remote[1-helper->currentRemote]) < 0)
                                {
                                  dprintf(LL_WARNING, "Connection failed.\n");
                                }
                            }
                        }
                    }
                  else
                    {
                      dprintf(LL_WARNING, "Master not found.\n");
                    }
                }
            }
        }
    }
  ehifStop();
  return 0;
}
#endif

static void checkConnection(radioModule_t *rm)
{
  //Check network status and reconnect if necessary
#ifdef MADO
  masterCheckLink(rm);
#endif
#ifdef SMOK
  //Slave. Connect to master if not not already connected
  // or connection lost.
  slaveCheckLink(rm);
#endif
}

static int SPI_Check(radioModule_t *rm, int eventType)
{
  int ret = 0;
  if (rm && rm->spi.spi)
    {
      ehifStart(&rm->spi);
      //Check if data is ready
      rm->status = ehifGetStatus();
      if(rm->status != 0xFFFF)
        {
          uint16_t clrMask = (uint32_t)rm->status & (BV_EHIF_EVT_VOL_CHG | BV_EHIF_EVT_PS_CHG | BV_EHIF_EVT_NWK_CHG | BV_EHIF_EVT_SR_CHG | BV_EHIF_EVT_DSC_RESET);
          if(clrMask)
            {
              //              if(clrMask&BV_EHIF_EVT_DSC_RESET) dprintf(LL_INFO, "Data channel has been reset\n");
              if(clrMask&BV_EHIF_EVT_VOL_CHG)   dprintf(LL_INFO, "Volume has changed\n");
              if(clrMask&BV_EHIF_EVT_PS_CHG)    dprintf(LL_INFO, "Power state has changed to %x\n", (rm->status >> 9)&0x7);
              if(clrMask&BV_EHIF_EVT_NWK_CHG)
                {
                  dprintf(LL_INFO, "Network state has changed\n");
                  rm->nwkStateChange++;
                }
              if(clrMask&BV_EHIF_EVT_SR_CHG)    dprintf(LL_INFO, "Sample rate has changed\n");
              clearFlags(clrMask);
            }
          if ( rm->status & BV_EHIF_STAT_CONNECTED )  //0xFFFF when no CS or no cable connection
            {
              if (rm->status & BV_EHIF_EVT_DSC_RX_AVAIL)
                ret =  1;
            }
        }
      ehifStop();
    }

  return ret;
}

static int radioReceive(radioModule_t *rm, portNum_t *portNum, char *rxBuffer)
{
  ehifStart(&rm->spi);
  uint32_t address = 0 ;
  uint8_t reset = 0;;
  int size = queueRX(&address, (uint8_t *)rxBuffer, &reset) ;
  ehifStop();
  if (size)
    {
      // data rx
      portNum_t p;
      for (p = 0; p < PORTS_NUM; p++)
        {
          spiHelper_t *helper = (spiHelper_t *)portData[p].SPIhelper;
          if (helper &&
              helper->radioModule &&
              helper->radioModule == rm &&
#ifdef MADO
              //For master the address is remote.
              helper->connDev.devID == address
#else
              //For slave it is slaves address
              helper->radioModule->device.devID == address
#endif
             )
            {
              *portNum = p;
              if(reset)
                portData[p].dscResetRx++;

              break;
            }
        }
      if (p == PORTS_NUM)
        {
          dprintf(LL_WARNING, "No matching port for message with address %x found.\n", address);
          return 0;
        }
    }
  else
    {
      //data not rx
    }
  return size;
}

static int radioSend(portNum_t portNum, char *txBuffer, int transferSize)
{
  spiHelper_t *helper = (spiHelper_t *)portData[portNum].SPIhelper;
  remoteDev_t *dev = &(helper->connDev);
  int sendReset = (portData[portNum].connState != DEV_CONN);
  if(txBuffer[0] == 0x55 && portData[portNum].connState == DEV_PCONN)
    massert(helper && helper->radioModule);
  ehifStart(&(helper->radioModule->spi));
  int stat = queueTX(dev, transferSize, (uint8_t *)txBuffer, sendReset);
  ehifStop();
  if(sendReset)
    {
      if(stat >= 0)
        portData[portNum].dscResetTx++;
    }

  if(stat == -1)
    helper->radioModule->txFull++;
  if(stat == -2)
    helper->radioModule->txNoConn++;

  return (stat >= 0)?0:1;
}


#ifdef RADIO_IMAGE

static void getImageChunk(uint8_t *chunk, const uint8_t *image, int offset, int size, uint32_t manId, uint32_t prodId)
{

  uint8_t *buff = intSafeMalloc(size+8);
  uint32_t prodIdbe = END_SWAP_32(prodId);
  uint32_t manIdbe = END_SWAP_32(manId);
  memcpy(buff, image-4+offset, size+8);
  int p;
  for(p = 0; p < size+8-3;)
    {
      uint32_t *id = (uint32_t*)&buff[p];
      switch(*id)
        {
        case 0x01EFCDAB:
          *id = manIdbe;
          p+= 4;
          break;
        case 0x02EFCDAB:
          *id = prodIdbe;
          p+= 4;
          break;
        default:
          p+=1;
          break;
        }
    }
  memcpy(chunk, buff+4, size);
  intSafeFree(buff);
}

static uint32_t getImageCRC(const uint8_t *image, uint32_t manId, uint32_t prodId)
{
  uint32_t imageSize = (image[0x1E] << 8) | image[0x1F];
  uint8_t *buff = intSafeMalloc(0x100);
  int sizeLeft = imageSize;
  uint32_t offset = 0;
  uint32_t crc = 0x0;
  while(sizeLeft > 0)
    {
      int chunk = (sizeLeft >= 0x100)?0x100:sizeLeft;
      getImageChunk(buff, image, offset, 0x100, manId, prodId);
      offset   += chunk;
      sizeLeft -= chunk;

      crc = crc32(crc, buff, chunk);
    }
  intSafeFree(buff);
  return END_SWAP_32(crc);
}
static uint16_t eraseProgVerifyFlash(const uint8_t* pFlashImage, uint32_t manId, uint32_t prodId)
{

  // Extract information from the image
  uint32_t imageSize = (pFlashImage[0x1E] << 8) | pFlashImage[0x1F];
  const uint32_t expectedCrcVal = getImageCRC(pFlashImage, manId, prodId);

  // Enter the SPI bootloader
  ehifBootResetSpi();
  uint16_t status = ehifBlUnlockSpi();
  if (status != EHIF_BL_SPI_LOADER_READY) return status;
  // Erase current flash contents
  status = ehifBlFlashMassErase();
  while(status == EHIF_BL_ERASE_WORKING)
    {
      mdelay(10);
      status = ehifGetStatus();
    }
  if (status != EHIF_BL_ERASE_DONE) return status;

  // For each 1 kB flash page ...
  uint16_t offset;
  uint8_t *buff = intSafeMalloc(0x400);
  for (offset = 0x0000; offset < 0x8000; offset += 0x0400)
    {
      getImageChunk(buff, pFlashImage, offset, 0x400, manId, prodId);

      // Bail out when the entire image has been programmed (it is normally less than 32 kB)
      if (offset >= imageSize) break;

      // Write the page data to RAM
      ehifSetAddr(0x6000);
      ehifWrite(0x0400, buff);

      // Program the page
      status = ehifBlFlashPageProg(0x6000, 0x8000 + offset);
      if (status != EHIF_BL_PROG_DONE)
        {
          intSafeFree(buff);
          return status;
        }
    }
  intSafeFree(buff);
  // Verify the flash contents by performing CRC-32 check. Also compare the calculated CRC with the one
  // in the image to make sure that we've actually programmed it and not just verified what was already
  // in the flash memory
  uint8_t pActualCrcVal[sizeof(uint32_t)];
  status = ehifBlFlashVerify(imageSize, pActualCrcVal);
  int n;
  for (n = 0; n < sizeof(pActualCrcVal); n++)
    {
      if (pActualCrcVal[n] != ((uint8_t*)&expectedCrcVal)[n])
        {
          status = EHIF_BL_VERIFY_FAILED;
        }
    }

  // Exit the SPI bootloader (not waiting for EHIF CMD_REQ_RDY since this will interfere with button
  // functionality on the CSn pin in autonomous operation)
  ehifSysResetSpi(1);
  ehifWaitReadyMs(300);
  ehifGetWaitReadyError();

  return status;

}

static void verifyRadioImage(int ms, int num, uint32_t manId, uint32_t prodId)
{
  uint32_t imageSize = (radioImage[ms][num][0x1E] << 8) | radioImage[ms][num][0x1F];
  uint32_t expectedCrcVal = getImageCRC((void*)radioImage[ms][num], manId, prodId);

  // Enter the SPI bootloader
  ehifBootResetSpi();
  uint16_t status = ehifBlUnlockSpi();
  if (status != EHIF_BL_SPI_LOADER_READY)
    {
      ehifSysResetSpi(1); // with wait rready flag
      ehifWaitReadyMs(300);
      ehifGetWaitReadyError();
      return;
    }
  uint32_t actualCrcVal = 0;
  ehifBlFlashVerify(imageSize, (uint8_t*)&actualCrcVal);

  ehifSysResetSpi(1); // with wait rready flag
  ehifWaitReadyMs(300);
  ehifGetWaitReadyError();

  //If crc differs then reaplce image
  if(actualCrcVal != expectedCrcVal)
    {
      dprintf(LL_WARNING, "\e[31mRadio flash invalid. Raplacing.\e[m\n");
      eraseProgVerifyFlash((void*)radioImage[ms][num], manId, prodId);
    }
  else
    dprintf(LL_INFO, "Radio flash valid.\n");

}
#endif

static void hwCommunicatorSPIInit(void *data)
{
  mdelay(100);
  hwCommunicatorRadioInit();
}

static int resetRadio(void)
{
  if(cfg.proto & 0x2)
    {
      ehifBootResetSpi();
    }
  else if (!reset_powerActive())
    {
      EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T devInfo;
      WAIT_BUSY;
      devInfo = getDeviceInfo();
      if (devInfo.deviceId != 0 && devInfo.deviceId != 0xFFFFFFFF)
        {
          clearFlags(0xFF);  //Clear them all
#ifdef MADO
          //I am master
          chipRadioEnable(ENABLE);
#endif

          setIntEvents( BV_EHIF_EVT_DSC_RX_AVAIL | BV_EHIF_EVT_DSC_RESET | BV_EHIF_EVT_NWK_CHG | BV_EHIF_EVT_DSC_RESET);
          return 1;
        }
    }
  return 0;
}

static void hwCommunicatorRadioInit(void)
{
  //Blink all leds
  LEDblink(LED_BLINKER_SPI_HELPER, ~0, 1, 500, 1, 1, 500, 0);
  int m;
  for (m = 0; m < RADIO_MODULES; m++)
    {
      //Set SPI hook for radio module HAL
      ehifStart(&radioModule[m].spi);
      EHIF_SPI_END();
      dprintf(LL_INFO, "Initializing Radio Module %d\n", m);
#ifdef RADIO_IMAGE

      uint32_t radioManId = cfg.manId;
      uint32_t radioProdId = cfg.kondPom<<PROD_ID_KONDPOM_BIT;
#ifdef MADO
      if(m == 1) radioManId |= MAN_ID_LR_MASK;
#endif
      int radioModuleSet = 0;
#ifdef SMOK
      int twoOutputs = 0;
#endif
      switch(cfg.kondPom)
        {
        case 0xEC:
        case 0xED:
        case 0xEE:
          twoOutputs = 1;
#ifdef MADO
          if(cfg.forcems)
            {
              rmChange(RM_MULTISLAVE, -1, 1);
            }
          else
            {
              rmChange(RM_SINGLESLAVE, -1, 1);
            }
#endif
          break;
        case 0xFF:
          //Factory defaults communication
#ifdef MADO
          if(rMode == RM_NONE)
            rmChange(RM_SINGLESLAVE, -1, 1);
#endif
          break;
        default:
#ifdef MADO
          if(rMode == RM_NONE)
            rmChange(RM_MULTISLAVE, -1, 1);
#endif
          break;
        }


#ifdef MADO
      switch(rMode)
        {
        case RM_MULTISLAVE:
        {
          portNum_t p;
          //Enable multislave ports
          for(p = PER1_PORT; p < PORTS_NUM; p++)
            portData[p].enabled = 1;
        }
        break;
        case RM_MULTISLAVE_STEREO:
        {
          radioManId |= MAN_ID_STEREO_MASK;
          radioModuleSet = 1;
          portNum_t p;
          //Enable multislave ports
          for(p = PER1_PORT; p < PORTS_NUM; p++)
            portData[p].enabled = 1;
        }
        break;
        case RM_SINGLESLAVE:
        {
          radioManId |= MAN_ID_STEREO_MASK;
          radioModuleSet = 1;
          portNum_t p;
          //Enable multislave ports
          for(p = PER1_PORT; p <= PER2_PORT; p++)
            portData[p].enabled = 1;
        }
        break;
        case RM_NONE:
        default:
          break;
        }
#endif
#ifdef SMOK
      radioProdId |= (((cfg.flags&FLAGS_PRIMARY_STREAM_MASK)>>FLAGS_PRIMARY_STREAM_BIT) <<PROD_ID_PRIMARY_STREAM_BIT);
      if(cfg.flags & FLAGS_RIGHT_CHANNEL_MASK)
        {
          //Right channel
          radioManId  |= MAN_ID_LR_MASK;
          radioProdId |= (2<<PROD_ID_LR_BIT);
        }
      else
        {
          //Left channel
          radioProdId |= (1<<PROD_ID_LR_BIT);
        }

#endif
      verifyRadioImage(radioModuleSet, m, radioManId, radioProdId);
#endif
      if(cfg.proto & 0x2)
        {
          ehifBootResetSpi();
        }
      else if(resetRadio())
        {
          EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T devInfo;
          WAIT_BUSY;
          devInfo = getDeviceInfo();
          if (devInfo.deviceId != 0 && devInfo.deviceId != 0xFFFFFFFF)
            {
              radioModule[m].device.devID = devInfo.deviceId;
              radioModule[m].device.manID = devInfo.mfctId;
              radioModule[m].device.prodID = devInfo.prodId;

              dprintf(LL_INFO, "Found Radio Module (%08x:%08x:%08x)\n",
                      devInfo.deviceId, devInfo.mfctId, devInfo.prodId);
#ifdef MADO
              LEDblink(LED_BLINKER_SPI_HELPER, LD4, 1, 500, 1, 1, 500, 0);
              /*
                            if(m == 0)
                              rfChannels(0xFF00);
                            else
                              rfChannels(0x00FF);
              */

              dprintf(LL_INFO, "Master initialized. Status %04x \n", (int)ehifGetStatus());
              portData[CEN_PORT].kond_pom = (devInfo.prodId >> 8)&0xFF;
#endif
#ifdef SMOK
              //I a slave.
              portData[CEN_PORT].urzadzenie = ((devInfo.prodId)&0xff);
              portData[CEN_PORT].kond_pom = (devInfo.prodId >> 8)&0xFF;
              spiHelper_t *helper = (spiHelper_t *)portData[CEN_PORT].SPIhelper;
              helper->radioModule = &radioModule[m];
              int channel = (cfg.flags & FLAGS_RIGHT_CHANNEL_MASK)?1:0;
              int pipeNum = (cfg.flags&FLAGS_PRIMARY_STREAM_MASK)?1:0;
              helper->currentRemote = (int)masterMap[channel][pipeNum][twoOutputs];

              connectionState(NET_NC);
              dprintf(LL_INFO, "Slave initialized. Status %04x \n", (int)ehifGetStatus());
              LEDblink(LED_BLINKER_SPI_HELPER, LED_WHITE, 2, 1000, 1000, 100, 1000, 0);
#endif
              ehifStop();
              continue;
            }
        }

      dprintf(LL_WARNING, "Did not find Radio Module %d\n", m);

#ifdef MADO
      LEDblink(LED_BLINKER_SPI_HELPER, LD3, 1, 1000, 100, 100, 100, 0);
#endif
#ifdef SMOK
      switchPortHelper(CEN_PORT, PORT_TYPE_USART);
      spiHelper_t *helper = (spiHelper_t *)portData[CEN_PORT].SPIhelper;
      helper->radioModule = NULL;
#endif
      radioModule[m].spi.spi = NULL;
      ehifStop();
    }
#ifdef MADO
  LEDset(LED_BLINKER_SPI_HELPER, LD5, 0);
#else
  SmokLEDsState(0, 0, SET_NONE, -1);
#endif
}

int __attribute__((weak))sendNetworkStatus(portNum_t portNum, netStatus_e status, uint8_t lastByte)
{
  //Weak placeholder.
  return 0;
}




/////////////////////////////////////////    CC85xx EHIF     /////////////////////////////////////////////////////////

/// Activates CSn, starting an SPI operation
void EHIF_SPI_BEGIN(void)
{
  if (EHIF_SPI_HOOK)
    GPIO_WriteBit(EHIF_SPI_HOOK->cs.gpio, EHIF_SPI_HOOK->cs.pin, 0);
}

/// Non-zero when EHIF is ready, zero when EHIF is not ready
char EHIF_SPI_IS_CMDREQ_READY(void)
{
  if (EHIF_SPI_HOOK)
    return readPin(EHIF_SPI_HOOK->miso.gpio, EHIF_SPI_HOOK->miso.pin);
  return 0;
}

/// Transmits a single byte
void EHIF_SPI_TX(char x)
{
  if (EHIF_SPI_HOOK)
    {
      txSPI(EHIF_SPI_HOOK->spi, x);
    }
}

/// Waits for completion of \ref EHIF_SPI_TX() (no timeout required!)
void EHIF_SPI_WAIT_TXRX(void)
{
} //No need to wait as it is done in EHIF_SPI_TX

/// The received byte after completing the last \ref EHIF_SPI_TX()
char EHIF_SPI_RX(void)
{
  if (EHIF_SPI_HOOK)
    {
      return rxSPI(EHIF_SPI_HOOK->spi);
    }
  return 0;
}

/// Deactivates CSn, ending an SPI operation
void EHIF_SPI_END(void)
{
  if (EHIF_SPI_HOOK)
    GPIO_WriteBit(EHIF_SPI_HOOK->cs.gpio, EHIF_SPI_HOOK->cs.pin, 1);
}

/// Forces the MOSI pin to the specified level
void EHIF_SPI_FORCE_MOSI(char x)
{
  if (EHIF_SPI_HOOK)
    forcePin(EHIF_SPI_HOOK->mosi.gpio, EHIF_SPI_HOOK->mosi.pin, x);
}
/// Ends forcing of the MOSI pin started by \ref EHIF_SPI_FORCE_MOSI()
void EHIF_SPI_RELEASE_MOSI(void)
{
  if (EHIF_SPI_HOOK)
    releasePin(EHIF_SPI_HOOK->mosi.gpio, EHIF_SPI_HOOK->mosi.pin);   // switch to alternate Functions
}
//-------------------------------------------------------------------------------------------------------
/// \name Reset Interface Macros
//@{

/// Activates RESETn, starting pin reset
void EHIF_PIN_RESET_BEGIN(void)
{
  if (EHIF_SPI_HOOK && EHIF_SPI_HOOK->reset.gpio)
    {
      GPIO_WriteBit(EHIF_SPI_HOOK->reset.gpio, EHIF_SPI_HOOK->reset.pin, 0);
    }

}

/// Deactivates RESETn, ending pin reset
void EHIF_PIN_RESET_END(void)
{
  if (EHIF_SPI_HOOK && EHIF_SPI_HOOK->reset.gpio)
    {
      GPIO_WriteBit(EHIF_SPI_HOOK->reset.gpio, EHIF_SPI_HOOK->reset.pin, 1);
    }
}

//uint32_t cnt = 1000;

void __attribute__((optimize("-Os")))EHIF_DELAY_US(int x)
{
  //Must not block CPUi
  //  EHIF_DELAY_MS(1);
  volatile int countDown =x*(SystemCoreClock/7000000UL);
  while(countDown--);
}
//-------------------------------------------------------------------------------------------------------




