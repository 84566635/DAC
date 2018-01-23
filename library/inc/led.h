int LEDblink(int LEDblinker, uint32_t ledsMask, int blinkNum, int time_on, int time_break, int time_pre, int time_post, uint8_t loop);

#define LED_BLINK_INFINITY -2
#define LED_BLINK_STOP     -1
int LEDset(int LEDblinker, uint32_t ledsMaskOn, uint32_t ledsMaskOff);
void LEDSset(uint32_t ledsMaskOn, uint32_t ledsMaskOff);
void LEDInit(void);
void LEDBreakLoop(int LEDblinker);

//Define the table externally
typedef struct
{
  GPIO_TypeDef *gpio;
  uint16_t pin;
  uint8_t invertPolarity;
} led_t;

extern led_t leds[];
void __attribute__((weak))LED_print(int LEDNum, int value);
