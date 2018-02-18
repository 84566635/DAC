#ifndef _PROJECT_H_
#define _PROJECT_H_

#ifndef VERSION
#define VERSION "MA_undefined"
#endif

#define DEVMODE_BOSON 1
#define DEVMODE_DAC   2

#define STACK_SIZE 0x1000
#define SPI_CHECK_FREQ 20       //[ms]
#define TIMEOUT_DATA_MS  20
#define MSG_TIMEOUT_DATA  110
#define MSG_TIMEOUT_REPLY 120

#define SPK_QTY 2
#define MSG_LEN 32

#define RADIO_MODULES 2
#define MAX_DATA_SIZE MSG_LEN

#define  MSG_TYPES_QTY       26


#define HW_ADDRESS_SIZE 6
#define UART_SPEED 115200
#define UART_SPEED_DAC 256000
#define CONN_MODE_C 1           // konfig po stronie buforów CEN, 1  UART, 2  SPI
#define CONN_MODE_P 2           // konfig po stronie buforów PER, 1  UART, 2  SPI

#define DBUFSIZE 5000
#define LED_BLINKER_NUM 2
#define LED_BLINKER_SPI_HELPER 0
#define LED_BLINKER_AUDIO      1

#define SPI_CHUNK_SIZE 2

typedef struct
{
  volatile int fSize;// size of image to flash
  volatile int fCRC;// CRC of image to flash
#define C_SMOK_MASK     CFG_PATCH(CFG_IDX(smok_mask))
  volatile int smok_mask;//maska podlaczonych SMOKow
  volatile int empty_buffers;//1 uzywaj czystych buforow
  volatile int trace_messages; // 1 - wyswietlaj komunikaty bez 0x55, 0xb0, 0xCC, 2 - wyswietlaj tez 0x55, 0xb0, 0xCC
  volatile int ka_period;         // okres wysylania keep aliveow
  volatile int ka_timeout;        // czas alarmowania po nieotrzymaniu keepalive
  volatile int manId;
  volatile int kondPom;
  volatile int kondPomSPK[4];
  volatile int proto;//0x01 - dla plytki prototypowej, 0x02 - zostaw radio w resecie, 0x04 - USB jako test patterny, 0x08 - inwersja trybu kabel/radio, 0x10-USB z STM zamiast  Tenora, 0x20 - FILES - rozpocznij przechwytywanie aktualnego strumienia, 0x40 - usb ULPI, 0x80 - wlacz USB po 10s
  volatile int cpha;//
  volatile int stats;//
  volatile int dlevel;// traces level
  volatile int forcems;// force MS formats in SS mode
  volatile int pll_m;// kwarc (MHz)
  volatile int ack50;// 0 - ACK jest wysylane bez czekania na sharka/PWMa
  volatile int disable_alerts;    // 1 - wylacz reakcje na alerty
  volatile int unmuteIgnore; //Czas ciszy w ms po otrzymaniu potwierdzenia na 0x92 od SMOKow
  volatile int SAIclk[8];
#define C_TIMEOUT_50     CFG_PATCH(CFG_IDX(timeout50))
  volatile int timeout50; //Czas na odpowiedz na komunikat 50
#define DEVICE_MODE CFG_PATCH(CFG_IDX(centralDeviceMode))
  volatile int centralDeviceMode; /* possible values: BOSON=1 (and others remaining values as default), DAC=2 */
  volatile int fillup[128-32];    //Fillup to 128 elements
} __attribute__((packed)) config_t;

#define CONFIG_CONST                                            \
  const __attribute__((section(".nvs")))config_t cfgnvs =       \
  {                                                             \
    CFG_EVAL(fSize),                                            \
    CFG_EVAL(fCRC),                                             \
    CFG_EVAL(smok_mask),                                        \
    CFG_EVAL(empty_buffers),                                    \
    CFG_EVAL(trace_messages),                                   \
    CFG_EVAL(ka_period),                                        \
    CFG_EVAL(ka_timeout),                                       \
    CFG_EVAL(manId),                                            \
    CFG_EVAL(kondPom),                                          \
    CFG_EVAL(kondPomSPK[0]),                                    \
    CFG_EVAL(kondPomSPK[1]),                                    \
    CFG_EVAL(kondPomSPK[3]),                                    \
    CFG_EVAL(kondPomSPK[2]),                                    \
    CFG_EVAL(proto),                                            \
    CFG_EVAL(cpha),                                             \
    CFG_EVAL(stats),                                            \
    CFG_EVAL(dlevel),                                           \
    CFG_EVAL(forcems),                                          \
    CFG_EVAL(pll_m),                                            \
    CFG_EVAL(ack50),                                            \
    CFG_EVAL(disable_alerts),                                   \
    CFG_EVAL(unmuteIgnore),                                     \
    CFG_EVAL(SAIclk[0]),                                        \
    CFG_EVAL(SAIclk[1]),                                        \
    CFG_EVAL(SAIclk[2]),                                        \
    CFG_EVAL(SAIclk[3]),                                        \
    CFG_EVAL(SAIclk[4]),                                        \
    CFG_EVAL(SAIclk[5]),                                        \
    CFG_EVAL(SAIclk[6]),                                        \
    CFG_EVAL(SAIclk[7]),                                        \
    CFG_EVAL(timeout50),                                        \
    CFG_EVAL(centralDeviceMode),                                \
  }                                                             \
 
#define CONFIG_PRINT()                                          \
  PRINT_CFG(fSize);                                             \
  PRINT_CFG(fCRC);                                              \
  PRINT_CFG(smok_mask);                                         \
  PRINT_CFG(empty_buffers);                                     \
  PRINT_CFG(trace_messages);                                    \
  PRINT_CFG(ka_period);                                         \
  PRINT_CFG(ka_timeout);                                        \
  PRINT_CFG(manId);                                             \
  PRINT_CFG(kondPom);                                           \
  PRINT_CFG(kondPomSPK[0]);                                     \
  PRINT_CFG(kondPomSPK[1]);                                     \
  PRINT_CFG(kondPomSPK[2]);                                     \
  PRINT_CFG(kondPomSPK[3]);                                     \
  PRINT_CFG(proto);                                             \
  PRINT_CFG(cpha);                                              \
  PRINT_CFG(stats);                                             \
  PRINT_CFG(dlevel);                                            \
  PRINT_CFG(forcems);                                           \
  PRINT_CFG(pll_m);                                             \
  PRINT_CFG(ack50);                                             \
  PRINT_CFG(disable_alerts);                                    \
  PRINT_CFG(unmuteIgnore);                                      \
  PRINT_CFG(SAIclk[0]);                                         \
  PRINT_CFG(SAIclk[1]);                                         \
  PRINT_CFG(SAIclk[2]);                                         \
  PRINT_CFG(SAIclk[3]);                                         \
  PRINT_CFG(SAIclk[4]);                                         \
  PRINT_CFG(SAIclk[5]);                                         \
  PRINT_CFG(SAIclk[6]);                                         \
  PRINT_CFG(SAIclk[7]);                                         \
  PRINT_CFG(timeout50);                                         \
  PRINT_CFG(centralDeviceMode);								     \


#define LEDS_NUM        6

typedef enum
{
  LD1 = 0x01,
  LD2 = 0x02,
  LD3 = 0x04,
  LD4 = 0x08,
  LD5 = 0x10,
  LD6 = 0x20,
} ledsNum_t;



typedef enum
{
  CEN_PORT,
  PER1_PORT,
  PER2_PORT,
  PER3_PORT,
  PER4_PORT,
  PER5_PORT,
  PER6_PORT,
  PER7_PORT,
  PER8_PORT,
  PORTS_NUM,
  PORT_INVALID,
} portNum_t;
extern const char *portStr[PORTS_NUM];
extern const portNum_t portCompl[PORTS_NUM];


//audio block constants
#define SAI_Block_TX SAI1_Block_A
#define TRANSFER_SIZE_A 32      //buffer for DMA for audio transmission 

#define DEBUG_UART UART8

#define WATCHDOG_LED(...)       (1<<0)
#define WATCHDOG_HWCOMMSPI(...) (1<<1)
#define WATCHDOG_HWCOMM(...)    (1<<2)
#define WATCHDOG_COMM(...)      (1<<3)
#define WATCHDOG_DEBUG(...)     (1<<5)
#define WATCHDOG_EXPECTED                                       \
  (WATCHDOG_LED(0)|WATCHDOG_HWCOMMSPI(0)|WATCHDOG_HWCOMM(0)     \
   |WATCHDOG_COMM(0)|WATCHDOG_DEBUG(0))
#define WATCHDOG_TIMEOUT     20 //20s

#define TEST_PATTERNS_NUM 24
#define USE_MUTE 1
#endif
