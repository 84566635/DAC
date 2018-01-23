#include <string.h>
#include <stdint.h>
#include "common.h"
#include "project.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "bqueue.h"
#include <cfg.h>

#define bEnterCS(...) taskENTER_CRITICAL()
#define bExitCS(...)  taskEXIT_CRITICAL()

void bAdd(bBuffer_t **first, bBuffer_t *buf)
{
  buf->next = *first;
  *first = buf;
}

void bRemove(bBuffer_t **first, bBuffer_t *buf)
{
  if (*first == buf)
    {
      *first = buf->next;
      buf->next = NULL;
    }
  else
    {
      bBuffer_t *prev = *first;
      while (prev && prev->next != buf)
        prev = prev->next;
      if (prev)
        {
          prev->next = buf->next;
          buf->next = NULL;
        }
    }
}

int bQueueInit(bQueue_t *queue, int maxCount, int belowCount, int aboveCount, int refillCount, void (*callback)(unsigned int num))
{
  queue->count      = 0;
  queue->callback   = callback;
  queue->maxCount   = maxCount;
  queue->belowCount = belowCount;
  queue->aboveCount = aboveCount;
  queue->refillCount = refillCount;
  queue->head       = NULL;
  queue->tail       = NULL;
  queue->full       = 0;
  queue->enqueues   = 0;
  queue->dequeues   = 0;
  queue->maxFilling = 0;
  queue->refilling  = 0;
  queue->refillCount = refillCount;
  return 0;
}
int __attribute__((optimize("-O4")))bEnqueue(bQueue_t *queue, bBuffer_t *buffer)
{
  if (queue->callback && queue->count > queue->aboveCount)
    queue->callback(queue->count);

  if (queue->count == queue->maxCount)
    {
      queue->full++;
      return -1;
    }
  bEnterCS();
  buffer->next = NULL;
  if (queue->head)
    {
      queue->tail->next = buffer;
      queue->tail = buffer;
    }
  else
    {
      queue->tail = buffer;
      queue->head = buffer;
    }
  queue->enqueues++;
  queue->count++;
  if (queue->count > queue->maxFilling)
    queue->maxFilling = queue->count;


  bExitCS();
  return 0;
}

bBuffer_t __attribute__((optimize("-O4"))) *bDequeue(bQueue_t *queue)
{
  unsigned int count = 0;
  bBuffer_t *buffer = NULL;
  bEnterCS();
  if (queue->refilling)
    {
      if (queue->refillCount < queue->count)
        queue->refilling = 0;
    }

  if (!queue->refilling)
    {
      buffer = queue->head;
      if (buffer)
        {
          queue->head = queue->head->next;
          buffer->next = NULL;
          count = --queue->count;
          queue->dequeues++;
        }
      else
        {
          queue->empty++;
          if (queue->refillCount > queue->count)
            queue->refilling = 1;
        }
    }
  else
    queue->empty++;

  bExitCS();

  //Call a callback if number of elements in queue went below initialized value
  if (queue->callback && (count < queue->belowCount || count > queue->aboveCount))
    queue->callback(count);

  return buffer;
}

#define ALLOC_BUFFER(arg_min_size, arg_size)    \
  case arg_min_size ... arg_size:		\
  buffer = bDequeue(&heap_##arg_min_size##_##arg_size);		\
  alloc_size = arg_size;			\
  break;

#define FREE_BUFFER(arg_min_size, arg_size)	\
  case arg_min_size ... arg_size:		\
  if(bEnqueue(&heap_##arg_min_size##_##arg_size, buffer) ==0)	\
    buffer = NULL;				\
  break;


//The most possible sizes
bQueue_t heap_1_32  = BQUEUE_STATIC_INIT(30, 0, 0, 0, NULL);
bQueue_t heap_33_192 = BQUEUE_STATIC_INIT(10, 0, 0, 0, NULL);
bQueue_t heap_512_512 = BQUEUE_STATIC_INIT(15, 0, 0, 0, NULL);
bQueue_t heap_513_1024 = BQUEUE_STATIC_INIT(15, 0, 0, 0, NULL);
bQueue_t heap_1025_2112 = BQUEUE_STATIC_INIT(5, 0, 0, 0, NULL);
unsigned int heapAllocs    = 0;
unsigned int heapFrees     = 0;
unsigned int heapAllocated = 0;
unsigned int heapLastSize  = 0;
bBuffer_t __attribute__((optimize("-O4"))) *bAlloc(int size)
{
  bBuffer_t *buffer = NULL;
  int alloc_size = size;
  switch (size)
    {
      ALLOC_BUFFER(  1,   32);
      ALLOC_BUFFER( 33,  192);
      ALLOC_BUFFER(512,  512);
      ALLOC_BUFFER(513, 1024);
      ALLOC_BUFFER(1025,2112);
    }
  if (!buffer)
    {
      //No suitable buffer on queue. Alloc from heap
      buffer = intSafeMalloc(alloc_size + sizeof(bBuffer_t));
      if (buffer)
        memset(buffer, 0x00, alloc_size);
      heapLastSize = alloc_size;
      bEnterCS();
      heapAllocs++;
      heapAllocated++;
      bExitCS();
    }
  if (buffer)
    {
      if(cfg.empty_buffers)
        memset(buffer->data, 0, size);
      buffer->maxSize = size;
      buffer->size = 0;
      buffer->offset = 0;
      buffer->ref = 1;
      buffer->next = NULL;
    }
  return buffer;
}
void __attribute__((optimize("-O4")))bFree(bBuffer_t *buffer)
{
  bEnterCS();
  int ref = --buffer->ref;
  bExitCS();

  if (ref == 0)
    {
      switch (buffer->maxSize)
        {
          FREE_BUFFER(  1,   32);
          FREE_BUFFER( 33,  192);
          FREE_BUFFER(512,  512);
          FREE_BUFFER(513, 1024);
          FREE_BUFFER(1025,2112);
        }
      if (buffer)
        {
          //No suitable buffer queue.
          intSafeFree(buffer);
          bEnterCS();
          heapFrees++;
          heapAllocated--;
          bExitCS();
        }
    }
}

void __attribute__((optimize("-O4")))bRef(bBuffer_t *buffer)
{
  bEnterCS();
  buffer->ref++;
  bExitCS();
}

bBuffer_t *__attribute__((optimize("-O4")))bCopy(bBuffer_t *buffer)
{
  bBuffer_t *new = bAlloc(buffer->maxSize);
  memcpy(new, buffer, buffer->maxSize + sizeof(bBuffer_t));
  return new;
}
