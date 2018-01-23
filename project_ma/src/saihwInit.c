#include <project.h>
#include <hwProfile.h>
#include <common.h>
#include <string.h>
#include <misc.h>
#include <startup.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <debug.h>

#include STMINCP()
#include STMINCS(_gpio)
#include STMINCS(_rcc)
#include STMINCS(_sai)
#include STMINCS(_dma)
#include STMINCS(_hwInit)

#include <bqueue.h>
#include <convert.h>
#include <audioTypes.h>
#include <saihwInit.h>

static audioRate_t mapFreq(int freq);
static int unmapFreq(audioRate_t rate);
static void BlockA_IRQHandler(void);
static void BlockB_IRQHandler(void);
static void SAI_IRQHandler(void);
static int audioPause(SAI_Block_TypeDef *SAI_Block_x, FunctionalState NewState);
const constDMAvars_t dmaStreamConstSettings_sai[] =
{
  [BLOCK_A] = {
    SAI_BlockA_DMA2_StreamX, DMA_SAI_BlockA_Channel, RCC_AHB1Periph_DMA2, BlockA_IRQn,
    DMA_IT_TCIF_BlockA, DMA_IT_TEIF_BlockA, DMA_IT_FEIF_BlockA, DMA_IT_DMEIF_BlockA,
    DMA_FLAG_TCIF_BlockA, DMA_FLAG_TEIF_BlockA, DMA_FLAG_FEIF_BlockA, DMA_FLAG_DMEIF_BlockA,
    BlockA_IRQHandler,
  },
  [BLOCK_B] = {
    SAI_BlockB_DMA2_StreamX, DMA_SAI_BlockB_Channel, RCC_AHB1Periph_DMA2, BlockB_IRQn,
    DMA_IT_TCIF_BlockB, DMA_IT_TEIF_BlockB, DMA_IT_FEIF_BlockB, DMA_IT_DMEIF_BlockB,
    DMA_FLAG_TCIF_BlockB, DMA_FLAG_TEIF_BlockB, DMA_FLAG_FEIF_BlockB, DMA_FLAG_DMEIF_BlockB,
    BlockB_IRQHandler,
  },
};

const saiSettings_t tableOfSaiSettings[] =
{
  //[INDEX] = {
  //  SAI_DataSize_xb, SAI_xynchronous,
  //  SAI_MasterDivider_DisEna, MasterDivider,
  //  SAI_FrameLength, SAI_ActiveFrameLength, SAI_FSDefinition, SAI_FS_Position,
  //  SAI_SlotSize_xb, slotsNum, slotsActive;
  //},
  [SET_I2S] = {
    SAI_DataSize_16b, SAI_Asynchronous,
    SAI_MasterDivider_Disabled, 0,
    2 * 32, 32, I2S_FS_ChannelIdentification, SAI_FS_BeforeFirstBit,
    SAI_SlotSize_32b, 2, SAI_SlotActive_0 | SAI_SlotActive_1,
  },
  [SET_DSP] = {
    SAI_DataSize_16b, SAI_Asynchronous,
    SAI_MasterDivider_Enabled, 0,
    4 * 16, 1, SAI_FS_StartFrame, SAI_FS_FirstBit,
    SAI_SlotSize_16b, 4, SAI_SlotActive_0 | SAI_SlotActive_1 | SAI_SlotActive_2 | SAI_SlotActive_3,
  },
};

// SAI_PLL
// I2S_PLL
// EXTERNAL
const uint32_t saiClkTable[2][3] =
{
  [BLOCK_A] = { [SAI_PLL] = RCC_SAIACLKSource_PLLSAI, [I2S_PLL] = RCC_SAIACLKSource_PLLI2S, [EXTERNAL] = RCC_SAIACLKSource_Ext },
  [BLOCK_B] = { [SAI_PLL] = RCC_SAIBCLKSource_PLLSAI, [I2S_PLL] = RCC_SAIBCLKSource_PLLI2S, [EXTERNAL] = RCC_SAIBCLKSource_Ext }
};

const constSaiDefs_t saiDefs[] =
{
  [BLOCK_A] = { RCC_SAIBlockACLKConfig },
  [BLOCK_B] = { RCC_SAIBlockBCLKConfig }
};


const uint32_t saiPllTable[8][3] =
{
  [_44_kHz] = {PLLN_44K, PLLQ_44K, PLLR_44K,  },
  [_48_kHz] = {PLLN_48K, PLLQ_48K, PLLR_48K,  },
  [_88_kHz] = {PLLN_88K, PLLQ_88K, PLLR_88K,  },
  [_96_kHz] = {PLLN_96K, PLLQ_96K, PLLR_96K,  },
};

internalDmaStruct_t internalDmaVars[] =
{
  [BLOCK_A] = {&outQueueSAI1, NULL, &(dmaStreamConstSettings_sai[BLOCK_A]), SAI1_Block_A },
  [BLOCK_B] = {&outQueueSAI2, NULL, &(dmaStreamConstSettings_sai[BLOCK_B]), SAI1_Block_B },
};

static int clkSrcFreq[] =
{
  [SAI_PLL]  = 44100,
  [I2S_PLL]  = 44100,
  [EXTERNAL] = 44100,
};

// changalble dma settings:
// typedef struct
// {
//   uint32_t DMA_PeripheralDataSize_HalfWord;
//   uint32_t DMA_MemoryDataSize_HalfWord;
// } varDMAsettings_t;


//dry DMA settings: no buffor or enable settings:
static void config_DMA(SAI_Block_TypeDef *SAI_Block_x, const constDMAvars_t *dmaSettings)
{
  DMA_InitTypeDef DMA_InitStructure;

  RCC_AHB1PeriphClockCmd(dmaSettings->RCC_AHBPeriph_DMAx , ENABLE);
  /* DMA2 Stream0 SAI_BlockB configuration **************************************/
  DMA_InitStructure.DMA_Channel = dmaSettings->DMA_Channel_x;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (SAI_Block_x->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = 0;     //will be set in separate function
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_BufferSize = 0xFFF;      //will be set in separate function
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; //also double buffered
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(dmaSettings->DMAy_Streamx, &DMA_InitStructure);

  //Enable interrupts:
  //#warning Use interrupt for error handling and buffer switching
  DMA_ITConfig(dmaSettings->DMAy_Streamx, DMA_IT_TC, ENABLE);
  DMA_ITConfig(dmaSettings->DMAy_Streamx, DMA_IT_TE, ENABLE);

  registerIRQ( dmaSettings->DMAx_Streamx_IRQn , dmaSettings->isrHandler);
  NVIC_SetPriority(dmaSettings->DMAx_Streamx_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);
  NVIC_EnableIRQ(dmaSettings->DMAx_Streamx_IRQn);
}

static saiBlocks_t getSaiBlockNum(SAI_Block_TypeDef *SAI_Block_x)
{
  saiBlocks_t ret = BLOCK_A;
  if (SAI_Block_x == SAI1_Block_A)
    {
      ret = BLOCK_A;
    }
  else if (SAI_Block_x == SAI1_Block_B)
    {
      ret = BLOCK_B;
    }
  return ret;
}

static uint32_t setupSai( SAI_Block_TypeDef *SAI_Block_x, const saiSettings_t *i2sSettings)
{

  //SAI_DeInit(NULL);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SAI1, ENABLE);

  SAI_InitTypeDef SAI_InitStruct;
  SAI_StructInit(&SAI_InitStruct);
  //General block peripherial config
  SAI_InitStruct.SAI_AudioMode = SAI_Mode_MasterTx;
  SAI_InitStruct.SAI_Protocol = SAI_Free_Protocol;
  SAI_InitStruct.SAI_DataSize = i2sSettings->SAI_DataSize_xb;
  SAI_InitStruct.SAI_FirstBit = SAI_FirstBit_MSB;
  SAI_InitStruct.SAI_ClockStrobing = SAI_ClockStrobing_RisingEdge;
  SAI_InitStruct.SAI_Synchro = i2sSettings->SAI_xynchronous;
  SAI_InitStruct.SAI_OUTDRIV = SAI_OutputDrive_Enabled;
  SAI_InitStruct.SAI_NoDivider = i2sSettings->SAI_MasterDivider_DisEna;
  SAI_InitStruct.SAI_MasterDivider = i2sSettings->MasterDivider;
  SAI_InitStruct.SAI_FIFOThreshold = SAI_FIFOThreshold_HalfFull; //SAI_FIFOThreshold_HalfFull;

  SAI_Init(SAI_Block_x, &SAI_InitStruct);

  // Program the Frame Length, Frame active Length, FS Definition, FS Polarity,
  //               FS Offset using SAI_FrameInit() function.

  SAI_FrameInitTypeDef SAI_FrameInitStruct;

  SAI_FrameInitStruct.SAI_FrameLength = i2sSettings->SAI_FrameLength;    //number of all clock pulses
  SAI_FrameInitStruct.SAI_ActiveFrameLength = i2sSettings->SAI_ActiveFrameLength;   // FS pulse length
  SAI_FrameInitStruct.SAI_FSDefinition = i2sSettings->SAI_FSDefinition;
  SAI_FrameInitStruct.SAI_FSPolarity = SAI_FS_ActiveHigh;
  SAI_FrameInitStruct.SAI_FSOffset = i2sSettings->SAI_FS_Position;//SAI_FS_BeforeFirstBit; //SAI_FS_FirstBit;

  SAI_FrameInit(SAI_Block_x, &SAI_FrameInitStruct);

  // Program the Slot First Bit Offset, Slot Size, Slot Number, Slot Active
  //               using SAI_SlotInit() function.

  SAI_SlotInitTypeDef SAI_SlotInitStruct;
  SAI_SlotInitStruct.SAI_FirstBitOffset = 0;
  SAI_SlotInitStruct.SAI_SlotSize = i2sSettings->SAI_SlotSize_xb;
  SAI_SlotInitStruct.SAI_SlotNumber = i2sSettings->slotsNum;
  SAI_SlotInitStruct.SAI_SlotActive = i2sSettings->slotsActive;
  SAI_SlotInit(SAI_Block_x, &SAI_SlotInitStruct);


  SAI_ITConfig(SAI_Block_x, SAI_IT_OVRUDR, ENABLE);
  SAI_ITConfig(SAI_Block_x, SAI_IT_AFSDET, ENABLE);  // specifies the interrupt source for anticipated frame synchronization
  SAI_ITConfig(SAI_Block_x, SAI_IT_LFSDET, ENABLE);   //to indicate if there is the detection of a audio frame synchronisation (FS) later than expected
  SAI_ITConfig(SAI_Block_x, SAI_IT_WCKCFG, ENABLE);   // wrong clock/frame config
  //SAI_ITConfig(SAI_Block_x, SAI_IT_MUTEDET, ENABLE); //mute
  //SAI_ITConfig(SAI_Block_x, SAI_IT_FREQ , ENABLE);   //fifo request

  uint32_t status = SAI_Block_x->SR;
  (void)status;
  SAI_FlushFIFO(SAI_Block_x);

  registerIRQ(SAI1_IRQn, SAI_IRQHandler);
  NVIC_SetPriority(SAI1_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);
  NVIC_EnableIRQ(SAI1_IRQn);

  SAI_DMACmd(SAI_Block_x, ENABLE);

  return 1;
}


static void SAI_IRQHandler(void)
{

  uint32_t status = SAI1_Block_A->SR;
  static uint32_t cnt = 0;
  if ( status & SAI_IT_FREQ)
    {
      SAI1_Block_A->DR = cnt++;
    }
  if ( status & SAI_IT_OVRUDR)
    {
      SAI_FlushFIFO(SAI1_Block_A);
      //clear flag:
      SAI1_Block_A->CLRFR = SAI_FLAG_OVRUDR;


      SAI1_Block_A->DR = 0x00;
    }
  if (status)
    {
      taskENTER_CRITICAL();
      int s = 1;
      while (s);
    }
  massert(0);
}

static uint8_t SAIBuf[2][16];
void printSAIBufs(int *bufSize, char *bufPtr)
{
  int i,j;
  DPRINTF(CLEAR_LINE"SAI buffers   : ");
  for(j = 0; j < 2; j++)
    {
      for(i = 0; i < sizeof(SAIBuf[0]); i++)
        {
          if((i%8) == 0 && i > 0) DPRINTF(" | ");
          DPRINTF("%02x", SAIBuf[j][i]);
        }
      DPRINTF("          ");
    }
  DPRINTF(CLEAR_LINE"\nSAI interfaces: ");
  for(j = 0; j < 2; j++)
    {
      for(i = 0; i < sizeof(SAIBuf[0]); i++)
        {
          DPRINTF("%02x", SAIBuf[j][(i&1)?i&0xfe:i|1]);
          if((i%2) == 1)
            {
              DPRINTF(" ");
            }
          if((i%8) == 7)
            {
              DPRINTF(" ");
            }
        }
      DPRINTF("   ");
    }
  DPRINTF(CLEAR_LINE"\n");
}


static void audioDmaGetNextBuf(saiBlocks_t block)
{
  internalDmaStruct_t *dmaVars = &(internalDmaVars[block]);
  uint32_t currMem = DMA_GetCurrentMemoryTarget(dmaVars->hwDma->DMAy_Streamx);
  bBuffer_t *oldBuffer;
  uint32_t DMA_Memory;
  if (currMem)
    {
      oldBuffer = bBufferFromData((uint8_t *)dmaVars->hwDma->DMAy_Streamx->M0AR);
      DMA_Memory = DMA_Memory_0;
    }
  else
    {
      oldBuffer = bBufferFromData((uint8_t *)dmaVars->hwDma->DMAy_Streamx->M1AR);
      DMA_Memory = DMA_Memory_1;
    }
  bFree(oldBuffer);
  bBuffer_t *newBuffer = bDequeue(dmaVars->queue);
  if (!newBuffer)
    {
      //No data. Transmit silence
      newBuffer = dmaVars->silenceBuf;
      bRef(dmaVars->silenceBuf);
    }
  memcpy(SAIBuf[block], bDataFromBuffer(newBuffer), sizeof(SAIBuf[0]));
  DMA_MemoryTargetConfig(dmaVars->hwDma->DMAy_Streamx,
                         (uint32_t)bDataFromBuffer(newBuffer), DMA_Memory);
}

static void audioDmaIRQHandler(saiBlocks_t block)
{
  internalDmaStruct_t *dmaVars = &(internalDmaVars[block]);
  if ( DMA_GetITStatus(dmaVars->hwDma->DMAy_Streamx, dmaVars->hwDma->dmaItTCIF) )
    {
      audioDmaGetNextBuf(block);

      //clear interrupt:
      DMA_ClearFlag(dmaVars->hwDma->DMAy_Streamx, dmaVars->hwDma->dmaFlagTCIF);

    }
  if ( DMA_GetITStatus(dmaVars->hwDma->DMAy_Streamx, dmaVars->hwDma->dmaItTEIF) )
    {
      SAI_Cmd(dmaVars->SAI_Block_x_cooperation, DISABLE);
      SAI_FlushFIFO(dmaVars->SAI_Block_x_cooperation);
      massert(0);
    }
  if ( DMA_GetITStatus(dmaVars->hwDma->DMAy_Streamx, dmaVars->hwDma->dmaItFEIF) )
    {
      massert(0);
    }
}

static void BlockA_IRQHandler(void)
{
  audioDmaIRQHandler(BLOCK_A);
}

static void BlockB_IRQHandler(void)
{
  audioDmaIRQHandler(BLOCK_B);
}

static void setupAudioClocks(SAI_Block_TypeDef *SAI_Block_x, saiClkSource_t saiClkSrc, audioRate_t clkFreq, uint8_t div)
{
  uint32_t timeout = 100000;
  int cr1Div = 0;
  switch (saiClkSrc)
    {
    case EXTERNAL:
      configureCKINpin();
      switch(div)
        {
        case 1:
          cr1Div = 0;
          break;
        case 2:
          cr1Div = 1;
          break;
        case 4:
          cr1Div = 2;
          break;
        }
      break;
    case SAI_PLL:
      clkFreq = mapFreq(unmapFreq(clkFreq)/div);
      massert(clkFreq == _44_kHz || clkFreq == _48_kHz || clkFreq == _88_kHz || clkFreq == _96_kHz);
      RCC_PLLSAICmd(DISABLE);
      RCC_PLLSAIConfig(DECODE_N(cfg.SAIclk[clkFreq]), DECODE_Q(cfg.SAIclk[clkFreq]), 7);
      RCC_SAIPLLSAIClkDivConfig(DECODE_D(cfg.SAIclk[clkFreq]));
      RCC_PLLSAICmd(ENABLE);
      while ( RCC_GetFlagStatus(RCC_FLAG_PLLSAIRDY) == RESET && timeout-- );
      break;
    case I2S_PLL:
      clkFreq = mapFreq(unmapFreq(clkFreq)/div);
      massert(clkFreq == _44_kHz || clkFreq == _48_kHz || clkFreq == _88_kHz || clkFreq == _96_kHz);
      RCC_PLLI2SCmd(DISABLE);
      RCC_PLLI2SConfig(DECODE_N(cfg.SAIclk[clkFreq]), DECODE_Q(cfg.SAIclk[clkFreq]), 7);
      RCC_SAIPLLI2SClkDivConfig(DECODE_D(cfg.SAIclk[clkFreq]));
      RCC_PLLI2SCmd(ENABLE);
      while ( RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY) == RESET && timeout-- );

      break;

    }

  if (!timeout)
    {
      xprintf("%s timeout\n", __FUNCTION__);
      massert(0);
    }

  //Set clock divisor
  uint32_t cr1 = SAI_Block_x->CR1;
  cr1 &= ~(0xF<<20);
  cr1 |= cr1Div<<20;
  SAI_Block_x->CR1 = cr1;


#ifdef CLOCK_OUTPUT
  //Output clock on pin to check frequency:
  //RCC_MCO2Config(RCC_MCO2Source_PLLI2SCLK, RCC_MCO2Div_1 );
  RCC_MCO2Config(RCC_MCO2Source_PLLI2SCLK, RCC_MCO2Div_5 );

  /* GPIOD Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_MCO);
#endif
}

//external interface functions:
static int  audioOutSetupHw(SAI_Block_TypeDef *SAI_Block_x, saiClkSource_t clkSrc, audioRate_t clkFreq, uint8_t div, audioI2SFormat_t audioI2SFormat)
{
  saiBlocks_t currentBlock = getSaiBlockNum(SAI_Block_x);

  saiDefs[currentBlock].clkSrcSelectFunction( saiClkTable[currentBlock][clkSrc] );

  clkSrcFreq[clkSrc] = clkFreq;
  setupAudioClocks(SAI_Block_x, clkSrc, clkFreq, div);

  const saiSettings_t *i2sSettings;
  i2sSettings = &(tableOfSaiSettings[audioI2SFormat]);

  setupSai( SAI_Block_x, i2sSettings );

  const constDMAvars_t *dmaConstSettings = &(dmaStreamConstSettings_sai[currentBlock]) ;
  config_DMA(SAI_Block_x, dmaConstSettings);

  return 1;
}

static int audioPlay(SAI_Block_TypeDef *SAI_Block_x, uint32_t pingAddr, uint32_t pongAddr, uint32_t bufferSizeTransfers)
{

  saiBlocks_t currentBlock = getSaiBlockNum(SAI_Block_x) ;

  const constDMAvars_t *dmaConstSettings = &(dmaStreamConstSettings_sai[currentBlock]) ;

  /* Configure the buffers address and size */
  DMA_SetCurrDataCounter(dmaConstSettings->DMAy_Streamx, bufferSizeTransfers);
  DMA_MemoryTargetConfig(dmaConstSettings->DMAy_Streamx, pingAddr, DMA_Memory_0);
  DMA_DoubleBufferModeConfig(dmaConstSettings->DMAy_Streamx, pongAddr, 0 );    // zero stands for first used buffer
  DMA_DoubleBufferModeCmd(dmaConstSettings->DMAy_Streamx, ENABLE);
  DMA_Cmd( dmaConstSettings->DMAy_Streamx, ENABLE);

  audioPause(SAI_Block_x,  ENABLE); // start SAI transmission

  return 1;
}

static audioRate_t mapFreq(int freq)
{
  switch (freq)
    {
    case  32000:
      return _32_kHz;
    case  40000:
      return _40_kHz;
    case  44100:
      return _44_kHz;
    case  48000:
      return _48_kHz;
    case  88200:
      return _88_kHz;
    case  96000:
      return _96_kHz;
    case 176400:
      return _176_kHz;
    case 192000:
      return _192_kHz;
    }
  return _EXTERNAL;
}
static int unmapFreq(audioRate_t rate)
{
  switch (rate)
    {
    case _32_kHz:
      return 32000;
    case _40_kHz:
      return 40000;
    case _44_kHz:
      return 44100;
    case _48_kHz:
      return 48000;
    case _88_kHz:
      return 88200;
    case _96_kHz:
      return 96000;
    case _176_kHz:
      return 176400;
    case _192_kHz:
      return 192000;
    default:
      return 0;
    }
  return 0;
}
typedef struct
{
  saiClkSource_t clkSrc;
  int            freq;
  uint8_t        div;
} saiClkCfg_t;
static saiClkCfg_t saiClkCfg[] =
{
  [BLOCK_A] = {SAI_PLL, 44100, 1},
  [BLOCK_B] = {SAI_PLL, 44100, 1},
};


void saiInit(void)
{
  internalDmaVars[BLOCK_A].silenceBuf = bAlloc(AUDIO_BUFFER_OUT_SIZE);
  internalDmaVars[BLOCK_A].silenceBuf->size = AUDIO_BUFFER_OUT_SIZE;
  //Prevent buffer from releasing as it may be used by 2 entities at a time
  bRef(internalDmaVars[BLOCK_A].silenceBuf);
  bRef(internalDmaVars[BLOCK_A].silenceBuf);
  memset(internalDmaVars[BLOCK_A].silenceBuf->data, 0, AUDIO_BUFFER_OUT_SIZE);
  internalDmaVars[BLOCK_B].silenceBuf = bAlloc(AUDIO_BUFFER_OUT_SIZE);
  internalDmaVars[BLOCK_B].silenceBuf->size = AUDIO_BUFFER_OUT_SIZE;
  //Prevent buffer from releasing as it may be used by 2 entities at a time
  bRef(internalDmaVars[BLOCK_B].silenceBuf);
  bRef(internalDmaVars[BLOCK_B].silenceBuf);
  memset(internalDmaVars[BLOCK_B].silenceBuf->data, 0, AUDIO_BUFFER_OUT_SIZE);

  audioOutSetupHw(SAI1_Block_A, saiClkCfg[BLOCK_A].clkSrc, mapFreq(saiClkCfg[BLOCK_A].freq), saiClkCfg[BLOCK_A].div, SET_DSP);
  audioOutSetupHw(SAI1_Block_B, saiClkCfg[BLOCK_B].clkSrc, mapFreq(saiClkCfg[BLOCK_B].freq), saiClkCfg[BLOCK_B].div, SET_DSP);
  audioPlay(SAI1_Block_A, (uint32_t)internalDmaVars[BLOCK_A].silenceBuf->data, (uint32_t)internalDmaVars[BLOCK_A].silenceBuf->data, AUDIO_BUFFER_OUT_SIZE / 2);
  audioPlay(SAI1_Block_B, (uint32_t)internalDmaVars[BLOCK_B].silenceBuf->data, (uint32_t)internalDmaVars[BLOCK_B].silenceBuf->data, AUDIO_BUFFER_OUT_SIZE / 2);
}

#define FOR_EACH_SAI(arg_action)					\
  {									\
    int idx;								\
    for(idx = 0; idx < 2; idx++)					\
      if((1<<idx)&(outMask))						\
	{								\
	  int block = idx?BLOCK_B:BLOCK_A;				\
	  SAI_Block_TypeDef *SAI1_Block_x = idx?SAI1_Block_B:SAI1_Block_A; \
	  arg_action;							\
	  (void)block;							\
	  (void)SAI1_Block_x;						\
	}								\
  }


int saiReconfigure(int outMask, saiClkSource_t clkSrc, int freq, uint8_t div)
{

  //Check if anything changed
  {
    int block = (outMask&1)?BLOCK_B:BLOCK_A;
    if (clkSrc == saiClkCfg[block].clkSrc && freq == saiClkCfg[block].freq && div == saiClkCfg[block].div)
      return 0;
  }

  //Disable SAI(s)
  FOR_EACH_SAI(
  {
    SAI_Cmd(SAI1_Block_x, DISABLE);
  });

  //Wait for SAI disabled
  FOR_EACH_SAI
  (
  {
    uint32_t timeoutCnt = 500;
    while (SAI_GetCmdStatus(SAI1_Block_x) != DISABLE && --timeoutCnt)vTaskDelay(1);

    if (timeoutCnt == 0) return -1;
  });

  clkSrcFreq[clkSrc] = freq;

  FOR_EACH_SAI
  (
  {
    setupAudioClocks(SAI1_Block_x, clkSrc, mapFreq(freq), div);
    if (mapFreq(freq) != _EXTERNAL)
      {
        saiDefs[block].clkSrcSelectFunction( saiClkTable[block][clkSrc] );
      }
    saiClkCfg[block].clkSrc = clkSrc;
    saiClkCfg[block].freq = freq;
    saiClkCfg[block].div = div;
  });

  FOR_EACH_SAI
  (
  {
    //release queues from obsolete data
    internalDmaStruct_t *dmaVars = &(internalDmaVars[block]);
    bBuffer_t *buffer;
    while((buffer = bDequeue(dmaVars->queue)))
      bFree(buffer);
    //And replace current with silence
    audioDmaGetNextBuf(block);
  });

  //Enable SAI(s)
  FOR_EACH_SAI
  (
  {
    SAI_Cmd(SAI1_Block_x, ENABLE);
  });

  return 0;
}

//ENABLE or DISABLE
static int audioPause(SAI_Block_TypeDef *SAI_Block_x, FunctionalState NewState)
{

  if (NewState != ENABLE)
    {
      SAI_Cmd(SAI_Block_x, DISABLE);
    }
  else
    {
      SAI_Cmd(SAI_Block_x, ENABLE);
    }

  return 1;
}

/* static int audioStop(SAI_Block_TypeDef *SAI_Block_x) */
/* { */
/*   //not yet implemented: */
/*   massert(0); */

/*   // Procedure to disable audio channel goes here */
/*   SAI_Cmd(SAI_Block_x, DISABLE); */
/*   //while(sai_not_disabled); */
/*   //flush FIFO, */
/*   //itp.... */

/*   // stop DMA */

/*   return 1; */
/* } */
