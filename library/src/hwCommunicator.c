#include <project.h>
#include <stdint.h>
#include <common.h>
#include <bqueue.h>
#include <moduleCommon.h>
#include STMINCP()
#include STMINCS(_gpio)
#include <hwCommunicator.h>
#include <hwCommunicatorSPIhelper.h>
#include <hwCommunicatorUARThelper.h>
#include <communicator.h>
#include <messageProcessor.h>
#include "FreeRTOS.h"
#include <task.h>
#include <string.h>
#include <watchdog.h>
#include <cfg.h>


#define WAIT_QUEUE_SIZE 16

//Loopback in hwCommunicator for testing purpuses
typedef struct
{

  char *rxBuffer;
  int rxBufferSize;
  int rxExpectedSize;
  int timestamp;

  int txHead;
  int txTail;
  int txNum;
  char *txBuffer[WAIT_QUEUE_SIZE];
  int txBufferSize[WAIT_QUEUE_SIZE];
  uint32_t handle[WAIT_QUEUE_SIZE];     //handle to pass when transfer is complete
  int txBufferOffset;

} portBuffer_t;

typedef union
{
  struct
  {
    uint8_t portNum;
    uint8_t subPortNum;
  };
  uint32_t raw;
} portSubport_t;

enum
{
  //Transmit command from the communicator
  CMD_COMM_TX,
  //Transmission of data chunk to the port complete
  CMD_PORT_TX,
  //Reception of data chunk from port complete
  CMD_PORT_RX,
  //Switch poort to another peripherial
  CMD_SWITCH_PORT,
};

static void commTx(void *data);
static void portTx(void *data);
static void portRx(void *data);
static void allEvents(void *data);
static void switchPort(void *data);

static portBuffer_t portBuffer[PORTS_NUM][2]__attribute__((section(".noload")));
static const action_t commands[] =
{
  [CMD_COMM_TX] = commTx,
  [CMD_PORT_TX] = portTx,
  [CMD_PORT_RX] = portRx,
  [CMD_SWITCH_PORT] = switchPort,
};

static moduleModule_t moduleHWComm =
{
  .queueLength = 512,
  .timeout = 101,               //[ms]
  .commandsNum = ARRAY_SIZE(commands),
  .initCallback = NULL,
  .commandCallback = commands,
  .timeoutCallback = NULL,
  .alwaysCallback = allEvents,
  .priority = 2,
  .stackSize = 2 * 1024,
  .name = "portCommunicator",
};

void HWCommInit(void)
{
  //Initialize ports module
  memset(portBuffer, 0, sizeof(portBuffer));
  moduleInit(&moduleHWComm);
}


/////////////////////////////////////////////////////////////////////
// Handle message transmission
typedef struct
{
  portNum_t portNum;
  char *buffer;
  int bufferSize;
  uint32_t handle;
} messageTxData_t;

int messageTx(portNum_t portNum, char *buffer, int bufferSize, uint32_t handle)
{
  //Allocate resources for data transfer
  messageTxData_t *message = dAlloc(sizeof(messageTxData_t));

  massert(portNum < PORTS_NUM);
  if (message == NULL)
    return -1;

  //Fill message
  message->portNum = portNum;
  message->buffer = buffer;
  message->bufferSize = bufferSize;
  message->handle = handle;

  //Send the message to the communicator
  if (moduleSendCommand(&moduleHWComm, CMD_COMM_TX, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  return 0;
}

#define PORT_BUFFER portBuffer[portNum][subPortNum]
#define TX_HEAD PORT_BUFFER.txHead
#define TX_TAIL PORT_BUFFER.txTail
#define TX_NUM  PORT_BUFFER.txNum

#define TX_BUFFER_H      (PORT_BUFFER.txBuffer[TX_HEAD])
#define TX_BUFFER_SIZE_H PORT_BUFFER.txBufferSize[TX_HEAD]
#define TX_HANDLE_H      PORT_BUFFER.handle[TX_HEAD]

#define TX_BUFFER_T      (PORT_BUFFER.txBuffer[TX_TAIL])
#define TX_BUFFER_SIZE_T PORT_BUFFER.txBufferSize[TX_TAIL]
#define TX_HANDLE_T      PORT_BUFFER.handle[TX_TAIL]

#define TX_BUFFER_OFFSET PORT_BUFFER.txBufferOffset
static void commTx(void *data)
{
  messageTxData_t *message = data;
  portNum_t portNum = message->portNum;

  massert(portNum < PORTS_NUM);

  comm_t *msgType = msgTab(message->buffer[0]);
  if(msgType)
    {
      msgType->sent[portNum]++;
      if(msgType->blocked)
        {
          dFree(message->buffer);
          dFree(message);
          return;
        }
    }
  if(cfg.trace_messages)
    {
      switch(message->buffer[0])
        {
        case 0xA2:
        case 0x55:
        case 0xCC:
          if(cfg.trace_messages < 3)
            break;
        case 0xB0:
        case 0xB1:
          if(cfg.trace_messages < 2)
            break;
        default:
        {
          if(cfg.trace_messages <= 3 || cfg.trace_messages == message->buffer[0])
            {
              dprintf(LL_DEBUG, "(%010d)Msg %x sent to %s:", xTaskGetTickCount(), message->buffer[0], portStr[portNum]);
              int i;
              for(i = 0; i < message->bufferSize; i++)
                dprintf(LL_DEBUG, "%02x ", message->buffer[i]);
              dprintf(LL_DEBUG, "\n");
            }
        }
        break;
        }
    }

  int subPortNum;
  for(subPortNum = 0; subPortNum < 2; subPortNum++)
    if((subPortNum == PORT_TYPE_SPI && portData[portNum].SPIhelper)
        ||(subPortNum == PORT_TYPE_USART && portData[portNum].UARThelper))
      {
        if (TX_NUM == WAIT_QUEUE_SIZE)
          {
            //wait queue is full
            while (messageTxComplete(portNum, COMM_TX_FAILED, message->handle) < 0)
              {
                //Sending failed. Try again
                vTaskDelay(1);
              }
            continue;
          }

        dRef(message->buffer);
        TX_BUFFER_SIZE_T = message->bufferSize;
        TX_BUFFER_T = message->buffer;
        TX_HANDLE_T = message->handle;
        TX_TAIL = (TX_TAIL + 1) % WAIT_QUEUE_SIZE;
        TX_NUM++;

        //Send first data if necessary
        if (TX_NUM == 1)
          {
            massert(TX_BUFFER_OFFSET == 0);
            int sentSize = portData[portNum].sendData[subPortNum](portNum, &(TX_BUFFER_H[TX_BUFFER_OFFSET]), TX_BUFFER_SIZE_H);

            TX_BUFFER_OFFSET += sentSize;
            TX_BUFFER_SIZE_H -= sentSize;
          }
      }
  dFree(message->buffer);
  dFree(message);
}

/////////////////////////////////////////////////////////////////////
//Handle transmission.
int portDataTxRxComplete(portNum_t portNum, portType_e portType, rxTxComplete_e dir)
{
  massert(portNum < PORTS_NUM);
  portSubport_t message = {.portNum = portNum, .subPortNum = portType};
  if (dir & RXTX_COMPLETE_TX)
    {
      if (moduleSendCommand(&moduleHWComm, CMD_PORT_TX, (void *)message.raw) < 0)
        {
          //Sending failed
          return -1;
        }
    }
  if (dir & RXTX_COMPLETE_RX)
    {
      if (moduleSendCommand(&moduleHWComm, CMD_PORT_RX, (void *)message.raw) < 0)
        {
          //Sending failed
          return -1;
        }
    }
  return 0;
}

static void portTx(void *data)
{
  portSubport_t message = {.raw = (uint32_t) data};
  portNum_t portNum = message.portNum;
  massert(portNum < PORTS_NUM);
  int subPortNum = message.subPortNum;
  if (TX_BUFFER_H)
    {
      if (TX_BUFFER_SIZE_H == 0)
        {
          //All data transmitted. Send Tx Complete message
          while (messageTxComplete(portNum, COMM_TX_SUCCESS, TX_HANDLE_H) < 0)
            {
              //Sending failed. Try again
              vTaskDelay(1);
            }
          dFree(TX_BUFFER_H);
          TX_BUFFER_H = NULL;
          TX_BUFFER_OFFSET = 0;
          TX_HEAD = (TX_HEAD + 1) % WAIT_QUEUE_SIZE;
          TX_NUM--;
        }
      if (TX_NUM)
        {
          //Next buffer is waiting
          int sentSize = portData[portNum].sendData[subPortNum](portNum, &(TX_BUFFER_H[TX_BUFFER_OFFSET]), TX_BUFFER_SIZE_H);

          TX_BUFFER_OFFSET += sentSize;
          TX_BUFFER_SIZE_H -= sentSize;
        }
    }
}

static void portRx(void *data)
{

  portSubport_t message = {.raw = (uint32_t) data};
  portNum_t portNum = message.portNum;
  int subPortNum = message.subPortNum;
  massert(portNum < PORTS_NUM);

  //Get data from the port if available
  int size;

  if (PORT_BUFFER.rxBuffer == NULL)
    {
      //Allocate a buffer for the message
      PORT_BUFFER.rxBuffer = dAlloc(MAX_DATA_SIZE);
      massert(PORT_BUFFER.rxBuffer);
      PORT_BUFFER.rxBufferSize = 0;
      PORT_BUFFER.rxExpectedSize = 1;
    }
  //Get data from receiver
  while((size = portData[portNum].receiveData[subPortNum](portNum, &(PORT_BUFFER.rxBuffer[PORT_BUFFER.rxBufferSize]),
                PORT_BUFFER.rxExpectedSize - PORT_BUFFER.rxBufferSize)))
    {
      if (PORT_BUFFER.rxBufferSize == 0)
        {
          //This is a first byte of message. Get expected size and remember time for timeout.
          PORT_BUFFER.rxExpectedSize = expectedMsgSize(portNum, msgTab(PORT_BUFFER.rxBuffer[0]));
          PORT_BUFFER.timestamp = xTaskGetTickCount();
        }
      PORT_BUFFER.rxBufferSize += size;

      //Is the message complete?
      if (PORT_BUFFER.rxBufferSize >= PORT_BUFFER.rxExpectedSize)
        {
          //Connection keepalive
          if(PORT_BUFFER.rxBuffer[0] == 0x55 || (PORT_BUFFER.rxBuffer[0]&0xc0) == 0xc0)
            portData[portNum].tsLastSeenSubPort[subPortNum] = xTaskGetTickCount();
          //Send the buffer to Communicator
          if(portData[portNum].portType == subPortNum)
            {
              if (messageRx(portNum, PORT_BUFFER.rxBuffer, PORT_BUFFER.rxBufferSize, 0) >= 0)
                {
                  //Buffer is now owned by Communicator. New one needs to be allocated
                  PORT_BUFFER.rxBuffer = dAlloc(MAX_DATA_SIZE);
                  massert(PORT_BUFFER.rxBuffer);
                }
            }
          PORT_BUFFER.rxBufferSize = 0;
          PORT_BUFFER.rxExpectedSize = 1;
        }
    }
}

/////////////////////////////////////////////////////////////////////
//Handle timeouts
static void allEvents(void *data)
{
  (void)data;
  //This function is always called. Regardles of the event type.
#ifdef WATCHDOG_HWCOMM
  wdogFeed(WATCHDOG_HWCOMM(0));
#endif


  //check for receive timeout
  portNum_t portNum;
  int subPortNum;
  for (portNum = 0; portNum < PORTS_NUM; portNum++)
    for (subPortNum = 0; subPortNum < 2; subPortNum++)
      if (PORT_BUFFER.rxBufferSize > 0)
        {
          if (xTaskGetTickCount() - PORT_BUFFER.timestamp > TIMEOUT_DATA_MS)
            {
              massert(PORT_BUFFER.rxBufferSize > 0);

              //Send the buffer to Communicator
              if (messageRx(portNum, PORT_BUFFER.rxBuffer, PORT_BUFFER.rxBufferSize, 0) >= 0)
                {
                  //Buffer is now owned by Communicator. New one needs to be allocated
                  PORT_BUFFER.rxBuffer = dAlloc(MAX_DATA_SIZE);
                  massert(PORT_BUFFER.rxBuffer);
                }
              PORT_BUFFER.rxBufferSize = 0;
              PORT_BUFFER.rxExpectedSize = MAX_DATA_SIZE;
            }
        }
}


typedef struct
{
  portNum_t portNum;
  int type;
  uint8_t wait;
} messageSwitchPort_t;

int switchPortHelper(portNum_t portNum, int type)
{
  //Allocate resources for data transfer
  messageSwitchPort_t *message = dAlloc(sizeof(messageSwitchPort_t));

  massert(portNum < PORTS_NUM);
  if (message == NULL)
    return -1;

  //Fill message
  message->portNum = portNum;
  message->type = type;
  message->wait = 1;

  //Send the message to the communicator
  if (moduleSendCommand(&moduleHWComm, CMD_SWITCH_PORT, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  while(message->wait) mdelay(1);
  dFree(message);
  return 0;

}

static void switchPort(void *data)
{
  messageSwitchPort_t *message = data;
  portNum_t portNum = message->portNum;
  int type = message->type;

  massert(portNum < PORTS_NUM);
  if(type == portData[portNum].portType)
    {
      message->wait = 0;
      return;
    }

  int subPortNum;
  for(subPortNum = 0; subPortNum < 2; subPortNum++)
    {
      //Release all buffers
      if(PORT_BUFFER.rxBuffer)
        {
          dFree(PORT_BUFFER.rxBuffer);
          PORT_BUFFER.rxBuffer=NULL;
        }
      int txQueueSize;
      for(txQueueSize = 0; txQueueSize < WAIT_QUEUE_SIZE; txQueueSize++)
        if(PORT_BUFFER.txBuffer[txQueueSize])
          {
            dFree(PORT_BUFFER.txBuffer[txQueueSize]);
          }
      memset(&PORT_BUFFER, 0, sizeof(portBuffer[0][0]));
    }
  if(type == PORT_TYPE_USART && portData[portNum].UARThelper)
    {
      portData[portNum].portType    = PORT_TYPE_USART;
      message->wait = 0;
      return;
    }
  if(type == PORT_TYPE_SPI && portData[portNum].SPIhelper)
    {
      portData[portNum].portType    = PORT_TYPE_SPI;
      message->wait = 0;
      return;
    }
  portData[portNum].portType    = PORT_TYPE_NONE;
  message->wait = 0;
}

//Get byte from round robin buffer
int rrGet(roundRobin_t *rr)
{
  taskENTER_CRITICAL();
  int ch = -1;
  if (rr->num > 0)
    {
      ch = rr->buffer[rr->head];
      rr->head = (rr->head + 1) % ROUND_ROBIN_SIZE;
      rr->num--;
    }
  taskEXIT_CRITICAL();
  return ch;
}

//Put a byte into round robin buffer
int rrPut(roundRobin_t *rr, char ch)
{
  int res = 0;
  taskENTER_CRITICAL();
  if (rr->num < ROUND_ROBIN_SIZE)
    {
      rr->buffer[rr->tail] = ch;
      rr->tail = (rr->tail + 1) % ROUND_ROBIN_SIZE;
      rr->num++;
    }
  else res = -1;
  taskEXIT_CRITICAL();
  return res;
}
int NONEreceiveData(portNum_t portNum, char *buffer, int maxSize)
{
  return 0;
}

int NONEsendData (portNum_t portNum, char *buffer, int size)
{
  portDataTxRxComplete(portNum, PORT_TYPE_NONE, RXTX_COMPLETE_TX);
  return size;
}
