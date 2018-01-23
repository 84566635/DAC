#include <project.h>
#include <stdint.h>
#include <common.h>
#include <moduleCommon.h>
#include <hwCommunicator.h>
#include <hwCommunicatorUARThelper.h>
#include "FreeRTOS.h"
#include <task.h>
#include <string.h>

#include STMINCP()
#include STMINCS(_usart)

//Pick data from USART Rx buffer
int USARTreceiveData(portNum_t portNum, char *buffer, int maxSize)
{
  int getNum = 0;
  while (maxSize)
    {
      int data = rrGet(&(((usartHelper_t *)portData[portNum].UARThelper)->rrRx));
      if (data < 0) break;
      *buffer = data;
      buffer++;
      maxSize--;
      getNum++;
    }
  return getNum;
}

//Put data into USART Tx buffer and initiate transmission.
int USARTsendData (portNum_t portNum, char *buffer, int size)
{
  int putNum = 0;
  taskENTER_CRITICAL();
  while (size)
    {
      if (rrPut(&(((usartHelper_t *)portData[portNum].UARThelper)->rrTx), *buffer) < 0) break;
      buffer++;
      size--;
      putNum++;
    }
  taskEXIT_CRITICAL();
  USART_ITConfig(((usartHelper_t *)portData[portNum].UARThelper)->usart, USART_IT_TXE, ENABLE);
  return putNum;
}
