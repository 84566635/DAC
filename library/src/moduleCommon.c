/* Standard includes. */
#include <string.h>
#include "common.h"
#include "startup.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "moduleCommon.h"

typedef struct
{
  int command;
  void *data;
} queueMessage_t;

int moduleSendCommand(moduleModule_t * moduleTaskData, int command, void *data)
{
  if (moduleTaskData->queueHandle == NULL)
    {
      //Queue not initialized
      //massert(0);
      return -1;
    }
  massert(command >=0 && command < moduleTaskData->commandsNum);
  queueMessage_t message =
  {
    .command = command,
    .data = data,
  };
  portBASE_TYPE status;

  //Check if this is a call from interrupt and use proper enqueue
  if (amIInIRQ())
    {
      portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

      status = xQueueSendFromISR(moduleTaskData->queueHandle, &message, &xHigherPriorityTaskWoken);

      //Switch context if necessary
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  else
    {
      status = xQueueSend(moduleTaskData->queueHandle, &message, 0 /*Do not wait for a free entry */ );
    }
  taskENTER_CRITICAL();
  if(status)
    moduleTaskData->enqueues++;
  else
    moduleTaskData->fenqueues++;
  taskEXIT_CRITICAL();
  return (status == pdTRUE) ? 0 : -1;
}

static void moduleTask(void *pvParameters)
{
  // Get module data
  moduleModule_t *moduleTaskData = (moduleModule_t *) pvParameters;
  queueMessage_t message;

  taskWaitInit();

  if(moduleTaskData->initCallback)
    moduleTaskData->initCallback(moduleTaskData->privateData);

  while(1)
    {
      //receive a command from the queue. Wait only a specified time.
      if (xQueueReceive(moduleTaskData->queueHandle, (void *)&message, moduleTaskData->timeout) == pdTRUE)
        {
          moduleTaskData->dequeues++;
          massert(message.command >=0 && message.command < moduleTaskData->commandsNum);
          if (moduleTaskData->commandsNum > message.command && moduleTaskData->commandCallback
              && moduleTaskData->commandCallback[message.command])
            moduleTaskData->commandCallback[message.command] (message.data);
        }
      else
        {
          //Timeout. No command received in the specified time
          if (moduleTaskData->timeoutCallback)
            moduleTaskData->timeoutCallback(moduleTaskData);
        }

      //Either timeout or command
      if (moduleTaskData->alwaysCallback)
        moduleTaskData->alwaysCallback(moduleTaskData);
    }
}

void moduleInit(moduleModule_t * moduleTaskData)
{
  //Create a queue for command reception
  massert((moduleTaskData->queueHandle = xQueueCreate(moduleTaskData->queueLength, sizeof(queueMessage_t))));

  //Create the task
  massert(xTaskGenericCreate(moduleTask,
                             (signed char *)moduleTaskData->name, moduleTaskData->stackSize/4, (void *)moduleTaskData, moduleTaskData->priority, NULL,
                             moduleTaskData->stack, NULL) == pdPASS);
}
