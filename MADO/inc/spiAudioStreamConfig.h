#ifndef SPI_AUDIO_STREAM_CONFIG_H_
#define SPI_AUDIO_STREAM_CONFIG_H_

//audioSpiStream:
/* #define OLINUXINO_RX_REQUEST_ENABLE()          _PIN_WRITE(OLINUXINO_ENABLE_GPIO, OLINUXINO_ENABLE_PIN, 0) */
/* #define OLINUXINO_RX_REQUEST_DISABLE()  _PIN_WRITE(OLINUXINO_ENABLE_GPIO, OLINUXINO_ENABLE_PIN, 1)  */

/* #define _PIN_WRITE(arg_gpio, arg_pin, arg_state)          GPIO_WriteBit( _P2(GPIO,arg_gpio), _P2(GPIO_Pin_, arg_pin), arg_state) */

void spiSetupDmaForStream(void);
void spiAudioStreamTC_Callback(uint32_t *replaceBufforAdr, uint32_t Size, signed portBASE_TYPE *xHigherPriorityTaskWoken);
#endif

