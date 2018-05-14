void adcSens(int enable);
#define ADC_HANDLER(arg_dma, arg_rxStream, arg_semaphore)               \
  static void arg_dma##_Stream##arg_rxStream##_IRQHandler(void)         \
  {                                                                     \
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;                   \
    static int newSens = -1;                                            \
    if(newSens == -1 && sensADCnew != sensADC)                          \
      {                                                                 \
        newSens = sensADCnew;                                           \
        adcSens(newSens);                                               \
      }                                                                 \
    else if(newSens != -1)                                              \
      {                                                                 \
        sensADC = newSens;                                              \
        newSens = -1;                                                   \
      }                                                                 \
    if(DMA_GetITStatus(arg_dma##_Stream##arg_rxStream, DMA_IT_TCIF##arg_rxStream))         \
      {                                 \
        DMA_ClearITPendingBit(arg_dma##_Stream##arg_rxStream, DMA_IT_TCIF##arg_rxStream);      \
        bufferNum = 1;                          \
        xSemaphoreGiveFromISR(arg_semaphore, &xHigherPriorityTaskWoken ); \
      }                                 \
    else if(DMA_GetITStatus(arg_dma##_Stream##arg_rxStream, DMA_IT_HTIF##arg_rxStream))        \
      {                                 \
        DMA_ClearITPendingBit(arg_dma##_Stream##arg_rxStream, DMA_IT_HTIF##arg_rxStream);      \
        bufferNum = 0;                          \
        xSemaphoreGiveFromISR(arg_semaphore, &xHigherPriorityTaskWoken ); \
      }                                 \
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );         \
  }

#define ADC_INIT_HANDLER(arg_dma, arg_rxStream)            \
  {                                 \
    registerIRQ(arg_dma##_Stream##arg_rxStream##_IRQn, arg_dma##_Stream##arg_rxStream##_IRQHandler); \
    NVIC_SetPriority(arg_dma##_Stream##arg_rxStream##_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);    \
    NVIC_EnableIRQ(arg_dma##_Stream##arg_rxStream##_IRQn);        \
  }


#define SPI_DMA_HANDLER(arg_dma, arg_rxStream, arg_spiDataNum)          \
  static void arg_dma##_Stream##arg_rxStream##_IRQHandler(void)             \
  {                                 \
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;           \
    if(DMA_GetITStatus(arg_dma##_Stream##arg_rxStream, DMA_IT_TCIF##arg_rxStream))  \
      {\
        DMA_ClearITPendingBit(arg_dma##_Stream##arg_rxStream, DMA_IT_TCIF##arg_rxStream); \
        xSemaphoreGiveFromISR( spiData[arg_spiDataNum].xSemaphore, &xHigherPriorityTaskWoken ); \
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );         \
      }                                 \
  }
#define SPI_INIT_DMA_HANDLER(arg_dma, arg_rxStream)             \
  {                                 \
    registerIRQ(arg_dma##_Stream##arg_rxStream##_IRQn, arg_dma##_Stream##arg_rxStream##_IRQHandler);    \
    NVIC_SetPriority(arg_dma##_Stream##arg_rxStream##_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);          \
    NVIC_EnableIRQ(arg_dma##_Stream##arg_rxStream##_IRQn);              \
  }

#define SPI_HANDLER(arg_spi, arg_spiDataNum)         \
  static void arg_spi##_IRQHandler(void)             \
  {                                 \
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;   \
    if(SPI_I2S_GetITStatus(arg_spi, SPI_I2S_IT_TXE))                    \
      {                                                                 \
        SPI_I2S_ITConfig(arg_spi, SPI_I2S_IT_TXE, DISABLE);             \
        xSemaphoreGiveFromISR(spiData[arg_spiDataNum].xSemaphore, &xHigherPriorityTaskWoken ); \
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );                 \
      }                                                                 \
  }

#define SPI_INIT_HANDLER(arg_spi)                                       \
  {                                                                     \
    if(registerIRQ(arg_spi##_IRQn, arg_spi##_IRQHandler) < 0)while(1);  \
    NVIC_SetPriority(arg_spi##_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY); \
    NVIC_EnableIRQ(arg_spi##_IRQn);                                     \
  }

void spiIRQEnable(int enable);


#define SPI_INIT_MODULE(arg_module,                                     \
                        arg_spi, arg_apb,                               \
                        arg_gpio_clk, arg_clk,				\
			arg_gpio_miso, arg_miso,			\
			arg_gpio_mosi, arg_mosi,			\
                        arg_dataSize, arg_prescaler)                    \
  {                                                                     \
    arg_module.spi = arg_spi;                                           \
    arg_module.miso.gpio = GPIO##arg_gpio_miso;				\
    arg_module.miso.pin = GPIO_Pin_##arg_miso;                          \
    arg_module.mosi.gpio = GPIO##arg_gpio_mosi;				\
    arg_module.mosi.pin = GPIO_Pin_##arg_mosi;                          \
    RCC_APB##arg_apb##PeriphClockCmd(RCC_APB##arg_apb##Periph_##arg_spi, ENABLE); \
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio_clk, ENABLE);	\
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio_miso, ENABLE);	\
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio_mosi, ENABLE);	\
    GPIO_PinAFConfig(GPIO##arg_gpio_miso, GPIO_PinSource##arg_miso, GPIO_AF_##arg_spi); \
    GPIO_PinAFConfig(GPIO##arg_gpio_mosi, GPIO_PinSource##arg_mosi, GPIO_AF_##arg_spi); \
    GPIO_PinAFConfig(GPIO##arg_gpio_clk, GPIO_PinSource##arg_clk, GPIO_AF_##arg_spi); \
    GPIO_InitTypeDef  GPIO_InitStructure;                               \
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                   \
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;                      \
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;                        \
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;                     \
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_##arg_clk;			\
    GPIO_Init(GPIO##arg_gpio_clk, &GPIO_InitStructure);			\
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_miso ;			\
    GPIO_Init(GPIO##arg_gpio_miso, &GPIO_InitStructure);		\
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_mosi;			\
    GPIO_Init(GPIO##arg_gpio_mosi, &GPIO_InitStructure);		\
    SPI_I2S_DeInit(arg_spi);                                            \
    SPI_InitTypeDef  SPI_InitStructure;                                 \
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  \
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_##arg_dataSize##b;    \
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;                          \
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;                        \
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                           \
    SPI_InitStructure.SPI_BaudRatePrescaler =  SPI_BaudRatePrescaler_##arg_prescaler; \
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                  \
    SPI_InitStructure.SPI_CRCPolynomial = 7;                            \
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                       \
    SPI_Init(arg_spi, &SPI_InitStructure);                              \
    vSemaphoreCreateBinary(spiData[spiToSpiNum(arg_spi)].xSemaphore);   \
    massert(spiData[spiToSpiNum(arg_spi)].xSemaphore != NULL);          \
    massert(xSemaphoreTake(spiData[spiToSpiNum(arg_spi)].xSemaphore, portMAX_DELAY ) == pdTRUE ); \
    SPI_Cmd(arg_spi, ENABLE);                                           \
  }

#define SPI_INIT_DMA_MODULE(arg_module,                        \
                            arg_spi, arg_apb,                  \
                            arg_gpio, arg_clk, arg_miso, arg_mosi,      \
                            arg_dataSize, arg_prescaler,                \
                            arg_dma)                                    \
  {                                                                     \
  arg_module.spi = arg_spi;                                             \
  arg_module.miso.gpio = GPIO##arg_gpio;                                \
  arg_module.miso.pin = GPIO_Pin_##arg_miso;                            \
  arg_module.mosi.gpio = GPIO##arg_gpio;                                \
  arg_module.mosi.pin = GPIO_Pin_##arg_mosi;                            \
  RCC_APB##arg_apb##PeriphClockCmd(RCC_APB##arg_apb##Periph_##arg_spi, ENABLE);\
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio, ENABLE);\
  GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_miso, GPIO_AF_##arg_spi);\
  GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_mosi, GPIO_AF_##arg_spi);\
  GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_clk, GPIO_AF_##arg_spi);\
  GPIO_InitTypeDef  GPIO_InitStructure;\
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;\
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;\
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;\
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_miso | GPIO_Pin_##arg_mosi | GPIO_Pin_##arg_clk;\
  GPIO_Init(GPIO##arg_gpio, &GPIO_InitStructure);\
  SPI_I2S_DeInit(arg_spi);\
  SPI_InitTypeDef  SPI_InitStructure;\
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;\
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_##arg_dataSize##b;\
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;\
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;\
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;\
  SPI_InitStructure.SPI_BaudRatePrescaler = arg_prescaler;\
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;\
  SPI_InitStructure.SPI_CRCPolynomial = 7;\
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;\
  SPI_Init(arg_spi, &SPI_InitStructure);\
  SPI_I2S_DMACmd(arg_spi, SPI_I2S_DMAReq_Tx, ENABLE);   \
  SPI_I2S_DMACmd(arg_spi, SPI_I2S_DMAReq_Rx, ENABLE);   \
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_##arg_dma, ENABLE); \
  vSemaphoreCreateBinary(spiData[spiToSpiNum(arg_spi)].xSemaphore);\
  massert(spiData[spiToSpiNum(arg_spi)].xSemaphore != NULL);                \
  massert(xSemaphoreTake(spiData[spiToSpiNum(arg_spi)].xSemaphore, portMAX_DELAY ) == pdTRUE ); \
}

#define SPI_CS_INIT_MODULE(arg_module, arg_gpio, arg_cs) \
  {\
    arg_module.cs.gpio = GPIO##arg_gpio;                \
    arg_module.cs.pin = GPIO_Pin_##arg_cs;       \
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio, ENABLE);\
    GPIO_InitTypeDef  GPIO_InitStructure;\
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;\
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;\
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;\
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_cs;\
    GPIO_Init(GPIO##arg_gpio, &GPIO_InitStructure);\
    GPIO_WriteBit(GPIO##arg_gpio, GPIO_Pin_##arg_cs, 1);\
  }

#define SPI_INIT(arg_spi, arg_apb,  \
                 arg_gpio, arg_clk, arg_miso, arg_mosi,   \
                 arg_dataSize, arg_prescaler  \
                 /*arg_dma, arg_tx_flag, arg_rx_flag)*/ \
                )   \
{\
  RCC_APB##arg_apb##PeriphClockCmd(RCC_APB##arg_apb##Periph_##arg_spi, ENABLE);\
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio, ENABLE);\
  GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_miso, GPIO_AF_##arg_spi);\
  GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_mosi, GPIO_AF_##arg_spi);\
  GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_clk, GPIO_AF_##arg_spi);\
  GPIO_InitTypeDef  GPIO_InitStructure;\
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;\
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;\
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;\
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_miso | GPIO_Pin_##arg_mosi | GPIO_Pin_##arg_clk;\
  GPIO_Init(GPIO##arg_gpio, &GPIO_InitStructure);\
  SPI_I2S_DeInit(arg_spi);\
  SPI_InitTypeDef  SPI_InitStructure;\
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;\
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_##arg_dataSize##b;\
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;\
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;\
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;\
  SPI_InitStructure.SPI_BaudRatePrescaler =  SPI_BaudRatePrescaler_##arg_prescaler;\
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;\
  SPI_InitStructure.SPI_CRCPolynomial = 7;\
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;\
  SPI_Init(arg_spi, &SPI_InitStructure);\
}

#define   \
  SPI_INIT_AUDIO_STREAM(arg_spi, arg_apb,  \
                        arg_gpio, arg_ssl, arg_clk, arg_miso, arg_mosi,   \
                        arg_mode, arg_dataSize, arg_prescaler, arg_cpha \
                        /*arg_dma, arg_tx_flag, arg_rx_flag)*/ \
                       )   \
  {\
    RCC_APB##arg_apb##PeriphClockCmd(RCC_APB##arg_apb##Periph_##arg_spi, ENABLE);\
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio, ENABLE);\
    GPIO_InitTypeDef  GPIO_InitStructure;\
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;\
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;\
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;\
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_miso | GPIO_Pin_##arg_mosi | GPIO_Pin_##arg_clk | GPIO_Pin_##arg_ssl;\
    /* GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;\*/  \
    /* GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_ssl;*/ \
    GPIO_Init(GPIO##arg_gpio, &GPIO_InitStructure);\
    GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_miso, GPIO_AF_##arg_spi);\
    GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_mosi, GPIO_AF_##arg_spi);\
    GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_clk, GPIO_AF_##arg_spi);\
    GPIO_PinAFConfig(GPIO##arg_gpio, GPIO_PinSource##arg_ssl, GPIO_AF_##arg_spi);\
    SPI_I2S_DeInit(arg_spi);\
    SPI_InitTypeDef  SPI_InitStructure;\
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_RxOnly;\
    SPI_InitStructure.SPI_DataSize = ((arg_dataSize)==2)?SPI_DataSize_16b:SPI_DataSize_8b; \
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;\
    SPI_InitStructure.SPI_CPHA = arg_cpha;\
    SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;\
    SPI_InitStructure.SPI_BaudRatePrescaler =  SPI_BaudRatePrescaler_##arg_prescaler;\
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;\
    SPI_InitStructure.SPI_CRCPolynomial = 7;\
    SPI_InitStructure.SPI_Mode = SPI_Mode_##arg_mode;\
    SPI_Init(arg_spi, &SPI_InitStructure);\
    SPI_Cmd (arg_spi, ENABLE); \
  }

#define DMA_INIT_SPI_AUDIO_STREAM(arg_spi,        \
                                  arg_dma, arg_stream, arg_channel, \
				  arg_ISR_Handler,		    \
				  arg_dataSize			    \
                                 )         \
{                 \
  DMA_InitTypeDef DMA_InitStructure;          \
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_##arg_dma, ENABLE);    \
  DMA_DeInit (arg_dma##_Stream##arg_stream);        \
  while (DMA_GetCmdStatus(arg_dma##_Stream##arg_stream) != DISABLE);  \
  DMA_StructInit (&DMA_InitStructure);          \
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (arg_spi->DR); \
  DMA_InitStructure.DMA_Channel = _P2(DMA_Channel_,arg_channel);  \
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;   \
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;   \
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)NULL;   \
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word; \
  DMA_InitStructure.DMA_PeripheralDataSize = ((arg_dataSize) == 4)?DMA_PeripheralDataSize_Word:((arg_dataSize) == 2)?DMA_PeripheralDataSize_HalfWord:DMA_PeripheralDataSize_Byte; \
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;     \
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;     \
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;    \
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull; \
  DMA_InitStructure.DMA_BufferSize = (uint32_t)( 1 ) ;      \
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;   \
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; \
  DMA_Init (arg_dma##_Stream##arg_stream, &DMA_InitStructure);    \
  DMA_DoubleBufferModeCmd(arg_dma##_Stream##arg_stream, ENABLE);      \
  DMA_ITConfig (arg_dma##_Stream##arg_stream, DMA_IT_TC, ENABLE); \
  registerIRQ(arg_dma##_Stream##arg_stream##_IRQn, arg_ISR_Handler);  \
  NVIC_SetPriority(arg_dma##_Stream##arg_stream##_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY); \
  NVIC_EnableIRQ(arg_dma##_Stream##arg_stream##_IRQn);      \
  SPI_I2S_DMACmd (arg_spi, SPI_I2S_DMAReq_Rx, ENABLE);      \
  DMA_Cmd (arg_dma##_Stream##arg_stream, DISABLE);      \
}

#define DMA_INIT_SPI_AUDIO_STREAM_TEST(arg_spi, \
                                       arg_dmax_streamx, arg_dmax, arg_stream, arg_channel,  \
                                       arg_ISR_Handler \
                                      ) \
{ \
  DMA_InitTypeDef DMA_InitStructure;\
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_##arg_dmax, ENABLE);  \
  DMA_DeInit ( _P1(arg_dmax_streamx) );      \
  while (DMA_GetCmdStatus( _P1(arg_dmax_streamx) ) != DISABLE);     \
  DMA_StructInit (&DMA_InitStructure);      \
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (arg_spi->DR);      \
  DMA_InitStructure.DMA_Channel = _P2(DMA_Channel_, arg_channel);      \
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;     \
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;     \
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)NULL;     \
  DMA_InitStructure.DMA_BufferSize = (uint32_t)( 10 ) ;      \
  DMA_Init ( _P1(arg_dmax_streamx) , &DMA_InitStructure);      \
  DMA_ITConfig (_P1(arg_dmax_streamx), DMA_IT_TC, ENABLE);     \
  registerIRQ( arg_dmax_streamx##_IRQn), arg_ISR_Handler);      \
  /* NVIC_SetPriority(arg_dmax##_Stream##arg_stream##_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);  */    \
  NVIC_EnableIRQ(arg_dmax_streamx##_IRQn);      \
  SPI_I2S_DMACmd (arg_spi, SPI_I2S_DMAReq_Rx, ENABLE);      \
}
/*
#define DMA_INIT_SPI_AUDIO_STREAM_TEST(arg_spi, \
                                  arg_dma_x, arg_dma_plus_stream, arg_channel  \
                                 ) \
{ \
  DMA_InitTypeDef DMA_InitStructure;\
  RCC_AHB1PeriphClockCmd( _P2( RCC_AHB1Periph_, DMA2 ) , ENABLE);  \
  DMA_DeInit ( _P1(arg_dma_plus_stream) );      \
  while (DMA_GetCmdStatus( _P1( arg_dma_plus_stream ) ) != DISABLE);     \
  DMA_StructInit (&DMA_InitStructure);      \
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (_P1(arg_spi)->DR);      \
  DMA_InitStructure.DMA_Channel = _P2(DMA_Channel_, arg_channel);      \
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;     \
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;     \
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)NULL;     \
  DMA_InitStructure.DMA_BufferSize = (uint32_t)( 10 ) ;      \
  DMA_Init (_P1(arg_dma_plus_stream), &DMA_InitStructure);      \
  DMA_ITConfig (_P1(arg_dma_plus_stream), DMA_IT_TC, ENABLE);     \
  registerIRQ(_P2(arg_dma_plus_stream,_IRQn), DMA2_Stream2_IRQHandler);      \
  NVIC_SetPriority(_P1(arg_dma_plus_stream)##_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);    \
  NVIC_EnableIRQ(_P1(arg_dma_plus_stream)##_IRQn);      \
  SPI_I2S_DMACmd (_P1(arg_spi), SPI_I2S_DMAReq_Rx, ENABLE);      \
}
*/



#define SPI_CS_INIT(arg_gpio, arg_cs) \
  {\
    RCC_AHB1PeriphClockCmd( _P2(RCC_AHB1Periph_GPIO,arg_gpio), ENABLE);\
    GPIO_InitTypeDef  GPIO_InitStructure;\
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;\
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;\
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;\
    GPIO_InitStructure.GPIO_Pin = _P2(GPIO_Pin_,arg_cs);\
    GPIO_Init( _P2(GPIO,arg_gpio), &GPIO_InitStructure);\
    GPIO_WriteBit( _P2(GPIO,arg_gpio), _P2(GPIO_Pin_,arg_cs), 1);\
  }

#define GPIO_OUT_INIT(arg_gpio, arg_cs) \
  {\
    RCC_AHB1PeriphClockCmd( _P2(RCC_AHB1Periph_GPIO,arg_gpio), ENABLE);\
    GPIO_InitTypeDef  GPIO_InitStructure;\
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;\
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;\
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;\
    GPIO_InitStructure.GPIO_Pin = _P2(GPIO_Pin_,arg_cs);\
    GPIO_Init( _P2(GPIO,arg_gpio), &GPIO_InitStructure);\
    GPIO_WriteBit( _P2(GPIO,arg_gpio), _P2(GPIO_Pin_,arg_cs), 1);\
  }

#define PIN_SET(arg_gpio, arg_cs, arg_reSER) \
  {\
    GPIO_WriteBit( _P2(GPIO,arg_gpio), _P2(GPIO_Pin_,arg_cs), arg_reSER);\
  }


#define EXTI_INIT(arg_gpio, arg_pin, arg_isr) \
  {\
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio, ENABLE);\
    GPIO_InitTypeDef  GPIO_InitStructure;\
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;\
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;\
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;\
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_pin;\
    GPIO_Init(GPIO##arg_gpio, &GPIO_InitStructure);\
    \
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);\
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIO##arg_gpio, EXTI_PinSource##arg_pin);\
    \
    /* Configure EXTI Line1 */  \
    EXTI_InitTypeDef   EXTI_InitStructure;\
    EXTI_InitStructure.EXTI_Line = EXTI_Line##arg_exti_line;\
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;\
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;\
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;\
    EXTI_Init(&EXTI_InitStructure);\
    /* Enable and set EXTI Line1 Interrupt to the lowest priority */  \
    NVIC_InitTypeDef   NVIC_InitStructure;\
    if(arg_pin>9 && arg_pin < 16){ \
        NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn; \  /*EXTI##arg_exti_line##_IRQn; */  \
      }else if (arg_pin > 4 & arg_pin <10 ){  \
        NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn; \  /*EXTI##arg_exti_line##_IRQn; */  \
      } else {  \
        NVIC_InitStructure.NVIC_IRQChannel = EXTI##arg_pin##_IRQn; \  /*EXTI##arg_exti_line##_IRQn; */  \
      } \
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x03;\
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;\
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;\
    NVIC_Init(&NVIC_InitStructure);\
  }



//  ////////////////////////////////////////////////////////////////////////////
// /*for macro: check for 10-15 pin for one interrupt: EXTI15_10_IRQn
// GPIO_InitTypeDef   GPIO_InitStructure;
// /* Configure PC14 pin as input floating */
// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
// GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
// GPIO_Init(GPIOC, &GPIO_InitStructure);

// /* Enable GPIOC clock */
// RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

// /* Enable SYSCFG clock */
// RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

// /* Connect EXTI Line15 to PC14 pin */
// SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource14);
// /* Configure EXTI Line13 */
// EXTI_InitTypeDef   EXTI_InitStructure;
// EXTI_InitStructure.EXTI_Line = EXTI_Line14;
// EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
// EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
// EXTI_InitStructure.EXTI_LineCmd = ENABLE;
// EXTI_Init(&EXTI_InitStructure);

// /* Enable and set EXTI15_10 Interrupt to the lowest priority */
// NVIC_InitTypeDef    NVIC_InitStructure;
// NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
// NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
// NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
// NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

// NVIC_Init(&NVIC_InitStructure);
//  ///////////////////////////////////////////////////////////////////////////


#define SAI1_BLOCKx_GPIO_INIT(argGPIO, arg_MCLK, arg_FS, arg_SCK, arg_SD ) \
  { \
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##argGPIO, ENABLE);\
    \
    GPIO_InitTypeDef GPIO_InitStruct; \
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_##arg_MCLK | GPIO_Pin_##arg_FS | GPIO_Pin_##arg_SCK | GPIO_Pin_##arg_SD;  \
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF; \
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; \
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz; \
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL; \
    \
    GPIO_Init(GPIO##argGPIO, &GPIO_InitStruct); \
    \
    GPIO_PinAFConfig(GPIO##argGPIO, GPIO_PinSource##arg_MCLK, GPIO_AF_SAI1); \
    GPIO_PinAFConfig(GPIO##argGPIO, GPIO_PinSource##arg_FS, GPIO_AF_SAI1); \
    GPIO_PinAFConfig(GPIO##argGPIO, GPIO_PinSource##arg_SCK, GPIO_AF_SAI1); \
    GPIO_PinAFConfig(GPIO##argGPIO, GPIO_PinSource##arg_SD, GPIO_AF_SAI1); \
  }

typedef struct
{
  SPI_TypeDef *SPIx;
  DMA_Stream_TypeDef *txStream;
  DMA_Stream_TypeDef *rxStream;
  uint32_t txChannel;
  uint32_t rxChannel;
  uint32_t intMask;
  xSemaphoreHandle xSemaphore;
} spiData_t;
extern spiData_t spiData[];

#define USART_INIT(arg_uart, arg_apb, arg_gpio_tx, arg_tx, arg_gpio_rx, arg_rx, arg_speed) \
  {                                 \
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio_tx, ENABLE);  \
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio_rx, ENABLE);  \
    RCC_APB##arg_apb##PeriphClockCmd(RCC_APB##arg_apb##Periph_##arg_uart, ENABLE); \
    GPIO_PinAFConfig(GPIO##arg_gpio_tx, GPIO_PinSource##arg_tx, GPIO_AF_##arg_uart); \
    GPIO_PinAFConfig(GPIO##arg_gpio_rx, GPIO_PinSource##arg_rx, GPIO_AF_##arg_uart); \
    GPIO_InitTypeDef  GPIO_InitStructure;               \
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          \
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            \
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            \
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;           \
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_tx; \
    GPIO_Init(GPIO##arg_gpio_tx, &GPIO_InitStructure);         \
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_rx ; \
    GPIO_Init(GPIO##arg_gpio_rx, &GPIO_InitStructure);         \
    USART_InitTypeDef USART_InitStructure;              \
    USART_InitStructure.USART_BaudRate = arg_speed;         \
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     \
    USART_InitStructure.USART_StopBits = USART_StopBits_1;      \
    USART_InitStructure.USART_Parity = USART_Parity_No;         \
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; \
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; \
    USART_Init(arg_uart, &USART_InitStructure);             \
    USART_Cmd(arg_uart, ENABLE);                    \
  }

#define LEDS_INIT_ON_GPIO(arg_gpio)     \
  {                     \
    int led;                                    \
    GPIO_InitTypeDef  GPIO_InitStructure;           \
    GPIO_InitStructure.GPIO_Pin = 0;                            \
    for(led = 0; led < sizeof(leds)/sizeof(leds[0]); led++)             \
      if(leds[led].gpio == GPIO##arg_gpio) GPIO_InitStructure.GPIO_Pin |= leds[led].pin; \
    if(GPIO_InitStructure.GPIO_Pin)                                     \
      {                                                                 \
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;                   \
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;                  \
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;              \
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;                \
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio, ENABLE);  \
        GPIO_Init(GPIO##arg_gpio, &GPIO_InitStructure);                 \
      }                                                                 \
  }
#define LEDS_INIT()                     \
  LEDS_INIT_ON_GPIO(A);                 \
  LEDS_INIT_ON_GPIO(B);                 \
  LEDS_INIT_ON_GPIO(C);                 \
  LEDS_INIT_ON_GPIO(D);                 \
  LEDS_INIT_ON_GPIO(E);                 \
  LEDS_INIT_ON_GPIO(F);                 \
  LEDS_INIT_ON_GPIO(G);

int spiToSpiNum(void *spi);

void forcePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t state);

uint8_t readPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

void releasePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

void configureCKINpin(void);

#define GPIO_INIT(arg_gpio, arg_pin, arg_mode, arg_pull, arg_Type, arg_Speed)	\
  {                 \
    GPIO_InitTypeDef  GPIO_InitStructure;       \
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_##arg_mode;    \
    GPIO_InitStructure.GPIO_OType = GPIO_OType_##arg_Type;      \
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_##arg_Speed;      \
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_##arg_pull;      \
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_pin;     \
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_gpio, ENABLE);  \
    GPIO_Init(GPIO##arg_gpio, &GPIO_InitStructure);     \
  }





#define SETUP_EXTI(arg_GPIO, arg_Pin, arg_IRQn, arg_IRQ_Handler)\
  {\
    GPIO_InitTypeDef   GPIO_InitStructure;  \
    EXTI_InitTypeDef   EXTI_InitStructure;  \
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO##arg_GPIO, ENABLE);  \
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);  \
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;  \
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  \
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_##arg_Pin; \
    GPIO_Init(GPIO##arg_GPIO, &GPIO_InitStructure); \
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIO##arg_GPIO, EXTI_PinSource##arg_Pin);  \
    EXTI_InitStructure.EXTI_Line = EXTI_Line##arg_Pin;  \
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; \
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; \
    EXTI_InitStructure.EXTI_LineCmd = ENABLE; \
    EXTI_Init(&EXTI_InitStructure); \
    registerIRQ(arg_IRQn, arg_IRQ_Handler); \
    NVIC_SetPriority(arg_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY); \
    NVIC_EnableIRQ(arg_IRQn); \
  }

void txSPI(void *spi, uint8_t txByte);
uint8_t rxSPI(void *spi);




