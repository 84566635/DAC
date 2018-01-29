/**
 ******************************************************************************
 * @file    usbd_audio.c
 * @author  MCD Application Team
 * @version V2.4.2
 * @date    11-December-2015
 * @brief   This file provides the Audio core functions.
 *
 * @verbatim
 *
 *          ===================================================================
 *                                AUDIO Class  Description
 *          ===================================================================
 *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
 *           Audio Devices V1.0 Mar 18, 98".
 *           This driver implements the following aspects of the specification:
 *             - Device descriptor management
 *             - Configuration descriptor management
 *             - Standard AC Interface Descriptor management
 *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
 *             - 1 Audio Streaming Endpoint
 *             - 1 Audio Terminal Input (1 channel)
 *             - Audio Class-Specific AC Interfaces
 *             - Audio Class-Specific AS Interfaces
 *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
 *             - Audio Feature Unit (limited to Mute control)
 *             - Audio Synchronization type: Asynchronous
 *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
 *          The current audio class version supports the following audio features:
 *             - Pulse Coded Modulation (PCM) format
 *             - sampling rate: 48KHz.
 *             - Bit resolution: 16
 *             - Number of channels: 2
 *             - No volume control
 *             - Mute/Unmute capability
 *             - Asynchronous Endpoints
 *
 * @note     In HS mode and when the DMA is used, all variables and data structures
 *           dealing with the DMA during the transaction process should be 32-bit aligned.
 *
 *
 *  @endverbatim
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "bqueue.h"
#include "convert.h"


/** @addtogroup STM32_USB_DEVICE_LIBRARY
 * @{
 */


/** @defgroup USBD_AUDIO
 * @brief usbd core module
 * @{
 */

/** @defgroup USBD_AUDIO_Private_TypesDefinitions
 * @{
 */
/**
 * @}
 */


/** @defgroup USBD_AUDIO_Private_Defines
 * @{
 */

/**
 * @}
 */


/** @defgroup USBD_AUDIO_Private_Macros
 * @{
 */

/**
 * @}
 */
static volatile uint8_t flag = 0;
static volatile uint32_t usbd_audio_AltSet = 0;
static volatile uint32_t frame_number = 0;
static volatile uint32_t feedback_data = 0x000000;
static uint8_t alignReset = 0;
static int total = 0;

#define USB_BUF_SIZE 1024
uint32_t usb_stats[10];
uint32_t usb_info[10];
static bBuffer_t *currentBuffer = NULL;
static __attribute__((aligned(4))) uint8_t xferBuffer[USB_BUF_SIZE];



/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
 * @{
 */


static uint8_t  USBD_AUDIO_Init (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

static uint8_t  USBD_AUDIO_DeInit (USBD_HandleTypeDef *pdev,
                                   uint8_t cfgidx);

static uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev,
                                  USBD_SetupReqTypedef *req);

static uint8_t  *USBD_AUDIO_GetCfgDesc (uint16_t *length);

static uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length);

static uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_EP0_RxReady (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_EP0_TxReady (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_SOF (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

static void AUDIO_REQ_GetRange(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

static void AUDIO_REQ_SetRange(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

/**
 * @}
 */

/** @defgroup USBD_AUDIO_Private_Variables
 * @{
 */

USBD_ClassTypeDef  USBD_AUDIO =
{
  USBD_AUDIO_Init,
  USBD_AUDIO_DeInit,
  USBD_AUDIO_Setup,
  USBD_AUDIO_EP0_TxReady,
  USBD_AUDIO_EP0_RxReady,
  USBD_AUDIO_DataIn,
  USBD_AUDIO_DataOut,
  USBD_AUDIO_SOF,
  USBD_AUDIO_IsoINIncomplete,
  USBD_AUDIO_IsoOutIncomplete,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetDeviceQualifierDesc,
};


//#define NO_24_BIT_FMT
//#define NO_16_BIT_FMT

#ifndef NO_24_BIT_FMT
#define ALTERNATE_SETTINGS_24 6
#else
#define ALTERNATE_SETTINGS_24 0
#endif
#ifndef NO_16_BIT_FMT
#define ALTERNATE_SETTINGS_16 6
#else
#define ALTERNATE_SETTINGS_16 0
#endif
#define ALTERNATE_SETTINGS   (1+ALTERNATE_SETTINGS_24+ALTERNATE_SETTINGS_16)
#define AUDIO_CONFIG_DESC_SIZE                        (95 + (ALTERNATE_SETTINGS-1)*(53))
//#define AUDIO_PACKET_SZE(arg_bytespersample, frq) 1000
#define AUDIO_PACKET_SZE(arg_bytespersample, frq) ((((frq * (arg_bytespersample) * 2)+999)/1000 + (6 - ((((frq * (arg_bytespersample) * 2)+999)/1000 ) % 6))) < 1025 ? (((frq * (arg_bytespersample) * 2)+999)/1000 + (6 - ((((frq * (arg_bytespersample) * 2)+999)/1000 ) % 6))) : 976)


#define VALUE2(arg_value) (((arg_value)>>0)&0xFF),(((arg_value)>>8)&0xFF)
#define VALUE3(arg_value) (((arg_value)>>0)&0xFF),(((arg_value)>>8)&0xFF),(((arg_value)>>16)&0xFF)
#define VALUE4(arg_value) (((arg_value)>>0)&0xFF),(((arg_value)>>8)&0xFF),(((arg_value)>>16)&0xFF),(((arg_value)>>24)&0xFF)
#define ALTERNATE_SETTING(arg_bitspersample, arg_bytespersample, arg_freq) \
\
    /* USB Speaker Standard AS Interface Descriptor */ \
    AUDIO_INTERFACE_DESC_SIZE,              /* bLength */			\
    USB_DESC_TYPE_INTERFACE,        /* bDescriptorType */		\
    0x01,                                 /* bInterfaceNumber */	\
    __COUNTER__+1,                      /* bAlternateSetting */			\
    0x02,                                 /* bNumEndpoints */		\
    USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */		\
    AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */	\
    AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */	\
    0x00,                                 /* iInterface */		\
                                                              \
    /* USB Speaker Audio Streaming Interface Descriptor */		\
    0x10,  /* bLength */			\
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */		\
    AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */	\
    0x01,                                 /* bTerminalLink */		\
    0x01,                                 /* bmControls */ \
    0x01,                                 /* bFormatType */			\
    0x01, 0x00, 0x00, 0x00,               /* bmFormat*/		\
    0x02,                                 /* bNrChannels */ \
    0x03, 0x00, 0x00, 0x00,               /* bmChannelConfig */ \
    0x00,                                 /* iChannelNames */ \
                                                                \
    /* USB Speaker Audio Type I Format Interface Descriptor */		\
    0x06,                                 /* bLength */			\
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */		\
    AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */	\
    AUDIO_FORMAT_TYPE_I,                  /* bFormatType */		\
    (arg_bytespersample),                 /* bSubFrameSize  */		\
    (arg_bitspersample),                  /* bBitResolution  */		\
   /* 0x01,  */                               /* bSamFreqType */		\
   /* VALUE3(arg_freq),     */                /* wFrequency */		\
                                                      \
    /* Endpoint 1 - Standard Descriptor */				\
    AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */			\
    USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */		\
    AUDIO_OUT_EP,                         /* bEndpointAddress */	\
    0x05,                                  /* bmAttributes */		\
    VALUE2(48+AUDIO_PACKET_SZE((arg_bytespersample), (arg_freq))), /* wMaxPacketSize */ \
    0x01,                                 /* bInterval */		\
                                                            \
    /* Endpoint - Audio Streaming Descriptor*/				\
    AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */			\
    AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */		\
    AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */		\
    0x00,                                 /* bmAttributes */		\
    0x00,                                 /* bmControls */ \
    0x00,                                 /* bLockDelayUnits */		\
    0x00, 0x00,                           /* wLockDelay */		\
                                                  \
    /* Endpoint 82 - Standard Descriptor */				\
    AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */			\
    USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */		\
    AUDIO_IN_EP,                          /* bEndpointAddress 2 in endpoint*/ \
    0x11,                                 /* bmAttributes */		\
    0x04, 0x00,                          /* wMaxPacketSize in Bytes  */ \
    0x01                                 /* bInterval */



/* USB AUDIO device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDesc[AUDIO_CONFIG_DESC_SIZE] __ALIGN_END =
{
  /* Configuration 1 */
  0x09, /* bLength */
  USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType */
  LOBYTE(AUDIO_CONFIG_DESC_SIZE), /* wTotalLength  109 bytes*/
  HIBYTE(AUDIO_CONFIG_DESC_SIZE), 
  0x02, /* bNumInterfaces */
  0x01, /* bConfigurationValue */
  0x01, /* iConfiguration */
  0x80, /* bmAttributes  BUS Powred*/
  0xF0, /* bMaxPower = 100 mA*/
  /* 09 byte*/
  
  /* Standard Interface Association Descriptor */
  0x08, /* bLength(0x08) */
  0x0B, /* bDescriptorType(0x0B) */
  0x00, /* bFirstInterface(0x00) */
  0x02, /* bInterfaceCount(0x02) */
  0x01, /* bFunctionClass(0x01): AUDIO */
  0x00, /* bFunctionSubClass(0x00) */
  0x20, /* bFunctionProtocol(0x2000): 2.0 AF_VERSION_02_00 */
  0x00, /* iFunction(0x00) */

  /* USB Speaker Standard interface descriptor */
  AUDIO_INTERFACE_DESC_SIZE, /* bLength */
  USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
  0x00, /* bInterfaceNumber */
  0x00, /* bAlternateSetting */
  0x00, /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOCONTROL, /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED, /* bInterfaceProtocol */
  0x00, /* iInterface */
  /* 09 byte*/

  /* USB Speaker Class-specific AC Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE, /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
  AUDIO_CONTROL_HEADER, /* bDescriptorSubtype */
  0x00,0x02, /* 2.00 */ /* bcdADC */
  0x01, /* bCategory */
  60, 0x00, /* wTotalLength = 48*/
  0x00, /* bmControls */
  /* 09 byte*/
  
  /* Clock Source Descriptor(4.7.2.1) */
  0x08, /* bLength(0x08) */
  0x24, /* bDescriptorType(0x24): CS_INTERFACE */
  0x0A, /* bDescriptorSubType(0x0A): CLOCK_SOURCE */
  0x10, /* bClockID(0x10): CLOCK_SOURCE_ID */
  0x01, /* bmAttributes(0x01): internal fixed clock */
  0x07, /* bmControls(0x07):
   clock frequency control: 0b11 - host programmable;
   clock validity control: 0b01 - host read only */
  0x00, /* bAssocTerminal(0x00) */
  0x01, /* iClockSource(0x01): Not requested */

  /* USB Speaker Input Terminal Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE, /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL, /* bDescriptorSubtype */
  0x01, /* bTerminalID */
  0x01, 0x01, /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101 */
  0x00, /* bAssocTerminal */
  0x10, /* bCSourceID */ 
  0x02, /* bNrChannels */
  0x03, 0x00, 0x00, 0x00, /* wChannelConfig 0x00000003  STEREO */
  0x00, /* iChannelNames */
  0x00, 0x00, /* bmControls */
  0x00, /* iTerminal */
  /* 17 byte*/

  /* USB Speaker Audio Feature Unit Descriptor */
  0x0E, /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT, /* bDescriptorSubtype */
  AUDIO_OUT_STREAMING_CTRL, /* bUnitID */
  0x01, /* bSourceID */
  0x01, 0x00, 0x00, 0x00, // |AUDIO_CONTROL_VOLUME, /* bmaControls(0) */
  0x01, 0x00, 0x00, 0x00, // |AUDIO_CONTROL_VOLUME, /* bmaControls(0) */
  0x00, /* iFeature */
  /* 14 byte*/ 

  /*USB Speaker Output Terminal Descriptor */
  0x0C, /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL, /* bDescriptorSubtype */
  0x03, /* bTerminalID */
  0x01, 0x03, /* wTerminalType  0x0301*/
  0x00, /* bAssocTerminal */
  0x02, /* bSourceID */
  0x10, /* bCSourceID */
  0x00, 0x00, /* bmControls */
  0x00, /* iTerminal */
  /* 12 byte*/

  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Zero Bandwith */
  /* Interface 1, Alternate Setting 0                                             */
  AUDIO_INTERFACE_DESC_SIZE, /* bLength */
  USB_DESC_TYPE_INTERFACE, /* bDescriptorType */
  0x01, /* bInterfaceNumber */
  0x00, /* bAlternateSetting */
  0x00, /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO, /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED, /* bInterfaceProtocol */
  0x00, /* iInterface */
  /* 09 byte*/

#ifndef NO_24_BIT_FMT
  ALTERNATE_SETTING(24, 3, 44100),
  ALTERNATE_SETTING(24, 3, 48000),
  ALTERNATE_SETTING(24, 3, 88200),
  ALTERNATE_SETTING(24, 3, 96000),
  ALTERNATE_SETTING(24, 3, 176400),
  ALTERNATE_SETTING(24, 3, 192000),
#endif
#ifndef NO_16_BIT_FMT
  ALTERNATE_SETTING(16, 2, 44100),
  ALTERNATE_SETTING(16, 2, 48000),
  ALTERNATE_SETTING(16, 2, 88200),
  ALTERNATE_SETTING(16, 2, 96000),
  ALTERNATE_SETTING(16, 2, 176400),
  ALTERNATE_SETTING(16, 2, 192000),
#endif
} ;


static int checkDesc[(AUDIO_CONFIG_DESC_SIZE == sizeof(USBD_AUDIO_CfgDesc))?0:-1];

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END=
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0xEF,
  0x02,
  0x01,
  0x40,
  0x01,
  0x00,
};

typedef struct
{
  int format;
  int rate;
  uint32_t rxSize;
  int alignment;
  uint32_t bytesPerSample;
  uint32_t feedback[2];
}usbFmt_t;

uint8_t freq[2 + 12 * ALTERNATE_SETTINGS];
uint8_t freq2[14];

#define FMT(arg_bits) ((arg_bits)==16)?IF_16L16P:IF_24L24P
#define ALIGN(arg_bits) ((arg_bits)==16)?16:24
#define BPS(arg_bits) ((arg_bits)==16)?4:6
#define FB(arg_freq) {(((arg_freq)/1000)-1)<<14, (((arg_freq)/1000)+1)<<14}

#define USBFMT(arg_bits, arg_freq) {FMT(arg_bits), (arg_freq), 48+AUDIO_PACKET_SZE(BPS(arg_bits)/2, (arg_freq)), ALIGN(arg_bits), BPS(arg_bits), FB(arg_freq)}
static const usbFmt_t usbFmt[ALTERNATE_SETTINGS] =
  {
    USBFMT(24, 44100),
#ifndef NO_24_BIT_FMT
    USBFMT(24, 44100),
    USBFMT(24, 48000),
    USBFMT(24, 88200),
    USBFMT(24, 96000),
    USBFMT(24, 176400),
    USBFMT(24, 192000),
#endif
#ifndef NO_16_BIT_FMT
    USBFMT(16, 44100),
    USBFMT(16, 48000),
    USBFMT(16, 88200),
    USBFMT(16, 96000),
    USBFMT(24, 176400),
    USBFMT(24, 192000),
#endif
  };
#define GET_ALIGN_REST(arg_size) ((arg_size)%usbFmt[usbd_audio_AltSet].alignment)
#define CUT_ALIGN_REST(arg_size) ((arg_size) - GET_ALIGN_REST(arg_size))
#define GET_MIN(arg_a, arg_b) (((arg_a) > (arg_b))?(arg_b):(arg_a))


/**
 * @}
 */

/** @defgroup USBD_AUDIO_Private_Functions
 * @{
 */

/**
 * @brief  USBD_AUDIO_Init
 *         Initialize the AUDIO interface
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t  USBD_AUDIO_Init (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx)
{
  (void)checkDesc;

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev,
                 AUDIO_OUT_EP,
                 USBD_EP_TYPE_ISOC,
                 AUDIO_OUT_PACKET);

  /* Open EP IN */
  USBD_LL_OpenEP(pdev,
                 AUDIO_IN_EP,
                 USBD_EP_TYPE_ISOC,
                 0x03);

  USBD_LL_FlushEP(pdev, AUDIO_IN_EP);

  /* Allocate Audio structure */
  pdev->pClassData = USBD_malloc(sizeof (USBD_AUDIO_HandleTypeDef));

  if(pdev->pClassData == NULL)
    {
      return USBD_FAIL;
    }
  else
    {

      /* Prepare Out endpoint to receive 1st packet */
      if(!currentBuffer)
        {
          currentBuffer = bAlloc(USB_BUF_SIZE);
        }

      USBD_LL_PrepareReceive(pdev,
                             AUDIO_OUT_EP,
                             xferBuffer,
                             AUDIO_OUT_PACKET);
    }
  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_Init
 *         DeInitialize the AUDIO layer
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t  USBD_AUDIO_DeInit (USBD_HandleTypeDef *pdev,
                                   uint8_t cfgidx)
{

  USBD_LL_FlushEP(pdev, AUDIO_IN_EP);
  USBD_LL_FlushEP(pdev, AUDIO_OUT_EP);

  /* Open EP IN */
  USBD_LL_CloseEP(pdev,
                  AUDIO_IN_EP);
  /* Open EP OUT */
  USBD_LL_CloseEP(pdev,
                  AUDIO_OUT_EP);

  /* DeInit  physical Interface components */
  if(pdev->pClassData != NULL)
    {
      USBD_free(pdev->pClassData);
      pdev->pClassData = NULL;
    }

  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_Setup
 *         Handle the AUDIO specific requests
 * @param  pdev: instance
 * @param  req: usb requests
 * @retval status
 */
static uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev,
                                  USBD_SetupReqTypedef *req)
{
  uint16_t len;
  uint8_t *pbuf;
  uint8_t ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_CLASS :
      switch (req->bRequest)
        {
        case AUDIO_REQ_CUR:
          
          if(req->bmRequest & 0x80)
          {
            AUDIO_REQ_GetCurrent(pdev, req);
          }
          else
          {
            AUDIO_REQ_SetCurrent(pdev, req);
          }
          
          break;
          
        case AUDIO_REQ_RANGE:
          
          if(req->bmRequest & 0x80)
          {
            AUDIO_REQ_GetRange(pdev, req);
          }
          else
          {
            AUDIO_REQ_SetRange(pdev, req);
          }
          
          break;
          

        default:
          USBD_CtlError (pdev, req);
          ret = USBD_FAIL;
        }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
        {
        case USB_REQ_GET_DESCRIPTOR:
          if( (req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
            {
              pbuf = USBD_AUDIO_CfgDesc + 18;
              len = MIN(USB_AUDIO_DESC_SIZ , req->wLength);


              USBD_CtlSendData (pdev,
                                pbuf,
                                len);
            }
          break;

        case USB_REQ_GET_INTERFACE :
          USBD_CtlSendData (pdev,
                            (uint8_t *)&(usbd_audio_AltSet),
                            1);
          break;

        case USB_REQ_SET_INTERFACE :
          if ((uint8_t)(req->wValue) < ALTERNATE_SETTINGS)
            {
              usbd_audio_AltSet   = (uint8_t)(req->wValue);
              alignReset = 1;
              if(usbd_audio_AltSet > 0)
                {
                  formatChange(INPUT_USB1, usbFmt[usbd_audio_AltSet].format, usbFmt[usbd_audio_AltSet].rate);
                  formatChange(INPUT_USB2, usbFmt[usbd_audio_AltSet].format, usbFmt[usbd_audio_AltSet].rate);
                }
            }
          else
            {
              /* Call the error management function (command will be nacked */
              USBD_CtlError (pdev, req);
            }
          break;

        default:
          USBD_CtlError (pdev, req);
          ret = USBD_FAIL;
        }
    }
  return ret;
}


/**
 * @brief  USBD_AUDIO_GetCfgDesc
 *         return configuration descriptor
 * @param  speed : current device speed
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t  *USBD_AUDIO_GetCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_AUDIO_CfgDesc);
  return USBD_AUDIO_CfgDesc;
}

/**
 * @brief  USBD_AUDIO_DataIn
 *         handle data IN Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev,
                                   uint8_t epnum)
{
  flag = 0;
  usb_stats[0]++;
  /* Only OUT data are processed */
  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_EP0_RxReady
 *         handle EP0 Rx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t  USBD_AUDIO_EP0_RxReady (USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR)
    {
      /* In this driver, to simplify code, only SET_CUR request is managed */

      if (haudio->control.unit == AUDIO_OUT_STREAMING_CTRL)
        {
          haudio->control.cmd = 0;
          haudio->control.len = 0;
        }
    }

  return USBD_OK;
}
/**
 * @brief  USBD_AUDIO_EP0_TxReady
 *         handle EP0 TRx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t  USBD_AUDIO_EP0_TxReady (USBD_HandleTypeDef *pdev)
{
  /* Only OUT control data are processed */
  return USBD_OK;
}
/**
 * @brief  USBD_AUDIO_SOF
 *         handle SOF event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t  USBD_AUDIO_SOF (USBD_HandleTypeDef *pdev)
{
  PCD_HandleTypeDef *hpcd = pdev->pData;

  USB_OTG_GlobalTypeDef *USBx = hpcd->Instance;

  usb_stats[4]++;

  if (usbd_audio_AltSet !=0)
    {
      int fdbkNum = (inQueueUSB1.count < 5 || inQueueUSB2.count < 5)?1:0;
      feedback_data = usbFmt[usbd_audio_AltSet].feedback[fdbkNum];
      usb_stats[5] = feedback_data;
      usb_stats[7] = usbd_audio_AltSet;

      static int cnt = 0;
      cnt++;
      if(cnt == 1000)
	{
	  usb_stats[6] = total/usbFmt[usbd_audio_AltSet].bytesPerSample;
	  total = 0;
	  cnt=0;
	}
      
      if (flag == 0)
        {
          if ((USBx_DEVICE->DSTS & (USB_OTG_DSTS_FNSOF) & (1 << 8))== frame_number)
            {

              USBD_LL_Transmit(pdev, AUDIO_IN_EP, (uint8_t *) &feedback_data, 3);
              flag = 1;
            }
        };
    }
  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_IsoINIncomplete
 *         handle data ISO IN Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t  USBD_AUDIO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  PCD_HandleTypeDef *hpcd = pdev->pData;

  USB_OTG_GlobalTypeDef *USBx = hpcd->Instance;

  frame_number = ( USBx_DEVICE->DSTS & (USB_OTG_DSTS_FNSOF) & (1 << 8));

  if (flag)
    {
	  usb_stats[2]++;
      flag = 0;
      USBD_LL_FlushEP(pdev, AUDIO_IN_EP);
    };

  return USBD_OK;

}
/**
 * @brief  USBD_AUDIO_IsoOutIncomplete
 *         handle data ISO OUT Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t  USBD_AUDIO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  usb_stats[3]++;
  return USBD_OK;
}
/**
 * @brief  USBD_AUDIO_DataOut
 *         handle data OUT Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev,
                                    uint8_t epnum)
{
  if (epnum == AUDIO_OUT_EP)
    {

      usb_stats[1]++;
      PCD_HandleTypeDef *hpcd = pdev->pData;
      USB_OTG_EPTypeDef *ep = &hpcd->OUT_ep[epnum & 0x7FU];
      int copySize = ep->xfer_count;
      //Alloc a new buffer
      if(!currentBuffer)
	currentBuffer = bAlloc(USB_BUF_SIZE);

      if(currentBuffer)
	{
	  if(alignReset)
	    {
	      alignReset = 0;
	      currentBuffer->size = 0;
	    }

	  //Get copy length as min of remaning space in buffer and available data
	  copySize  = GET_MIN(USB_BUF_SIZE-currentBuffer->size, ep->xfer_count);
	  //Make sure that dst buffer is a multiple of alignment by removing alignment rest
	  copySize -= GET_ALIGN_REST(copySize + currentBuffer->size);
	  //Copy data to buffer
	  memcpy(&currentBuffer->data[currentBuffer->size], xferBuffer, copySize);
	  //Update size
	  currentBuffer->size += copySize;
 	  total+=currentBuffer->size;
	  //Send for processing. If failed reuse the buffer
	  bBuffer_t *b1 = bCopy(currentBuffer);
	  bBuffer_t *b2 = bCopy(currentBuffer);
	  if(bEnqueue(&inQueueUSB1, b1) != 0) bFree(b1);
	  if(bEnqueue(&inQueueUSB2, b2) != 0) bFree(b2);
	  
	}

      //Alloc a new buffer if necessary
      if(!currentBuffer)
	currentBuffer = bAlloc(USB_BUF_SIZE);

      currentBuffer->size = 0;
      if(currentBuffer && ep->xfer_count > copySize)
	{
	  //move rest of data to a new buffer
	  currentBuffer->size = ep->xfer_count - copySize;
	  memcpy(currentBuffer->data, &xferBuffer[copySize], currentBuffer->size);
	}
	
      USBD_LL_PrepareReceive(pdev,
			     AUDIO_OUT_EP,
			     xferBuffer,
			     usbFmt[usbd_audio_AltSet].rxSize);

    }

  return USBD_OK;
}

/**
 * @brief  AUDIO_Req_GetCurrent
 *         Handles the GET_CUR Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
  uint8_t buff[5];
  if(req->wValue == USB_CS_CLOCK_VALID_CONTROL)
  {
    buff[0] = 1;
    USBD_CtlSendData (pdev,
                    buff,
                    req->wLength);
  }
  else if(req->wValue == USB_CS_SAM_FREQ_CONTROL)
  {

    freq[0] = (uint8_t)(usbFmt[usbd_audio_AltSet].rate >> 0);
    freq[1] = (uint8_t)(usbFmt[usbd_audio_AltSet].rate >> 8);
    freq[2] = (uint8_t)(usbFmt[usbd_audio_AltSet].rate >> 16);
    freq[3] = (uint8_t)(usbFmt[usbd_audio_AltSet].rate >> 24);
    freq[4] = (uint8_t)(usbFmt[usbd_audio_AltSet].rate >> 0);
    freq[5] = (uint8_t)(usbFmt[usbd_audio_AltSet].rate >> 8);
    freq[6] = (uint8_t)(usbFmt[usbd_audio_AltSet].rate >> 16);
    freq[7] = (uint8_t)(usbFmt[usbd_audio_AltSet].rate >> 24);
    freq[8] = 0x00;
    freq[9] = 0x00;
    freq[10] = 0x00;
    freq[11] = 0x00;
    
    USBD_CtlSendData (pdev,
                    freq,
                    req->wLength);
  }
  else
  {
    memset(haudio->control.data, 0, 64);
    /* Send the current mute state */
    USBD_CtlSendData (pdev,
                    haudio->control.data,
                    req->wLength);
  }

}

/**
 * @brief  AUDIO_Req_GetRange
 *         Handles the GET_RANGE

 Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetRange(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
//  int i = 0;

  if(req->wValue == USB_CS_SAM_FREQ_CONTROL)
  {
    freq[0] = 0x01;
    freq[1] = 0x00;
    
    freq[2] = (uint8_t)(usbFmt[usbd_audio_AltSet+1].rate >> 0);
    freq[3] = (uint8_t)(usbFmt[usbd_audio_AltSet+1].rate >> 8);
    freq[4] = (uint8_t)(usbFmt[usbd_audio_AltSet+1].rate >> 16);
    freq[5] = (uint8_t)(usbFmt[usbd_audio_AltSet+1].rate >> 24);
    freq[6] = (uint8_t)(usbFmt[usbd_audio_AltSet+1].rate >> 0);
    freq[7] = (uint8_t)(usbFmt[usbd_audio_AltSet+1].rate >> 8);
    freq[8] = (uint8_t)(usbFmt[usbd_audio_AltSet+1].rate >> 16);
    freq[9] = (uint8_t)(usbFmt[usbd_audio_AltSet+1].rate >> 24);
    freq[10] = 0x00;
    freq[11] = 0x00;
    freq[12] = 0x00;
    freq[13] = 0x00;
    
    /* Send the current mute state */
    USBD_CtlSendData (pdev,
                    freq,
                    req->wLength);
  }
  
}

/**
 * @brief  AUDIO_Req_SetCurrent
 *         Handles the SET_CUR Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{

  if (req->wLength)
    {
      if(req->wValue == USB_CS_SAM_FREQ_CONTROL)
      {
        /* Prepare the reception of the buffer over EP0 */
        USBD_CtlPrepareRx (pdev,
                           freq2,
                           req->wLength);

      }
      
    }
}

/**
 * @brief  AUDIO_Req_SetRange
 *         Handles the SET_CUR Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_SetRange(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{

  if (req->wLength)
    {
      if(req->wValue == USB_CS_SAM_FREQ_CONTROL)
      {
        /* Prepare the reception of the buffer over EP0 */
        USBD_CtlPrepareRx (pdev,
                           freq,
                           req->wLength);
      }
    }
}


/**
 * @brief  DeviceQualifierDescriptor
 *         return Device Qualifier descriptor
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_AUDIO_DeviceQualifierDesc);
  return USBD_AUDIO_DeviceQualifierDesc;
}

/**
 * @}
 */


/**
 * @}
 */


/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
