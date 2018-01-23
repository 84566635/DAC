#include <stdio.h>
#include <string.h>
#include <project.h>
#include <common.h>
#include STMINCP()
#include STMINCS(_gpio)
#include "FreeRTOS.h"
#include <task.h>
#include <semphr.h>
#include <audioTypes.h>
#include <audioQueue.h>


queue_t *qInit(uint8_t dataSize, uint32_t bufferSize)
{
  massert(dataSize <= 4);
  queue_t *handler = intSafeMalloc(sizeof(queue_t));
  massert(handler);
  handler->first = NULL;
  handler->last = NULL;
  handler->count = 0;

  handler->dataSize = dataSize;
  handler->bufferSize = bufferSize;

  return handler;
}

int qReinit(queue_t *q, uint8_t dataSize, uint32_t bufferSize)
{
  if (!q)
    return 0;

  taskENTER_CRITICAL();

  //check if it is possible to change queue elements size (only if no elements in queue)
  if (   (dataSize * bufferSize != (q->dataSize) * (q->bufferSize))  &&  (q->count != 0)   )
    return 0;

  q->dataSize = dataSize;
  q->bufferSize = bufferSize;
  taskEXIT_CRITICAL();

  return 1;
}


uint32_t qInsert(queue_t *q, unit_t *add)
{
  uint32_t ret = 0;
  taskENTER_CRITICAL();
  if (!add || !q)
    return 0;
  if (q->first == NULL)
    {
      massert(!q->count);
      massert(!q->last);
      add->prev = NULL;
      add->next = NULL;
      q->first = add;
      q->last = add;
      q->count++;
      ret = 1;
    }
  else if (q->last != NULL)
    {
      q->last->next = add;
      add->next = NULL;
      add->prev = q->last;
      q->last = add;
      q->count++;
      ret = 1;
    }
  else
    {
      ret = 0;
    }

  taskEXIT_CRITICAL();
  return ret;
}

unit_t *qRemove(queue_t *q)
{
  taskENTER_CRITICAL();
  unit_t *ret = q->first;
  if (ret)
    {
      q->first = ret->next;
      if (q->first) //taken element was the last in queue
        {
          q->first->prev = NULL;
        }
      else
        {
          q->last = NULL; //
        }
      q->count--;

    }else{
      massert(!q->count);
    }
  taskEXIT_CRITICAL();

  return ret;
}

uint32_t qClear(queue_t *q)
{
  uint32_t ret = 0;
  if (!q)
    return 0;

  unit_t *ptr;
  taskENTER_CRITICAL();
  while (getCount(q))
    {
      ptr = qRemove(q);
      if (ptr)
        intSafeFree(qRemove(q));
      ret++;
    }
  taskEXIT_CRITICAL();
  return ret;
}



uint32_t getCount(queue_t *q)
{
  return q->count;
}
