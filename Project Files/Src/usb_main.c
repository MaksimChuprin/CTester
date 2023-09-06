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
#include "main.h"
#include "cmsis_os.h"

extern USBD_HandleTypeDef  	USBD_Device;
extern TaskHandle_t			USBThreadHandle;

char * helpStrings[] = {
		{ "START - start testing process\r\n" },
		{ "STOP - terminate testing process\r\n" },
		{ "TEST VOLTAGE=<value> - enter <value> of test voltage in Volts\r\n" },
		{ "MEASURE VOLTAGE=<value> - enter <value> of measure voltage in Volts\r\n" },
		{ "TEST TIME=<value> - enter <value> of test time in Hours\r\n" },
		{ "MEASURE PERIOD=<value> - enter <value> of measure period in Minutes\r\n" },
		{ "SET TIME=<YYYY:MM:DD:HH:MM> - enter current time\r\n" },
		{ "READ STATUS - display the current state of the system\r\n" },
		{ "READ RESULTS - display measurement results\r\n" },
		{ "READ SETTINGS - display settings values\r\n" },
		{ "MEASURE - take the measurement immediately and show a result\r\n" },
		{ "HELP - list available commands\r\n" },
		0
};

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
		for(; sendCDCmessage("Capacitor Testing System\r\n SW version: 0.0.1\r\n"); osDelay(1000) );
		// do connection
		for(;;)
		{
			// wait message
			uint32_t event = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(500) );

			if(event)
			{
				getCDCmessage( usb_message );
				messageDecode( usb_message );
			}
			else
			if( !isCableConnected() )
			{
				USBD_DeInit(&USBD_Device);
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

/**
  * @brief  USB CDC message Decoder
  * @param  argument: pointer to message string
  * @retval
  */
static void messageDecode( char * ptrmessage)
{
	char * ptr;

	//---------------------------------------------
	if( strstr( ptrmessage, "START" ) )
	{

		return;
	}

	//---------------------------------------------
	if( strstr( ptrmessage, "STOP" ) )
	{

		return;
	}

	//---------------------------------------------
	if( strstr( ptrmessage, "MEASURE" ) )
	{

		return;
	}

	//---------------------------------------------
	ptr = strstr( ptrmessage, "TEST VOLTAGE=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------
	ptr = strstr( ptrmessage, "MEASURE VOLTAGE=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------
	ptr = strstr( ptrmessage, "TEST TIME=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------
	ptr = strstr( ptrmessage, "MEASURE PERIOD=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------
	ptr = strstr( ptrmessage, "SET TIME=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------
	if( strstr( ptrmessage, "READ STATUS" ) )
	{

		return;
	}

	//---------------------------------------------
	if( strstr( ptrmessage, "READ RESULTS" ) )
	{

		return;
	}

	//---------------------------------------------
	if( strstr( ptrmessage, "READ SETTINGS" ) )
	{

		return;
	}


	//---------------------------------------------
	if( strstr( ptrmessage, "HELP" ) )
	{
		for(uint32_t i = 0; ; i++)
		{
			if( helpString[i] )
			{
				strcpy( usb_message, helpString[i] );
				while( sendCDCmessage(usb_message) ) osDelay(50);
			}
			else
				break;
		}
		return;
	}

	//---------------------------------------------
	sendCDCmessage( "Unknown command - enter HELP command\r\n" );
}
