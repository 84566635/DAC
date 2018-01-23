
#define LED_BLINK_INFINITY -2
#define LED_BLINK_STOP     -1

void CC85xxControlTaskInit(void);
int LEDblink2(int LED_module, uint32_t ledsMask, int blinkNum, int time_on, int time_break, int time_pre, int time_post);


/////// general includes for project.h /////////////        Section to be moved
#define CC85XX_CONTROL_MODULES_NUM 1