/**
  ******************************************************************************
  * @file    USB_Device/CDC_Standalone/Src/usb_main.c
  * @author  MCD Application Team
  * @brief   USB device CDC application main file
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

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "main.h"
#include "cmsis_os.h"

extern USBD_HandleTypeDef  	USBD_Device;
extern TaskHandle_t			USBThreadHandle;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static char 				usb_message[APP_CDC_DATA_SIZE];
/* Private function prototypes -----------------------------------------------*/
static bool 				isCableConnected(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  UsbCDCThread
  * @param  argument: Not used
  * @retval None
  */
void UsbCDCThread(const void *argument)
{

	for(;; osDelay(500))
	{
		if( !isCableConnected() ) continue;

		/* Init Device Library */
		USBD_Init(&USBD_Device, &VCP_Desc, 0);
		/* Add Supported Class */
		USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
		/* Add CDC Interface Class */
		USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
		/* Start Device Process */
		USBD_Start(&USBD_Device);
		// wait for connect
		for(; sendCDCmessage("Capacitors Testing System\r\n SW version: 0.0.1\r\n"); osDelay(1000) );
		// do connection
		for(;;)
		{
			// wait message
			uint32_t event = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(500) );

			BSP_LED_Toggle(LED_BLUE);
			if(event)
			{
				getCDCmessage( usb_message );
				sendCDCmessage( usb_message );
			}
			else
			if( !isCableConnected() )
			{
				USBD_DeInit(&USBD_Device);
				BSP_LED_Off(LED_BLUE);
				break;
			}
		}
	}
}

/**
  * @brief  is USB Cable Connected
  * @param  argument: Not used
  * @retval
  */
static bool isCableConnected(void)
{
	return BSP_PAN_GetState();
}

