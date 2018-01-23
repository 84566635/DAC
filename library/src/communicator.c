#include <project.h>
#include <common.h>
#include <stdint.h>
#include <bqueue.h>
#include <moduleCommon.h>
#include <communicator.h>
#include <hwCommunicator.h>
#include "FreeRTOS.h"
#include <watchdog.h>


enum
{
  CMD_COMM_PORT_RX,
  CMD_COMM_PORT_TX_COMPLETE,
  CMD_ADC_MEASURE,
  CMD_COMM_NULL,
};

static void commPortRx(void *data);
static void commPortTxComplete(void *data);
static void adcMeasure(void *data);
static void commNULL(void *data);
static void timeOut(void *data);

static const action_t commands[] =
{
  [CMD_COMM_PORT_RX] = commPortRx,
  [CMD_COMM_PORT_TX_COMPLETE] = commPortTxComplete,
  [CMD_ADC_MEASURE] = adcMeasure,
  [CMD_COMM_NULL] = commNULL,
};

static moduleModule_t moduleComm =
{
  .queueLength = 256,
  .timeout = 10,               //[ms]
  .commandsNum = sizeof(commands) / sizeof(commands[0]),
  .initCallback = NULL,
  .commandCallback = commands,
  .timeoutCallback = NULL,
  .alwaysCallback = timeOut,
  .priority = 3,
  .stackSize = 2 * 1024,
  .name = "Communicator",
};

void CommunicatorInit(void)
{
  //Initialize Communicator
  moduleInit(&moduleComm);
}

/////////////////////////////////////////////////////////////////////
// Receive data from the communication port
typedef struct
{
  portNum_t portNum;
  char *buffer;
  int bufferSize;
  uint8_t gen;
} messageRxData_t;

//A function for the port module to send message to the Communicator
int messageRx(portNum_t portNum, char *buffer, int bufferSize, uint8_t gen)
{
  //Allocate resources for data transfer
  messageRxData_t *message = dAlloc(sizeof(messageRxData_t));

  if (message == NULL)
    return -1;

  //Fill message
  message->portNum = portNum;
  message->buffer = buffer;
  message->bufferSize = bufferSize;
  message->gen = gen;//Locally generated message

  //Send the message to communicator
  if (moduleSendCommand(&moduleComm, CMD_COMM_PORT_RX, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  return 0;
}

static void commPortRx(void *data)
{
  messageRxData_t *message = data;

  communicatorRx(message->portNum, message->buffer, message->bufferSize, message->gen);
  dFree(message);
}

/////////////////////////////////////////////////////////////////////
typedef struct
{
  portNum_t portNum;
  int status;
  uint32_t handle;
} messageTxCompleteData_t;

int messageTxComplete(portNum_t portNum, int status, uint32_t handle)
{
  //Allocate resources for data transfer
  messageTxCompleteData_t *message = dAlloc(sizeof(messageTxCompleteData_t));

  if (message == NULL)
    return -1;

  //Fill message
  message->portNum = portNum;
  message->status = status;
  message->handle = handle;

  //Send the message to communicator
  if (moduleSendCommand(&moduleComm, CMD_COMM_PORT_TX_COMPLETE, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  return 0;
}

static void commPortTxComplete(void *data)
{
  messageTxCompleteData_t *message = data;

  communicatorTxComplete(message->portNum, message->status, message->handle);
  dFree(message);
}

/////////////////////////////////////////////////////////////////////
typedef struct
{
  int samplesNum;
  int sampleSensADC;
  int V1accu;
  int V2accu;
  int I1max;
  int V2I1max;
  int I1accu;
  int I2accu;
} messageADCMeasureData_t;

int messageADCMeasure(int samplesNum,int sampleSensADC,
                      int V1accu, int V2accu,
                      int I1max, int V2I1max, int I1accu, int I2accu)
{
  //Allocate resources for data transfer
  messageADCMeasureData_t *message = dAlloc(sizeof(messageADCMeasureData_t));

  if (message == NULL)
    return -1;

  //Fill message
  message->samplesNum = samplesNum;
  message->sampleSensADC = sampleSensADC;
  message->V1accu = V1accu;
  message->V2accu = V2accu;
  message->I1max  = I1max;
  message->V2I1max= V2I1max;
  message->I1accu = I1accu;
  message->I2accu = I2accu;

  //Send the message to communicator
  if (moduleSendCommand(&moduleComm, CMD_ADC_MEASURE, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  return 0;
}

static void adcMeasure(void *data)
{
  messageADCMeasureData_t *message = data;

  communicatorADCMeasure(message->samplesNum, message->sampleSensADC,
                         message->V1accu, message->V2accu,
                         message->I1max, message->V2I1max, message->I1accu, message->I2accu);
  dFree(message);
}

/////////////////////////////////////////////////////////////////////
typedef struct
{
  int null;
} messageCommNULL_t;

int messageCommNULL(void)
{
  //Allocate resources for data transfer
  messageCommNULL_t *message = dAlloc(sizeof(messageCommNULL_t));

  if (message == NULL)
    return -1;

  //Send the message to communicator
  if (moduleSendCommand(&moduleComm, CMD_COMM_NULL, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  return 0;
}

static void commNULL(void *data)
{
  messageCommNULL_t *message = data;

  dFree(message);
}

/////////////////////////////////////////////////////////////////////
static void timeOut(void *data)
{
  communicatorPeriodic();
#ifdef WATCHDOG_COMM
  wdogFeed(WATCHDOG_COMM(0));
#endif

}
void __attribute__((weak)) communicatorADCMeasure(int samplesNum,int sampleSensADC,
    int V1accu, int V2accu,
    int I1max, int V2I1max, int I1accu, int I2accu)
{
}
