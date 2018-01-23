#ifndef I2S_CONFIG_H_
#define I2S_CONFIG_H_

/* Mask for the bit EN of the I2S CFGR register */
#define I2S_ENABLE_MASK                 0x0400

void I2S_AudioInterfaceInit( SPI_TypeDef *SPIx );
void I2S_AudioPlay(SPI_TypeDef *SPIx, uint32_t Addr, uint32_t Addr2, uint32_t Size);
void I2S_AudioPause(SPI_TypeDef *SPIx);
void I2S_AudioStop(SPI_TypeDef *SPIx);
int I2S_AudioIsFER(SPI_TypeDef *SPIx);


//user Callbacks:
void AUDIO_TransferComplete_CallBack(uint32_t *replaceBufforAdr, uint32_t Size, signed portBASE_TYPE *xHigherPriorityTaskWoken);
void I2S_AUDIO_Error_CallBack(void *pData);
void i2sResync(void);
#endif

