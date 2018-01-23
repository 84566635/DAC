#include <project.h>
#include <common.h>
#include <string.h>
#include <math.h>
#include <misc.h>

#include STMINCP()
#include STMINCS(_iwdg)
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <hwCommunicator.h>
#include <hwCommunicatorUARThelper.h>
#include <hwCommunicatorSPIhelper.h>
#include <communicator.h>
#include <led.h>
#include <adc.h>
#include <startup.h>


#include <bqueue.h>
#include <debug.h>
#ifdef MADO
#include <convert.h>
#endif
#include <watchdog.h>
#include <messageProcessor.h>
#include <cfg.h>

#ifdef MADO
#include <messageProcessorMADO.h>
#include <wm8805.h>
#endif
#ifdef SMOK
#include <messageProcessorSMOK.h>
#include <uit_monitor.h>
#endif

void printi2s(int *bufSize, char *bufPtr);
#ifdef VT100
#define POS1 22
#define POS2 35
#define POS3 48
#define POS4 61
#define POS5 74
#define POS6 87
#else
#define POS1  8
#define POS2 11
#define POS3 14
#define POS4 17
#define POS5 20
#define POS6 23
#endif

#define QSTAT(arg_q)\
  DPRINTF("|%15s|%8d|%8d|%8d|%8d|%8d|%8d|"CLEAR_LINE"\n", #arg_q, arg_q.count, arg_q.dequeues, arg_q.enqueues, arg_q.maxFilling, arg_q.full, arg_q.empty);

static char *bufPtr = NULL;
static int bSize = 0;
static int *bufSize = &bSize;
void printfCommStats(void)
{
  int i=0;
  portNum_t p;
  DPRINTF("Rx");
  for(i = 0; i < MSG_TYPES_QTY; i++)
    {
      if(MSG_TYPES_TAB[i].code != 0)
        DPRINTF("| %02x", MSG_TYPES_TAB[i].code);
    }
  DPRINTF(CLEAR_LINE"|\n");
  for(p = 0; p < PORTS_NUM; p++)
    {
      if(portData[p].enabled == 0)
        continue;

      DPRINTF(" %d|", p);
      for(i = 0; i < MSG_TYPES_QTY; i++)
        {
          if(MSG_TYPES_TAB[i].code != 0)
            DPRINTF("%3d|", MSG_TYPES_TAB[i].received[p]);
        }
      DPRINTF(CLEAR_LINE"\n");
    }
  DPRINTF("Tx");
  for(i = 0; i < MSG_TYPES_QTY; i++)
    {
      if(MSG_TYPES_TAB[i].code != 0)
        DPRINTF("| %02x", MSG_TYPES_TAB[i].code);
    }
  DPRINTF(CLEAR_LINE"|\n");
  for(p = 0; p < PORTS_NUM; p++)
    {
      if(portData[p].enabled == 0)
        continue;

      DPRINTF(" %d|", p);
      for(i = 0; i < MSG_TYPES_QTY; i++)
        {
          if(MSG_TYPES_TAB[i].code != 0)
            DPRINTF("%3d|", MSG_TYPES_TAB[i].sent[p]);
        }
      DPRINTF(CLEAR_LINE"\n");
    }
}
static int LEDsMask = 0;
static int oldLEDsMask = -1;
void LED_print(int LEDNum, int value)
{
  if(value)
    LEDsMask |= (1<<LEDNum);
  else
    LEDsMask &= ~(1<<LEDNum);
}
#ifdef MADO
static void print_LED(void)
{
  if(LEDsMask != oldLEDsMask)
    {
      char LEDS[] = CUR_SAVE CUR_HOME(0) "DIODY: ["C_GREEN" "NO_COLOR"]["C_YELLOW" "NO_COLOR"]["C_ORANGE" "NO_COLOR"]["C_BLUE" "NO_COLOR"]["C_RED" "NO_COLOR"]["C_RED" "NO_COLOR"]"CLEAR_LINE"\n"CUR_UNSAVE;
      if(LEDsMask&LD1)LEDS[POS1]='1';
      if(LEDsMask&LD2)LEDS[POS2]='1';
      if(LEDsMask&LD3)LEDS[POS3]='1';
      if(LEDsMask&LD4)LEDS[POS4]='1';
      if(LEDsMask&LD5)LEDS[POS5]='1';
      if(LEDsMask&LD6)LEDS[POS6]='1';
      xprintf("%s", LEDS);
      oldLEDsMask = LEDsMask;
    }
}
#endif
#ifdef SMOK
static void print_LED(void)
{
  if(LEDsMask != oldLEDsMask)
    {
      char LEDS[] = CUR_SAVE CUR_HOME(0) "DIODY: ["C_GREEN" "NO_COLOR"]["C_WHITE" "NO_COLOR"]["C_YELLOW" "NO_COLOR"]["C_BLUE" "NO_COLOR"]["C_RED" "NO_COLOR"]"CLEAR_LINE"\n"CUR_UNSAVE;
      if(LEDsMask&LD1)LEDS[POS1]='1';
      if(LEDsMask&LD2)LEDS[POS2]='1';
      if(LEDsMask&LD3)LEDS[POS3]='1';
      if(LEDsMask&LD4)LEDS[POS4]='1';
      if(LEDsMask&LD5)LEDS[POS5]='1';
      xprintf("%s", LEDS);
      oldLEDsMask = LEDsMask;
    }
}
#endif

#ifdef SMOK
extern int myPipeNum;
#endif
const static char conTypeCh[4] =
{
  [DEV_NCONN] = 'N',
  [DEV_PCONN] = 'P',
  [DEV_CONN]  = 'F',
  [DEV_UCONN] = 'U',
};
static void portState(portNum_t p)
{
  if(portData[p].enabled == 0)
    return;

#ifdef MADO
  int myPipeNum = getPipeNum(portData[p].kond_pom);
#endif
  uint32_t now = xTaskGetTickCount();
  uint32_t upConn = (now - portData[p].tsConnected)/1000;
  uint32_t upConnDatagram = (now - portData[p].tsConnectedDatagram)/1000;
  uint32_t upTime = portData[p].uptime/1000;
  uint32_t lastSeen = portData[p].uptime?(now - portData[p].tsLastSeen)/1000:-1;
  switch(portData[p].connState)
    {
    case DEV_NCONN:
      upConn = 0;
    case DEV_PCONN:
      upConnDatagram = 0;
      upTime = 0;
      break;
    }
  spiHelper_t * helper = (spiHelper_t *)portData[p].SPIhelper;
  DPRINTF("|%4s|%c%c%c|%02x|%02x|", portStr[p],
          IS_CON_TYPE_ACTIVE(p, PORT_TYPE_USART)?'U':' ',
          IS_CON_TYPE_ACTIVE(p, PORT_TYPE_SPI)?'S':' ',
          (portData[p].portType == PORT_TYPE_SPI)?'S':((portData[p].portType == PORT_TYPE_USART)?'U':'N'),
          portData[p].kond_pom, portData[p].urzadzenie);
  if(helper)
    {
      int prevConnState = portData[p].prevConnState;
      int connState = portData[p].connState;
      DPRINTF("%c%c|%c|%c|%04x|%08x|%08x|%08x|%8d|%8d|%8d|%8d|%8d|%8d|%8d|"CLEAR_LINE"\n" ,
              conTypeCh[prevConnState],conTypeCh[connState],
              (myPipeNum == 0)?'0':((myPipeNum == 1)?'1':'N'),
              (helper->radioModule == NULL)?'N':((helper->radioModule == &radioModule[0])?'0':'1'),
              helper->connDev.audioConnState,
              helper->connDev.devID,helper->connDev.manID,helper->connDev.prodID,
              upConn, upConnDatagram, upTime, lastSeen,
              portData[p].txFullDrop,
              portData[p].dscResetRx,
              portData[p].dscResetTx);
    }
  else
    DPRINTF("  | | |    |        |        |        |        |        |        |        |        |        |        |"CLEAR_LINE"\n");
}
#ifdef VT100
#define DISPLAY_INTERVAL 1000
extern uint8_t refresh;
#else
#define DISPLAY_INTERVAL 5000
static uint8_t refresh = 0;
#endif
#ifdef SMOK
extern int Uwe_val, Uwy_val, sMaxiL, sMaxvL, meaniL, UwyI, meaniH, UweI, Uwy_k;
extern int temp1, temp2;
extern int kSensADC;
#endif

#ifdef MADO
extern uint32_t usb_stats[10];
#endif
static void dTask(void *pvParameters)
{
  int i = 0;
  while (1)
    {
      if(!cfg.stats)
        {
          vTaskDelay(100);
#ifdef WATCHDOG_DEBUG
          wdogFeed(WATCHDOG_DEBUG());
#endif
          continue;
        }
#ifdef WATCHDOG_DEBUG
      if(i%10)
        wdogFeed(WATCHDOG_DEBUG());
#endif

      vTaskDelay(1);
      if ((i % DISPLAY_INTERVAL) == 0 /* || refresh */)
        {
          bufPtr = dAlloc(DBUFSIZE);
          *bufSize = 0;

          DPRINTF(CUR_SAVE CUR_HOME(1) CLEAR_LINE"\n");
          DPRINTF("Build time %s %s revision:%s"CLEAR_LINE"\n", __DATE__, __TIME__, VERSION);
          if(cfg.stats&0x01)
            {
              DPRINTF("heap: allocs:%d frees:%d allocated:%d last alloc size:%d "CLEAR_LINE"\n", heapAllocs, heapFrees, heapAllocated, heapLastSize);
              DPRINTF("+----------queue+---count+-dequeue+-enqueue+-----max+----full+---empty+"CLEAR_LINE"\n");
#ifdef MADO
              QSTAT(inQueueUSB1);
              QSTAT(inQueueUSB2);
              QSTAT(inQueueSPI1);
              QSTAT(inQueueSPI2);
              QSTAT(inQueueI2S);
              QSTAT(inQueueTest);
              QSTAT(outQueueSAI1);
              QSTAT(outQueueSAI2);
              DPRINTF("+---------------+--------+--------+--------+--------+--------+--------+"CLEAR_LINE"\n");
#endif
              QSTAT(heap_1_32);
              QSTAT(heap_33_192);
              QSTAT(heap_512_512);
              QSTAT(heap_513_1024);
              QSTAT(heap_1025_2112);
              DPRINTF("+---------------+--------+--------+--------+--------+--------+--------+"CLEAR_LINE"\n");
            }
          DPRINTF(CUR_UNSAVE);
          *bufSize = 0;
          xprintf(bufPtr);

          if(cfg.stats&0x01)
            {
#ifdef MADO
              DPRINTF(CUR_SAVE CUR_HOME(19) CLEAR_LINE"\n");
#else
              DPRINTF(CUR_SAVE CUR_HOME(11) CLEAR_LINE"\n");
#endif
            }
          else
            {
              DPRINTF(CUR_SAVE CUR_HOME(3) CLEAR_LINE"\n");
            }
          if(cfg.stats&0x02)
            {
              int m;
              for(m = 0; m < RADIO_MODULES; m++)
                DPRINTF("%d: devId=%08x manId=%08x prodId=%08x smplRate=%6dHz tsPeriod=%4dus dtxFull=%8d txNoConn=%8d status=%04x"CLEAR_LINE"\n", m,
                        radioModule[m].device.devID,
                        radioModule[m].device.manID,
                        radioModule[m].device.prodID,
                        radioModule[m].smplRate,
                        radioModule[m].tsPeriod,
                        radioModule[m].txFull,
                        radioModule[m].txNoConn,
                        radioModule[m].status);

              DPRINTF("+----+---+--+--+--+-+-+----+--r.dev.+--r.man.+-r.prod.+-upConn-+upConnDa+-uptime-+lastSeen+txFullDr+dscRstRx+dscRstTx+"CLEAR_LINE"\n");
              portNum_t p;
              for(p=CEN_PORT; p < PORTS_NUM; p++) portState(p);
              DPRINTF("+----+---+--+--+--+-+-+----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+"CLEAR_LINE"\n");
            }
          if(cfg.stats&0x04)
            printfCommStats();
#ifdef SMOK
          taskENTER_CRITICAL();
          int Uwe_val_local = Uwe_val;
          int Uwy_val_local = Uwy_val;
          int meaniH_local = meaniH;
          int meaniL_local = meaniL;
          int UwyI_local = UwyI;
          int UweI_local = UweI;
          int sMaxiL_local = sMaxiL;
          int sMaxvL_local = sMaxvL;
          int Uwy_k_local = Uwy_k;
          taskEXIT_CRITICAL();
          int Pwe = meaniH_local*UweI_local;
          int Pwy = meaniL_local*UwyI_local;
          int Pdiff = Pwe-Pwy;
#define VF "%s%d.%03d"
#define VFV(arg_value, arg_div) ((arg_value)<0?"-":""), ABS((arg_value))/(arg_div), ABS((arg_value))%(arg_div)

          DPRINTF("%s Uwe_val:"VF"V, Uwy_val:"VF"V, meaniL:"VF"A, meaniH:"VF"A, UwyI:"VF"V, UweI:"VF"V, Pwy:"VF"W, Pwe:"VF"W, Pwe-Pwy:"VF"W, sMaxiL:"VF"A, sMaxvL:"VF"V k=%s@Uwy:"VF"V "CLEAR_LINE"\n", isOn()?"ON":"OFF",
                  VFV(Uwe_val_local, 1000),
                  VFV(Uwy_val_local, 1000),
                  VFV(meaniL_local, 1000),
                  VFV(meaniH_local, 1000),
                  VFV(UwyI_local, 1000),
                  VFV(UweI_local, 1000),
                  VFV(Pwy, 1000000),
                  VFV(Pwe, 1000000),
                  VFV(Pdiff, 1000000),
                  VFV(sMaxiL_local, 1000),
                  VFV(sMaxvL_local, 1000),
                  kSensADC?"33":"3.3",
                  VFV(Uwy_k_local, 1000));
          DPRINTF("temp1:%d.%dC, temp2:%d.%dC"CLEAR_LINE"\n", (temp1>>8),  (10*(temp1&0xFF)/256), (temp2>>8),  (10*(temp2&0xFF)/256));
#endif
#ifdef MADO
          if(cfg.stats&0x08)
            {
              showSpkSets(bufSize, bufPtr);
            }
          if(cfg.stats&0x10)
            {
              WM8805DBG(bufSize, bufPtr);
            }
          if(cfg.stats&0x20)
            {
              printi2s(bufSize, bufPtr);
            }

          DPRINTF("USB In:%d, Out:%d, InIncplt:%d, OutIncmplt:%d, SOF:%d feedback:%06x:%d altSetting:%d xfer:%d"CLEAR_LINE"\n", usb_stats[0], usb_stats[1], usb_stats[2], usb_stats[3], usb_stats[4]
		  , usb_stats[5], ((usb_stats[5]>>14)*1000), usb_stats[7], usb_stats[6]);
          
          printPipes(bufSize, bufPtr);
#endif
          DPRINTF("---------------------------------------------------------------------------------"CLEAR_LINE"\n");
          DPRINTF(CUR_UNSAVE);
          *bufSize = 0;
          xprintf(bufPtr);
          dFree(bufPtr);
          oldLEDsMask = -1;//Reprint LEDs
        }

      if((i%10)==0 || refresh)
        {
          print_LED();
          refresh = 0;
        }
#ifdef  MADO
      if(streamCaptureBuffer && (streamCaptureBuffer->offset == streamCaptureBuffer->size))
        {
          cfg.stats = 0;
          int i = 0;
          taskENTER_CRITICAL();
          for(i = 0; i < streamCaptureBuffer->size; i+=8)
            {
              uint16_t *ptr = (uint16_t *)&(streamCaptureBuffer->data[i]);
              uint32_t left = (((uint32_t)ptr[0]) <<16) + (((uint32_t)ptr[1]));
              uint32_t right = (((uint32_t)ptr[2]) <<16) + (((uint32_t)ptr[3]));
              xprintf("%3d; %9d; %9d; 0x%08x; 0x%08x;\n", i/8, left, right, left, right);
              IWDG_ReloadCounter();
            }
          taskEXIT_CRITICAL();

        }
#endif
      i++;
    }
}

void debugInit(void)
{
  massert(xTaskCreate(dTask, (signed char *)"D", 0x1000 / 4, NULL, 1, NULL) == pdPASS);
}
