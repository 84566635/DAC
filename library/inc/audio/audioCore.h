#ifndef AUDIO_CORE_H_
#define AUDIO_CORE_H_


#define AUDIO_CORE_STACK_SIZE 2048   //

#define AUDIO_CORE_QUEUE_LENGTH 5


 #include <audioTypes.h>
void audioTaskSetup( void );
uint8_t amInterface(xQueueHandle queueMsg , audioCoreMsg_t msg);

//just for debug purposes
void audioTestTaskSetup( void );

#endif