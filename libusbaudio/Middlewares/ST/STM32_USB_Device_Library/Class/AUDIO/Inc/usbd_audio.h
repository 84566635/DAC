/**
  ******************************************************************************
  * @file    usbd_audio.h
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   header file for the usbd_audio.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_AUDIO_H
#define __USB_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

  /* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"

  /** @addtogroup STM32_USB_DEVICE_LIBRARY
    * @{
    */

  /** @defgroup USBD_AUDIO
    * @brief This file is the Header file for usbd_audio.c
    * @{
    */


  /** @defgroup USBD_AUDIO_Exported_Defines
    * @{
    */
#define SOF_RATE 0x05
//#define VALUE2(arg_value) 							  (((arg_value)>>0)&0xFF),(((arg_value)>>8)&0xFF)
#define AUDIO_OUT_PACKET                              (uint32_t)1000
//#define AUDIO_SAMPLE_FREQ(frq)      (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_OUT_EP                                  0x01
#define AUDIO_IN_EP                                  0x81
#define AUDIO_INTERFACE_DESC_SIZE                     9
#define USB_AUDIO_DESC_SIZ                            0x09
#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE             0x07
#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE            0x08

#define AUDIO_DESCRIPTOR_TYPE                         0x21
#define USB_DEVICE_CLASS_AUDIO                        0x01
#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01
#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02
#define AUDIO_PROTOCOL_UNDEFINED                      0x20
#define AUDIO_STREAMING_GENERAL                       0x01
#define AUDIO_STREAMING_FORMAT_TYPE                   0x02

  /* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25

  /* Audio Control Interface Descriptor Subtypes */
#define AUDIO_CONTROL_HEADER                          0x01
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06

#define AUDIO_INPUT_TERMINAL_DESC_SIZE                0x11
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               0x0C
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           0x07

#define AUDIO_CONTROL_MUTE                            0x00000000

#define AUDIO_FORMAT_TYPE_I                           0x01
#define AUDIO_FORMAT_TYPE_III                         0x03

#define AUDIO_ENDPOINT_GENERAL                        0x01

#define AUDIO_REQ_CUR                            	  0x01
#define AUDIO_REQ_RANGE                          	  0x02
#define AUDIO_REQ_GET_CUR                             0x01
#define AUDIO_REQ_GET_RANGE                           0x02
#define AUDIO_REQ_SET_CUR                             0x81
#define AUDIO_REQ_SET_RANGE                           0x82

#define AUDIO_OUT_STREAMING_CTRL                      0x02

#define USB_CS_SAM_FREQ_CONTROL						  0x0100
#define USB_CS_CLOCK_VALID_CONTROL					  0x0200



#define AUDIO_DEFAULT_VOLUME                          70

  /** @defgroup USBD_CORE_Exported_TypesDefinitions
    * @{
    */
  typedef struct
  {
    uint8_t cmd;
    uint8_t data[USB_MAX_EP0_SIZE];
    uint8_t len;
    uint8_t unit;
  }
  USBD_AUDIO_ControlTypeDef;



  typedef struct
  {
    //    __IO uint32_t             alt_setting;
    USBD_AUDIO_ControlTypeDef control;
  }
  USBD_AUDIO_HandleTypeDef;


  /**
    * @}
    */



  /** @defgroup USBD_CORE_Exported_Macros
    * @{
    */

  /**
    * @}
    */

  /** @defgroup USBD_CORE_Exported_Variables
    * @{
    */

  extern USBD_ClassTypeDef  USBD_AUDIO;
#define USBD_AUDIO_CLASS    &USBD_AUDIO
  /**
    * @}
    */


#ifdef __cplusplus
}
#endif

#endif  /* __USB_AUDIO_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
