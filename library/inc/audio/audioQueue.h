
#ifndef AUDIO_QUEUE_H_
#define AUDIO_QUEUE_H_

typedef struct __attribute__ ((__packed__))
{
  unit_t *first;
  unit_t *last;
  uint32_t count;

  uint32_t bufferSize;
  uint8_t dataSize;
} queue_t;



queue_t *qInit(uint8_t dataSize, uint32_t bufferSize);

uint32_t qInsert(queue_t *q, unit_t *add);

unit_t *qRemove(queue_t *q);

uint32_t getCount(queue_t *q);

uint32_t qClear(queue_t *q);

int qReinit(queue_t *q, uint8_t dataSize, uint32_t bufferSize);

#endif
