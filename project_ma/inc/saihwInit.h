#ifndef SAI_HW_INIT_H_
#define SAI_HW_INIT_H_

typedef enum
{
  BLOCK_A = 0,
  BLOCK_B,
} saiBlocks_t;

// typedef enum
// {
//   _EXTERNAL = 17,
//   _44_kHz = 44,
//   _48_kHz = 48,
//   _96_kHz = 96,
//   _192_kHz = 192,
// } audioRate_t;

typedef struct __attribute__ ((__packed__))
{
  DMA_Stream_TypeDef *DMAy_Streamx;
  uint32_t DMA_Channel_x;
  uint32_t RCC_AHBPeriph_DMAx;
  IRQn_Type DMAx_Streamx_IRQn;

  uint32_t dmaItTCIF;
  uint32_t dmaItTEIF;
  uint32_t dmaItFEIF;
  uint32_t dmaItDMEIF;
  uint32_t dmaFlagTCIF;
  uint32_t dmaFlagTEIF;
  uint32_t dmaFlagFEIF;
  uint32_t dmaFlagDMEIF;

  void (*isrHandler) (void);
} constDMAvars_t;

typedef struct
{
  void (*clkSrcSelectFunction)(uint32_t);
} constSaiDefs_t;


typedef struct
{
  bQueue_t *queue;
  bBuffer_t *silenceBuf;
  const constDMAvars_t *hwDma;
  SAI_Block_TypeDef *SAI_Block_x_cooperation;
} internalDmaStruct_t;


typedef struct
{
  uint32_t DMA_MemoryDataSize;
  uint32_t DMA_PeripheralDataSize;
  uint32_t DMA_dir;
} dmaSettings_t;

typedef struct
{
  uint32_t SAI_DataSize_xb;
  uint32_t SAI_xynchronous;

  uint32_t SAI_MasterDivider_DisEna;
  uint32_t MasterDivider;

  uint32_t SAI_FrameLength;
  uint32_t SAI_ActiveFrameLength;
  uint32_t SAI_FSDefinition;
  uint32_t SAI_FS_Position;

  uint32_t SAI_SlotSize_xb;
  uint32_t slotsNum;
  uint32_t slotsActive;
} saiSettings_t;

void saiInit(void);
int saiReconfigure(int outNum, saiClkSource_t clkSrc, int freq, uint8_t half);
void printSAIBufs(int *bufSize, char *bufPtr);
#endif

