/**
  ******************************************************************************
  * @file    USB_Device/CDC_Standalone/Inc/usb_main.h
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_MAIN_H
#define __USB_MAIN_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define SEND_CDC_MESSAGE(MSG)	{ while( sendCDCmessage( (MSG) ) && isCableConnected() ) osDelay(50); }
/* Exported functions ------------------------------------------------------- */
void UsbCDCThread(const void *argument);
bool isCableConnected(void);

#endif /* __USB_MAIN_H */
