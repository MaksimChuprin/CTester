/**
  ******************************************************************************
  * @file    usb_main.c
  * @author  Epta
  * @brief   USB device CDC application main file
  ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

extern 	USBD_HandleTypeDef  	USBD_Device;
extern 	TaskHandle_t			USBThreadHandle;

const 	char * helpStrings[] = {

		"Start - start testing process.\r\n",
		"Stop - terminate testing process.\r\n",
		"Test Voltage=<value> - enter <value> of test voltage in Volts.\r\n",
		"Measure Voltage=<value> - enter <value> of measure voltage in Volts.\r\n",
		"Test Time=<value> - enter <value> of test time in Hours.\r\n",
		"Measure Period=<value> - enter <value> of measure period in Minutes.\r\n",
		"Set Time=<YYYY:MM:DD:HH:MM> - enter current time.\r\n",
		"Measure - take the measurement immediately and show a result.\r\n",
		"Read Status - display the current state of the system.\r\n",
		"Read Results - display measurement results.\r\n",
		"Read Settings - display settings values.\r\n",
		"Echo <On>/<Off> - switch echo entered command.\r\n",
		"Set Ki Ampl=<value> - enter coefficient of current amplifier.\r\n",
		"Set Kd HV=<value> - enter division coefficient of HV.\r\n",
		"Help - list available commands.\r\n",
		0
};

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define SEND_CDC_MESSAGE(MSG)	{ while( sendCDCmessage( (MSG) ) && isCableConnected() ) osDelay(50); }
/* Private variables ---------------------------------------------------------*/
static char 				usb_message[APP_CDC_DATA_SIZE];
/* Private function prototypes -----------------------------------------------*/
static bool 				isCableConnected	( void );
static void 				messageDecode		( char * ptrmessage );
static void 				UpperCase			( char * ptrmessage );
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
		SEND_CDC_MESSAGE( "Capacitor Testing System\r\n SW version: 0.0.1\r\n" );
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
  * @brief  convert message to upper case
  * @param  argument: Not used
  * @retval
  */
static void UpperCase( char * ptrmessage )
{
	size_t len = strlen(ptrmessage);

	for( size_t i = 0; i < len; i++ )
		if( 'a' <= ptrmessage[i] && ptrmessage[i] <= 'z' ) ptrmessage[i] -= 0x20;
}


/**
  * @brief  USB CDC message Decoder
  * @param  argument: pointer to message string
  * @retval
  */
static void messageDecode( char * ptrmessage )
{
	static bool echoOFF = false;
	char * ptr;

	if( echoOFF == false ) while( sendCDCmessage(ptrmessage) ) osDelay(50);

	UpperCase(ptrmessage);

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
	ptr = strstr( ptrmessage, "SET KI AMPL=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------
	ptr = strstr( ptrmessage, "SET KD HV=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------
	ptr = strstr( ptrmessage, "ECHO " );
	if( ptr )
	{
		if( strstr( ptr, "ON" ) )
		{
			sendCDCmessage("Echo switched ON\r\n");
			echoOFF = false;
			return;
		}

		if( strstr( ptr, "OFF" ) )
		{
			sendCDCmessage("Echo switched OFF\r\n");
			echoOFF = true;
			return;
		}
	}

	//---------------------------------------------
	if( strstr( ptrmessage, "HELP" ) )
	{
		for(uint32_t i = 0; ; i++)
		{
			if( helpStrings[i] )
			{
				strcpy( usb_message, helpStrings[i] );
				while( sendCDCmessage(usb_message) ) osDelay(50);
			}
			else
				break;
		}
		return;
	}

	//---------------------------------------------
	sendCDCmessage( "Unknown command - enter HELP!\r\n" );
}
