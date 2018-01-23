#ifndef ISR_HANDLERS_PROTOTYPES_H_
#define ISR_HANDLERS_PROTOTYPES_H_

void DMA_SPI_audioStream_ISR(void);
void SAI1_IRQHandler(void);
void SAI_IRQHandler(void);
void BlockA_IRQHandler(void);
void BlockB_IRQHandler(void);

void isrDmaTC_BlockA_CallBack(uint32_t *usedBuff, signed portBASE_TYPE *xHigherPriorityTaskWoken);
void isrDmaTC_BlockB_CallBack(uint32_t *usedBuff, signed portBASE_TYPE *xHigherPriorityTaskWoken);

void inputAudioI2S_IRQHandler(void);
#endif
