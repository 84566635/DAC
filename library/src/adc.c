#include <string.h>
#include <stdint.h>
#include "common.h"
#include "startup.h"
#include "project.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include <communicator.h>
#include <adc.h>
#include <watchdog.h>
#include <cfg.h>

#define ADC_RESOLUTION 12 /*bits*/
#define ADC_MAX ((1<<ADC_RESOLUTION)-1)
int bufferNum = 0;
xSemaphoreHandle ADC_xSemaphore;
//Sampling of a channel takes ~0.36us (3 +12 cycles of 42MHz clock)
//Sampling of 4 channels takes ~1.43us
//Buffer half size must be ADC_FREQ/0.36us which makes  ADC_FREQ/1.43us samples
#define SAMPLES_IN_42MHz_3    2800  /* 700kHz (per 1 of 4 channels*/
#define SAMPLES_IN_42MHz_15   1556  /* 389kHz */
#define SAMPLES_IN_42MHz_28   1050  /* 262kHz */
#define SAMPLES_IN_42MHz_56    618  /* 154kHz */
#define SAMPLES_IN_42MHz_84    438  /* 109kHz */
#define SAMPLES_IN_42MHz_112   339  /*  84kHz */
#define SAMPLES_IN_42MHz_144   269  /*  67kHz */
#define SAMPLES_IN_42MHz_480    85  /*  21kHz */

//set for 250kHz per channel
#define SAMPLES_IN_42MHz SAMPLES_IN_42MHz_28

#define ADC_CHANNELS 4
#define SAMPLES_PER_ADC_FREQ ((SAMPLES_IN_42MHz*ADC_FREQ)/ADC_CHANNELS)
#define HALF_BUFFER_SIZE     (SAMPLES_PER_ADC_FREQ*sizeof(uint16_t)*ADC_CHANNELS)
#define BUFFER_SIZE          (HALF_BUFFER_SIZE*2)
#define SET_MAX(arg_value, arg_tempValue) if((arg_value) < (arg_tempValue)) arg_value = arg_tempValue
#define SET_MIN(arg_value, arg_tempValue) if((arg_value) > (arg_tempValue)) arg_value = arg_tempValue
#define SET_MIN_MAX_ACCU(arg_param, arg_sampleIdx, arg_paramIdx)        \
  arg_param##accu += buffer[bufferNum][4*(arg_sampleIdx)+(arg_paramIdx)];       \
  SET_MAX(arg_param##max, buffer[bufferNum][4*(arg_sampleIdx)+(arg_paramIdx)]); \
  SET_MIN(arg_param##min, buffer[bufferNum][4*(arg_sampleIdx)+(arg_paramIdx)]);

int sensADC = 0;
int sensADCnew = 0;

int kSensADC = 0;
static const int k10[2] = {33, 330};

int Uwe_val, Uwy_val, sMaxiL, sMaxvL, meaniL, UwyI, meaniH, UweI, Uwy_k;
unsigned long long Uwe_val_data, Uwy_val_data, meaniL_data, UwyI_data, meaniH_data, UweI_data, Uwy_k_data;
unsigned int Uwe_val_smpl, Uwy_val_smpl, meaniL_smpl, UwyI_smpl, meaniH_smpl, UweI_smpl, Uwy_k_smpl;
static uint64_t getImV(uint64_t  data)
{
  //Convert to ADC voltage
  return  (3300*data)/4095;
}

static uint64_t getmV(uint64_t  data)
{
  //Convert to ADC voltage
  return  (3300*12*data)/4095;
}

static int getVmV(uint64_t data)
{
  return  getmV(data);
}

static int getAvgVmV(uint64_t data, int smpl)
{
  if(smpl)
    {
      //Get average
      return getmV(data)/smpl;
    }
  return 0;
}

static int getAvgImA(uint64_t data, int smpl)
{
  if(smpl)
    {
      //Get average and convert yo I
      return (int)((100ULL * getImV(data))/(uint64_t)(smpl*k10[kSensADC]));
    }
  return 0;
}

static int getImA(uint64_t data)
{
  return (int)((100ULL * getImV(data))/(uint64_t)(k10[kSensADC]));
}

static volatile char measeureDoubleBuffer[BUFFER_SIZE]__attribute__((section(".noload")));
static volatile uint16_t *buffer[2] = {(uint16_t *) &measeureDoubleBuffer[0], (uint16_t *) &measeureDoubleBuffer[HALF_BUFFER_SIZE]};
static void adcTask(void *pvParameters)
{
  taskWaitInit();
  //First time measure for ADC_CHECK time
  int measures_per_message = ADC_CHECK / ADC_FREQ;
  memset((void *)measeureDoubleBuffer, 0, BUFFER_SIZE);
  ADC_startConversion((void *)measeureDoubleBuffer, BUFFER_SIZE);

  while (1)
    {
      int V1min = ADC_MAX, V1max = 0, V1accu = 0;
      int I1min = ADC_MAX, I1max = 0, I1accu = 0;
      int V2min = ADC_MAX, V2max = 0, V2accu = 0;
      int I2min = ADC_MAX, I2max = 0, I2accu = 0;
      int V2I1max = 0;
      int measureNum;
      for (measureNum = 0; measureNum < measures_per_message; measureNum++)
        {
          //Wait before next measure
          massert(xSemaphoreTake(ADC_xSemaphore, portMAX_DELAY ) == pdTRUE );
          int sample;
          for (sample = 0; sample < SAMPLES_PER_ADC_FREQ; sample++)
            {
              //Snapshot of voltage for max I1
              if(I1max < buffer[bufferNum][4*(sample)+(2)])
                V2I1max = buffer[bufferNum][4*(sample)+(1)];

              SET_MIN_MAX_ACCU(V1, sample, 0);
              SET_MIN_MAX_ACCU(V2, sample, 1);
              SET_MIN_MAX_ACCU(I1, sample, 2);
              SET_MIN_MAX_ACCU(I2, sample, 3);
            }
        }

      taskENTER_CRITICAL();
      int samplesNum = (SAMPLES_PER_ADC_FREQ * measureNum);
      kSensADC = sensADC;
      Uwe_val_data += V1accu;
      Uwy_val_data += V2accu;
      Uwy_k_data += V2accu;
      meaniL_data  += I1accu;
      UwyI_data += V2accu;
      meaniH_data  += I2accu;
      UweI_data += V1accu;

      Uwe_val_smpl += samplesNum;
      Uwy_val_smpl += samplesNum;
      Uwy_k_smpl += samplesNum;
      meaniL_smpl  += samplesNum;
      UwyI_smpl += samplesNum;
      meaniH_smpl  += samplesNum;
      UweI_smpl += samplesNum;

      Uwe_val = getAvgVmV(Uwe_val_data, Uwe_val_smpl);
      Uwy_val = getAvgVmV(Uwy_val_data, Uwy_val_smpl);
      Uwy_k = getAvgVmV(Uwy_k_data, Uwy_k_smpl);
      UweI = getAvgVmV(UweI_data, UweI_smpl);
      meaniH  = getAvgImA(meaniH_data, meaniH_smpl) - cfg.c;
      UwyI = getAvgVmV(UwyI_data, UwyI_smpl);
      meaniL  = getAvgImA(meaniL_data, meaniL_smpl) - (int)(((int64_t)UwyI*(int64_t)cfg.a_nom)/(int64_t)cfg.a_den) - cfg.b;
      int newMaxiL = getImA(I1max) - (int)(((int64_t)UwyI*(int64_t)cfg.a_nom)/(int64_t)cfg.a_den) - cfg.b;
      if(sMaxiL < newMaxiL)
        {
          sMaxiL  = newMaxiL;
          sMaxvL  = getVmV(V2I1max);
        }
      taskEXIT_CRITICAL();

      //Send result to the Communicator
      messageADCMeasure((SAMPLES_PER_ADC_FREQ * measureNum), sensADC,
                        V1accu, V2accu,
                        I1max, V2I1max, I1accu, I2accu);

      //All measures but first are for ADC_WINDOW time
      measures_per_message = ADC_WINDOW / ADC_FREQ;
#ifdef WATCHDOG_ADC
      wdogFeed(WATCHDOG_ADC(0));
#endif
    }
}

static void adcProtoTask(void *pvParameters)
{
  while (1)
    {
      mdelay(100);
#ifdef WATCHDOG_ADC
      wdogFeed(WATCHDOG_ADC(0));
#endif
    }
}

void ADCInit(void)
{
  if(!(cfg.proto&0x01))
    {
      ADC_hwInit();

      //Create the task
      massert(xTaskCreate(adcTask, (signed char *)"ADC", 0x1000 / 4, NULL, 1, NULL) == pdPASS);
    }
  else
    {
      massert(xTaskCreate(adcProtoTask, (signed char *)"ADC", 0x1000 / 4, NULL, 1, NULL) == pdPASS);
    }
}
