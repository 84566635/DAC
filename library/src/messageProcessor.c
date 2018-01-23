#include <project.h>
#include <common.h>
#include <string.h>

#include STMINCP()
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
#include <cfg.h>
#include <project.h>

#define LEDBLINK(arg_diode, arg_num) LEDblink(arg_diode, arg_num, 400, 300, 200, 400)


void messageProcessorInit(void);
void messageProcessorInit(void)
{
}

//Get MSG_TYPES_TAB entry from message code
comm_t *msgTab(uint8_t code)
{
  int i;
  for (i = 0; i < sizeof(MSG_TYPES_TAB) / sizeof(MSG_TYPES_TAB[0]); i++)
    {
      if(MSG_TYPES_TAB[i].size == 0) continue;
      if (code == MSG_TYPES_TAB[i].code)
        return &MSG_TYPES_TAB[i];//Found message in MSG_TYPES_TAB
    }
  return NULL;
}

//Get expected message type
int expectedMsgSize(portNum_t portNum, comm_t *msgType)
{
  if (msgType) return (portNum == CEN_PORT)?msgType->size:msgType->size_RX;
  return 0;
}

static void forwardMessage(bBuffer_t *bBuffer, comm_t *msgType, uint8_t forwardMask)
{
  //Store the message
  bBuffer->timestamp = xTaskGetTickCount() + msgType->timeout;
  bBuffer->privateData = forwardMask;
  bAdd(&(msgType->buffers), bBuffer);
  bRef(bBuffer);
  int bits = forwardMask;
  portNum_t sendPort = PER1_PORT;
  while(bits)
    {
      if(bits & 1)
        {
          //Send to the port
          bRef(bBuffer);
          messageTx(sendPort, (void*)bBuffer->data, bBuffer->size, (uint32_t)bBuffer);
        }
      sendPort++;
      bits >>= 1;
    }
}

void communicatorRx(portNum_t portNum, char *buffer, int bufferSize, uint8_t gen)
{

  //Check message type and size
  comm_t *msgType = msgTab(buffer[0]);
  if (!msgType)
    {
      dprintf(LL_WARNING, "Unknown message %x received on port %d\n", buffer[0], portNum);
      //Unknown message
      //LEDblink(LED_PROPOX, portNum*2, 400,200,500, 200);
      dFree(buffer);
      return;
    }

  //Check message size
  if (expectedMsgSize(portNum, msgType) > bufferSize)
    {
      dprintf(LL_WARNING, "Incomplete message %x received on port %d\n", buffer[0], portNum);
      //Timeout on receive. Message too short
      Timeout_Data(portNum, buffer, bufferSize);
      dFree(buffer);
      return;
    }

  bBuffer_t *bBuffer = bBufferFromData(buffer);
  bBuffer->size = bufferSize;
  bBuffer->timestamp = xTaskGetTickCount() + msgType->timeout;
  bBuffer->privateData = 0;
  msgType->received[portNum]++;

  if(msgType->blocked)
    {
      dFree(buffer);
      return;
    }

  if(cfg.trace_messages)
    {
      switch(buffer[0])
        {
        case 0xA2:
        case 0xCC:
        case 0x55:
          if(cfg.trace_messages < 3)
            break;
        case 0xB0:
        case 0xB1:
          if(cfg.trace_messages < 2)
            break;
        default:
        {
          if(cfg.trace_messages <= 3 || cfg.trace_messages == buffer[0])
            {
              dprintf(LL_INFO, "(%010d)Msg %x %s from %s(size %d): ", xTaskGetTickCount(), buffer[0], gen?"gen":"rcvd", portStr[portNum], bBuffer->size);
              int i;
              for(i = 0; i < bBuffer->size; i++)
                dprintf(LL_INFO, "%02x ", buffer[i]);
              dprintf(LL_INFO, "\n");
            }
        }
        break;
        }
    }

  int processLocally = 0;
  if (portNum == CEN_PORT)
    {
      //from CEN

      //Process locally if necessary
      processLocally = 1;
    }
  else
    {
      //from PER
      if (msgType->type == NEG_CONFIRMATION)
        {
          //Negative confirmation
          //Find a message refered by this nACK
          comm_t *msgACK = msgTab(msgType->nackCode);
          bBuffer_t *buf = msgACK->buffers;
          while(buf)
            {
              bBuffer_t *next = buf->next;
              if (!memcmp(&buf->data[1], &bBuffer->data[1], msgACK->size_RX-1))
                {
                  //Message found. Delete it
                  bRemove(&(msgACK->buffers), buf);
                  bFree(buf);
                  processLocally = 1;
                }
              buf = next;
            }
        }
      else
        {
          if(msgType->type == CONF_TIMEOUT)
            {
              //Positive confirmation
              //Find a message refered by this ACK
              bBuffer_t *buf = msgType->buffers;
              while(buf)
                {
                  bBuffer_t *next = buf->next;
                  if (!memcmp(buf->data, bBuffer->data, msgType->size_RX))
                    {
                      //Message found.
                      //Clear flag
                      buf->privateData &= ~(1<<(portNum - PER1_PORT));
                      if(!buf->privateData)
                        {
                          //No more confirmations needed
                          bRemove(&(msgType->buffers), buf);
                          bFree(buf);

                          processLocally = 1;
                        }
                    }
                  buf = next;
                }
            }
          else
            {
              //Other message. Send locally or send further
              processLocally = 1;
            }
        }
    }

  int send = 0;

  uint8_t forwardMask = msgType->spk_flags;
  if(processLocally)
    {
      if(msgType->type == NEG_CONFIRMATION) send |= CB_RET_nACK;
      if(msgType->type == CONF_TIMEOUT) send |= CB_RET_ACK;
      if(msgType->callback)
        send = msgType->callback(portNum, msgType, bBuffer, &forwardMask);

      if(((send & CB_RET_nACK) && msgType->type == CONF_TIMEOUT)
          || ((send & CB_RET_ACK) && msgType->type == NEG_CONFIRMATION))
        bBuffer->data[0] = msgType->nackCode;

      if(send&(CB_RET_nACK | CB_RET_ACK))
        {
          bRef(bBuffer);
          messageTx(CEN_PORT, (void*)bBuffer->data, bBuffer->size, (uint32_t)bBuffer);
        }
    }
#ifdef SMOK
  if(cfg.flags & FLAGS_FAKE_FORWARD_MASK)
    {
      //Fake forward. Respond with ACK
      if(forwardMask && DIRECTION(portNum) && (send & CB_RET_nACK) == 0)
        {
          //Send ACK
          bRef(bBuffer);
          messageTx(CEN_PORT, (void*)bBuffer->data, bBuffer->size, (uint32_t)bBuffer);
          forwardMask = 0;
        }
    }
#endif
  //Send further?
  if(forwardMask && DIRECTION(portNum) && (send & CB_RET_nACK) == 0)
    forwardMessage(bBuffer, msgType, forwardMask);


  //Release the incoming buffer
  bFree(bBuffer);
}

void msgGenSend(bBuffer_t *msg)
{
  //Simulate message receiving from CEN. It will be sent further
  messageRx(CEN_PORT, (void*)msg->data, msg->size, 1);
}

void communicatorTxComplete(portNum_t portNum, int status, uint32_t handle)
{
}

void communicatorPeriodic(void)
{
  communicatorPeriodicPrivate();
  int i;
  for (i = 0; i < sizeof(MSG_TYPES_TAB) / sizeof(MSG_TYPES_TAB[0]); i++)
    {
      if(MSG_TYPES_TAB[i].size == 0) continue;
      if(MSG_TYPES_TAB[i].type == CONF_TIMEOUT)
        {
          bBuffer_t *buf = MSG_TYPES_TAB[i].buffers;
          while(buf)
            {
              bBuffer_t *next = buf->next;

              //Now check timeout expiry
              if (buf->timestamp < xTaskGetTickCount())
                {

                  //Check which speakers did not repond. Restart link.
                  portNum_t portNum;
                  int send = CB_RET_nACK;//assume nACK
                  for(portNum = PER1_PORT; portNum < PORTS_NUM; portNum++)
                    {
                      if((buf->privateData & (1<<(portNum-1))))
                        {

#ifdef MADO
                          //                          SPIevent(portNum, SPI_EV_TYPE_LINK_ERROR);
#endif
                          if(MSG_TYPES_TAB[i].timeoutCallback)
                            send = MSG_TYPES_TAB[i].timeoutCallback(portNum,  &MSG_TYPES_TAB[i]);
                        }
                    }

                  if(buf->privateData)
                    {
                      //No confirmation came
                      //Remove message from table and send it as nACK
                      bRemove(&(MSG_TYPES_TAB[i].buffers), buf);

                      if(MSG_TYPES_TAB[i].nackCode && send)
                        {
                          //Send nACK/ACK to CEN
                          if(send &CB_RET_nACK)
                            buf->data[0] = MSG_TYPES_TAB[i].nackCode;
                          messageTx(CEN_PORT, (void*)buf->data, buf->size, (uint32_t)buf);
                        }
                      else
                        bFree(buf);
                    }
                }
              buf = next;
            }
        }
    }
}

void __attribute__((weak))connectionState(netState_e state)
{
}
void __attribute__((weak))sendKANow(portNum_t portNum)
{
}

portNum_t getSecondPort(portNum_t portNum)
{
  if(portNum >= PER1_PORT && portNum < PORTS_NUM)
    {
      if( (portNum - PER1_PORT)&1) return portNum - 1;
      else  return portNum + 1;
    }
  return CEN_PORT;
}

void lockNonFlashMessages(uint8_t lock)
{
  int i;
  for (i = 0; i < sizeof(MSG_TYPES_TAB) / sizeof(MSG_TYPES_TAB[0]); i++)
    {
      if(MSG_TYPES_TAB[i].size == 0) continue;
      switch(MSG_TYPES_TAB[i].code)
        {
        case 0xA2:
        case 0x55:
        case 0xCC:
        case 0xC1:
        case 0xC2:
          //Do not block
          break;
        default:
          MSG_TYPES_TAB[i].blocked = lock;
          break;

        }
    }

}
