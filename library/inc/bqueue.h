#ifndef _BBUFFER_H
#define _BBUFFER_H
typedef struct bBuffer_s
{
  uint16_t size;
  uint16_t maxSize;
  uint16_t offset;
  uint16_t ref;
  uint32_t timestamp;
  uint32_t privateData;//may be casted to a pointer
  struct bBuffer_s *next;
  uint8_t  data[];
} bBuffer_t;

#define bBufferFromData(arg_data)  (bBuffer_t*)(&((uint8_t*)arg_data)[-offsetof(bBuffer_t, data)])
#define bDataFromBuffer(arg_buffer) (void*)(arg_buffer)->data

void bAdd(bBuffer_t **first, bBuffer_t *buf);
void bRemove(bBuffer_t **first, bBuffer_t *buf);
//Allocate/Free buffer
bBuffer_t *bAlloc(int size);
void bFree(bBuffer_t *buffer);
void bRef(bBuffer_t *buffer);
bBuffer_t *bCopy(bBuffer_t *buffer);
//Allocate/Free buffer (hide a header. Export data buffer only)
#define dAlloc(arg_size) ({bBuffer_t *buf = bAlloc(arg_size); (buf != NULL)?bDataFromBuffer(buf):NULL;})
#define dFree(arg_buffer) bFree(bBufferFromData(arg_buffer))
#define dRef(arg_buffer) bRef(bBufferFromData(arg_buffer))

typedef struct
{
  bBuffer_t *head;
  bBuffer_t *tail;
  unsigned int count;
  unsigned int maxCount;
  unsigned int belowCount;
  unsigned int aboveCount;

  int refillCount:31; //Min level to refill when encountered empty
  int refilling:1;

  void (*callback)(unsigned int count);

  //statistics
  unsigned int dequeues;
  unsigned int enqueues;
  unsigned int full;
  unsigned int empty;
  unsigned int maxFilling;
} bQueue_t;
#define BQUEUE_STATIC_INIT(arg_maxCount, arg_belowCount, arg_aboveCount, arg_refillCount, arg_callback) \
  {.head=NULL, .tail=NULL, .count=0, .maxCount = arg_maxCount, .belowCount=arg_belowCount, \
      .aboveCount=arg_aboveCount, .callback=arg_callback,		\
      .full=0, .enqueues=0, .dequeues=0, .maxFilling=0, .refilling = 0, .refillCount = arg_refillCount,}

int bQueueInit(bQueue_t *queue, int maxCount, int belowNum, int aboveNum, int refillCount, void (*callback)(unsigned int num));
int bEnqueue(bQueue_t *queue, bBuffer_t *buffer);
bBuffer_t *bDequeue(bQueue_t *queue);
#define dEnqueue(arg_queue, arg_buffer) bEnqueue(arg_queue, bBufferFromData(buffer))
#define dDequeue(arg_queue) ({bBuffer_t *buf = bDequeue(arg_queue); (buf != NULL)?bDataFromBuffer(buf):NULL;})

extern bQueue_t heap_1_32;
extern bQueue_t heap_33_192;
extern bQueue_t heap_512_512;
extern bQueue_t heap_513_1024;
extern bQueue_t heap_1025_2112;

extern unsigned int heapAllocs;
extern unsigned int heapFrees;
extern unsigned int heapAllocated;
extern unsigned int heapLastSize;

#endif
