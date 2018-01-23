#ifndef _COMMON_H_
# define _COMMON_H_ 1
int disableIrq(void);
void enableIrq(int primask);

#include <stdint.h>
#include <stdlib.h>
#include <xprintf.h>
#include <cfg.h>
#ifdef VT100
#define CLEAR_LINE "\e[K"
#define CLEAR_SCREEN "\e[2J"
#define CUR_HOME(arg_row) "\e["#arg_row";0H"
#define CUR_SAVE "\e[s"
#define CUR_UNSAVE "\e[u"
#define BG_COLOR(arg_color) "\e[4"#arg_color";0m"
#define NO_COLOR "\e[m"
#define C_WHITE  "\e[47;0m"
#define C_BLUE   "\e[44;0m"
#define C_ORANGE "\e[41;0m"
#define C_YELLOW "\e[43;0m"
#define C_GREEN  "\e[42;0m"
#define C_RED    "\e[41;0m"
#define C_FG_RED "\e[31m"
#else
#define CLEAR_LINE ""
#define CLEAR_SCREEN ""
#define CUR_HOME(arg_row) ""
#define CUR_SAVE ""
#define CUR_UNSAVE ""
#define BG_COLOR(arg_color) ""
#define NO_COLOR ""
#define C_WHITE  ""
#define C_BLUE   ""
#define C_ORANGE ""
#define C_YELLOW ""
#define C_GREEN  ""
#define C_RED  ""
#define C_FG_RED  ""
#endif

# define ARRAY_SIZE(arg_name) (sizeof(arg_name)/sizeof(arg_name[0]))
# define massert(arg_cond) if(!(arg_cond)) {xprintf(C_FG_RED"assert in %s(%d): %s\n"NO_COLOR, __FUNCTION__, __LINE__, #arg_cond);volatile int stop = 1; while(stop);}

void hwInit(void);
void hwBoardInit(void);

#define CAT_NAME2(arg_prefix, arg_postfix) arg_prefix##arg_postfix
#ifdef STM32F4xx
#define STMINCS(arg_name) <stm32f4xx##arg_name.h>
#define STMINCP(arg_name) <arg_name##stm32f4xx.h>
#else
#define STMINCS(arg_name) <stm32f10x##arg_name.h>
#define STMINCP(arg_name) <arg_name##stm32f10x.h>
#endif
int getIrqNum(void);

#define APPLY_MASK(argReg, argClrMask, argSetMask) argReg &= ~(argClrMask);  argReg |= (argSetMask);
#define LOW_BYTE_MASK 0x00FF
#define HIGH_BYTE_MASK 0xFF00

#define LL_ERROR   0
#define LL_WARNING 1
#define LL_INFO    2
#define LL_DEBUG   3
#define dprintf(arg_level, ...) {if((arg_level) <= cfg.dlevel) xprintf(__VA_ARGS__);};
#define PRINTF(arg_format, ...) xprintf("\e[32m%d:%s(%d)\e[m "arg_format, xTaskGetTickCount(), __FUNCTION__, __LINE__, ##__VA_ARGS__);
//paste function (preprocessor concatenation)
#define _P1(a) a
#define _P2(a,b)a##b
#define _P3(a,b,c)a##b##c
#define _P4(a,b,c,d)a##b##c##d

#define SPI_NUM_MACRO(arg_num) _P2(SPI, arg_num)

#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

#ifdef DEBUG_UART
void consoleLock(void);
void consoleUnlock(void);
int getChar(void);
void putChar(unsigned char ch);
void sendChar(unsigned char ch);
#include <xprintf.h>
#endif
void hexDump(void* ptr, int size, int rowSize);

void *intSafeMalloc(int size);
void intSafeFree(void *buf);
#define mdelay(arg_ms) vTaskDelay(arg_ms)
#define abs(a) (((int)(a)< 0)?-(int)(a):(int)(a))
#endif
void taskWaitInit(void);
void tasksInitEnd(void);

uint64_t getUptimePrecise(void);
uint64_t getUptime(void);


