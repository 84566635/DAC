#ifndef _PROJECT_H_
#define _PROJECT_H_

#ifndef VERSION
#define VERSION "SMOK_undefined"
#endif

#define DEVMODE_BOSON 1
#define DEVMODE_DAC   2

#define STACK_SIZE 0x1000
#define  SPI_CHECK_FREQ 20       //[ms]
#define  TIMEOUT_DATA_MS  20
#define  MSG_TIMEOUT_DATA  110
#define  MSG_TIMEOUT_REPLY 120
#define ADC_FREQ 1               // odstep pomiedzu pomiarami w ms
#define ADC_WINDOW 10          // w ms  - wielkosc okna , w ktorym sa zbierane pomiary
#define ADC_CHECK  10             // w ms  - wielkosc okna , w ktorym sa zbierane pomiary

#define  TIMEOUT_DATA_MS  20
#define  MSG_TIMEOUT_DATA  110
#define  MSG_TIMEOUT_REPLY 120

#define TIME_TEMP 1000
#define MSG_LEN 32

#define DBUFSIZE 0x1000
#define RADIO_MODULES 1
#define MAX_DATA_SIZE MSG_LEN

#define MSG_TYPES_QTY       22

#define UART_SPEED 115200
#define UART_SPEED_DAC 256000

#define LED_BLINKER_NUM 1
#define LED_BLINKER_SPI_HELPER 0

#define LEDS_NUM        5


typedef struct config_s
{
  volatile int fSize;             // size of image to flash
  volatile int fCRC;              // CRC of image to flash
  volatile int ack50;             // 0 - ACK jest wysylane bez czekania na sharka/PWMa
  volatile int fout22;            // 1 - tylko jedna czestotliwosc wyjsciowa PLLa
  volatile int ckin141;           // 1 -  zwiekszona czestotliwosc wejsciowa
  volatile int disable_alerts;    // wylacz reakcje na alert - maska (0x01 - Uwy=10%Uwe, 0x02 - watchdog, 0x04 - Uwe, 0x08 - Uwy, 0x10 - PWM_STAT, 0x20 - PB3, 0x40 - PB4, 0x80 - PC14, 0x100 - PC15, 0x200 - Imax, 0x400 - tAir, 0x800 - tFET, 0x1000 -PLL;, 0x2000 - PE13
  volatile int empty_buffers;     // 1 - uzywaj czystych buforow
  volatile int trace_messages;    // 1 - wyswietlaj komunikaty bez 0x55, 0xb0, 0xCC, 2 - wyswietlaj tez 0x55, 0xb0, 0xCC
  volatile int ka_period;         // okres wysylania keep aliveow
  volatile int ka_timeout;        // czas alarmowania po nieotrzymaniu keepalive
  volatile int do_not_send_status;// 1 - nie wysylaj 0xb0
  volatile int no_rst_amp;        // nie uzywaj rst_amp
  volatile int manId;             // manufacturere Id
  volatile int kondPom;           // domyslna kondPom
#define FLAGS_RIGHT_CHANNEL_BIT   0
#define FLAGS_RIGHT_CHANNEL_MASK  0x00000001
#define FLAGS_STEREO_SUPPORT_BIT  1
#define FLAGS_STEREO_SUPPORT_MASK 0x00000002
#define FLAGS_PRIMARY_STREAM_BIT  2
#define FLAGS_PRIMARY_STREAM_MASK 0x00000004
#define FLAGS_OMIT_PLL_BIT        6
#define FLAGS_OMIT_PLL_MASK       (1<<6)
#define FLAGS_POWER_CYCLE_VARI_BIT  7
#define FLAGS_POWER_CYCLE_VARI_MASK (1<<7)
#define FLAGS_FAKE_FORWARD_BIT    8
#define FLAGS_FAKE_FORWARD_MASK   (1<<8)
#define FLAGS_AUTO_ON_BIT         9
#define FLAGS_AUTO_ON_MASK        (1<<9)
  volatile int flags;             // bit 0 - left/right, bit 1 - wspiera tryb stereo w Multislave, bit2 - podstawowy stream 0/1, bit 6 - omin ustaianie PLLki, bit 7 - power cycle dla Vari, bit 8 -fake forward (emuluje SHARK/PWM) , bit 9 - 1 - automatycznie wlacz SMOKa gdy flags55 wskazuja na prosbe wlaczenia
  volatile int x92delay;          // wartosc opoznienia w ms pomiedzy otrzymaniem 0x92 a ustawieniem PLL
  volatile int xPLLdelay;         // wartosc opoznienia w ms pomiedzy PLL a wyslaniem 0x92 do sharka
  volatile int stats;             // pokaz statystyki
  volatile int conDelay;          // opoznienie polaczenia SMOKa fo MADO
  volatile int proto;             // 0-SMOK lub eval, 1-MADO Proto emulujacy SMOKa, 2-wylaczony (radio w resecie)
  volatile int dlevel;            // traces level
  volatile int sharcSPISpeed;     // predkosc programowania sharc'a
  volatile int sharcProgramRetry; // liczba powtorzen programowania sharc'a
  volatile int term1Addr;         // adres i2c termometru air
  volatile int term2Addr;         // adres i2c termometru fet
  volatile int msgRoundMax[2];    // liczba slotow rundy algorytmu wysylania komunikatow B0
  volatile int msgRoundTime[2];   // czas jednego slotu
  volatile int msgUwe[2];         // maska wysylanie Uwe
  volatile int msgUwy[2];         // maska wysylanie Uwy
  volatile int msgIavg[2];        // maska wysylanie Iavg
  volatile int msgImax[2];        // maska wysylanie Imax
  volatile int msgTemp[2];        // maska wysylanie temperatur
  volatile int imax_alarm;        // wartosc alarmu Imax
  volatile int t_air_max;         // wartosc alarmu temperatuy air w wielokrotnosci 0.5C
  volatile int t_fet_max;         // wartosc alarmu temperatuy fet
  volatile int temp_time;         // okres pomiaru temperatury
  volatile int uwyRange;          // wartosc napiecie przelaczania wzmocnienia
  volatile int uwe_alarm;          // max Uwe przy starcie
  volatile int uwy_alarm;          // max Uwy przy starcie
  volatile int a_nom;             // wspolczynnik a (licznik)
  volatile int a_den;             // wspolczynnik a (mianownik)
  volatile int b;                 // wspolczynnik b[mA]
  volatile int c;                 // wspolczynnik c[mA]
  volatile int powerOnDelay;      //opoznienie od wlaczenia zasilania do przeslania komunikatu 50 do sharka/pwma
  volatile int ampEnableDelay50;    //opoznienie przed zalaczeniem wzmacniacza rst_amp
  volatile int ampEnableDelay92;    //opoznienie przed zalaczeniem wzmacniacza rst_amp
  volatile int ampEnableDelay58;    //opoznienie przed zalaczeniem wzmacniacza rst_amp
  volatile int plli2c;            //numer I2C, do ktorego jest podlaczony PLL
  volatile int pllRetryNum;       //liczba prob ustawienia PLL
  volatile int polar;            //polar1 - bit0, polar2 - bit1
#define C_TIMEOUT_50     CFG_PATCH(CFG_IDX(timeout50))
  volatile int timeout50; //Czas na odpowiedz na komunikat 50
#define DEVICE_MODE     CFG_PATCH(CFG_IDX(centralDeviceMode))
  volatile int centralDeviceMode; /* possible values: BOSON=1 (and others remaining values as default), DAC=2 */
  volatile int fillup[128-59];    //Fillup to 128 elements
} __attribute__((packed)) config_t;

#define CONFIG_CONST                                            \
  const __attribute__((section(".nvs"))) config_t cfgnvs =      \
  {                                                             \
    CFG_EVAL(fSize),                                            \
    CFG_EVAL(fCRC),                                             \
    CFG_EVAL(ack50),                                            \
    CFG_EVAL(fout22),                                           \
    CFG_EVAL(ckin141),                                          \
    CFG_EVAL(disable_alerts),                                   \
    CFG_EVAL(empty_buffers),                                    \
    CFG_EVAL(trace_messages),                                   \
    CFG_EVAL(ka_period),                                        \
    CFG_EVAL(ka_timeout),                                       \
    CFG_EVAL(do_not_send_status),                               \
    CFG_EVAL(no_rst_amp),                                       \
    CFG_EVAL(manId),                                            \
    CFG_EVAL(kondPom),                                          \
    CFG_EVAL(flags),                                            \
    CFG_EVAL(x92delay),                                         \
    CFG_EVAL(xPLLdelay),                                        \
    CFG_EVAL(stats),                                            \
    CFG_EVAL(conDelay),                                         \
    CFG_EVAL(proto),                                            \
    CFG_EVAL(dlevel),                                           \
    CFG_EVAL(sharcSPISpeed),                                    \
    CFG_EVAL(sharcProgramRetry),                                \
    CFG_EVAL(term1Addr),                                        \
    CFG_EVAL(term2Addr),                                        \
    CFG_EVAL(msgRoundMax[0]),                                   \
    CFG_EVAL(msgRoundMax[1]),                                   \
    CFG_EVAL(msgRoundTime[0]),                                  \
    CFG_EVAL(msgRoundTime[1]),                                  \
    CFG_EVAL(msgUwe[0]),                                        \
    CFG_EVAL(msgUwe[1]),                                        \
    CFG_EVAL(msgUwy[0]),                                        \
    CFG_EVAL(msgUwy[1]),                                        \
    CFG_EVAL(msgIavg[0]),                                       \
    CFG_EVAL(msgIavg[1]),                                       \
    CFG_EVAL(msgImax[0]),                                       \
    CFG_EVAL(msgImax[1]),                                       \
    CFG_EVAL(msgTemp[0]),                                       \
    CFG_EVAL(msgTemp[1]),                                       \
    CFG_EVAL(imax_alarm),                                       \
    CFG_EVAL(t_air_max),                                        \
    CFG_EVAL(t_fet_max),                                        \
    CFG_EVAL(temp_time),                                        \
    CFG_EVAL(uwyRange),                                         \
    CFG_EVAL(uwe_alarm),                                        \
    CFG_EVAL(uwy_alarm),                                        \
    CFG_EVAL(a_nom),                                            \
    CFG_EVAL(a_den),                                            \
    CFG_EVAL(b),                                                \
    CFG_EVAL(c),                                                \
    CFG_EVAL(powerOnDelay),                                     \
    CFG_EVAL(ampEnableDelay50),                                 \
    CFG_EVAL(ampEnableDelay92),                                 \
    CFG_EVAL(ampEnableDelay58),                                 \
    CFG_EVAL(plli2c),                                           \
    CFG_EVAL(pllRetryNum),                                      \
    CFG_EVAL(polar),                                            \
    CFG_EVAL(timeout50),                                        \
    CFG_EVAL(centralDeviceMode),                                \
  }

#define CONFIG_PRINT()                               \
  PRINT_CFG(fSize);                                  \
  PRINT_CFG(fCRC);                                   \
  PRINT_CFG(ack50);                                  \
  PRINT_CFG(fout22);                                 \
  PRINT_CFG(ckin141);                                \
  PRINT_CFG(disable_alerts);                         \
  PRINT_CFG(empty_buffers);                          \
  PRINT_CFG(trace_messages);                         \
  PRINT_CFG(ka_period);                              \
  PRINT_CFG(ka_timeout);                             \
  PRINT_CFG(do_not_send_status);                     \
  PRINT_CFG(no_rst_amp);                             \
  PRINT_CFG(manId);                                  \
  PRINT_CFG(kondPom);                                \
  PRINT_CFG(flags);                                  \
  PRINT_CFG(x92delay);                               \
  PRINT_CFG(xPLLdelay);                              \
  PRINT_CFG(stats);                                  \
  PRINT_CFG(conDelay);                               \
  PRINT_CFG(proto);                                  \
  PRINT_CFG(dlevel);                                 \
  PRINT_CFG(sharcSPISpeed);                          \
  PRINT_CFG(sharcProgramRetry);                      \
  PRINT_CFG(term1Addr);                              \
  PRINT_CFG(term2Addr);                              \
  PRINT_CFG(msgRoundMax[0]);                         \
  PRINT_CFG(msgRoundMax[1]);                         \
  PRINT_CFG(msgRoundTime[0]);                        \
  PRINT_CFG(msgRoundTime[1]);                        \
  PRINT_CFG(msgUwe[0]);                              \
  PRINT_CFG(msgUwe[1]);                              \
  PRINT_CFG(msgUwy[0]);                              \
  PRINT_CFG(msgUwy[1]);                              \
  PRINT_CFG(msgIavg[0]);                             \
  PRINT_CFG(msgIavg[1]);                             \
  PRINT_CFG(msgImax[0]);                             \
  PRINT_CFG(msgImax[1]);                             \
  PRINT_CFG(msgTemp[0]);                             \
  PRINT_CFG(msgTemp[1]);                             \
  PRINT_CFG(imax_alarm);                             \
  PRINT_CFG(t_air_max);                              \
  PRINT_CFG(t_fet_max);                              \
  PRINT_CFG(temp_time);                              \
  PRINT_CFG(uwyRange);                               \
  PRINT_CFG(uwe_alarm);                              \
  PRINT_CFG(uwy_alarm);                              \
  PRINT_CFG(a_nom);                                  \
  PRINT_CFG(a_den);                                  \
  PRINT_CFG(b);                                      \
  PRINT_CFG(c);                                      \
  PRINT_CFG(powerOnDelay);                           \
  PRINT_CFG(ampEnableDelay50);                       \
  PRINT_CFG(ampEnableDelay92);                       \
  PRINT_CFG(ampEnableDelay58);                       \
  PRINT_CFG(plli2c);                                 \
  PRINT_CFG(pllRetryNum);                            \
  PRINT_CFG(polar);                                  \
  PRINT_CFG(timeout50);				    			 \
  PRINT_CFG(centralDeviceMode);					     \
 

typedef enum
{
  LD1 = 0x01,
  LD2 = 0x02,
  LD3 = 0x04,
  LD4 = 0x08,
  LD5 = 0x10,
} ledsNum_t;
#define LED_GREEN  LD1
#define LED_WHITE  LD2|LD1
#define LED_YELLOW LD3
#define LED_BLUE   LD4
#define LED_RED    LD5


typedef enum
{
  CEN_PORT,
  PER1_PORT,
  PER2_PORT,
  PORTS_NUM,
  PORT_INVALID,
} portNum_t;
extern const char *portStr[PORTS_NUM];
#define DEBUG_UART UART5

#define URZADZENIE    ((cfg.flags & FLAGS_RIGHT_CHANNEL_MASK)?2:1)

#define WATCHDOG_LED(...)       (1<<0)
#define WATCHDOG_HWCOMMSPI(...) (1<<1)
#define WATCHDOG_HWCOMM(...)    (1<<2)
#define WATCHDOG_COMM(...)      (1<<3)
#define WATCHDOG_ADC(...)       (1<<4)
#define WATCHDOG_DEBUG(...)     (1<<5)
#define WATCHDOG_EXPECTED					\
  (WATCHDOG_LED(0)|WATCHDOG_HWCOMMSPI(0)|WATCHDOG_HWCOMM(0)	\
   |WATCHDOG_COMM(0)|WATCHDOG_ADC(0)|WATCHDOG_DEBUG(0))

#define WATCHDOG_TIMEOUT     20 //20s

void sharc_spi_init(void);
#endif
