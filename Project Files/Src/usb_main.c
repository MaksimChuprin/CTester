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
extern 	sysCfg_t				systemConfig;

const 	char * helpStrings[] = {

		"Start - start testing process.\r\n",
		"Stop - terminate testing process.\r\n",
		"Measure - take the measurement immediately and show a result.\r\n",
		"Read Status - display the current state of the system.\r\n",
		"Read Results - display measurement results.\r\n",
		"Read Settings - display settings values.\r\n",
		"Set Tt=<value> - enter <value> of test time in Hours.\r\n",
		"Set Mp=<value> - enter <value> of measure period in Minutes.\r\n",
		"Set Vt=<value> - enter <value> of test voltage in Volts.\r\n",
		"Set Vm=<value> - enter <value> of measure voltage in Volts.\r\n",
		"Set Ki=<value> - enter coefficient of current amplifier.\r\n",
		"Set Kd=<value> - enter division coefficient of HV.\r\n",
		"Set Td=<value> - enter <value> of discharge time in mSec.\r\n",
		"Set RTC=<YYYY:MM:DD:HH:MM> - enter current time.\r\n",
		"Echo On - switch on echo of entered command.\r\n",
		"Echo Off - switch off echo of entered command.\r\n",
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
		SEND_CDC_MESSAGE( "\r\n********************************************\r\n" );
		SEND_CDC_MESSAGE( "Start Capacitor Testing System\r\n SW version: 0.0.1\r\n" );
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured!" );
								break;
		case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready to test." );
								break;
		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: testing in progress..." );
								break;
		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: testing finished, data available." );
								break;
		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail!" );
								break;
		}

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

	//--------------------------------------------- START
	if( strstr( ptrmessage, "START" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured! Set configuration first." );
								break;

		case READY_STATUS:		SEND_CDC_MESSAGE( "Testing process starting:" );
								sprintf( usb_message, "Testing voltage: %lu Volts, Measure voltage: %lu Volts\r\nTesting time: %lu Hours, Measure period: %lu Minutes\r\n",
																systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
								SEND_CDC_MESSAGE( usb_message );
								SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ACTIVE_STATUS );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: testing already in progress!" );
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: testing finished, read data first." );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail, testing unavailable!" );
								break;
		}

		return;
	}

	//--------------------------------------------- STOP
	if( strstr( ptrmessage, "STOP" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured! Set configuration first." );
								break;

		case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready, nothing to stop." );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: testing terminated!" );
								if(systemConfig.measureSavedPoints)
								{
									sprintf( usb_message, "There are data to read: %lu Point(s)\r\n", systemConfig.measureSavedPoints );
									SEND_CDC_MESSAGE( usb_message );
									SAVE_SYSTEM_CNF( &systemConfig.sysStatus, FINISH_STATUS );
								}
								else
								{
									SEND_CDC_MESSAGE( "There are no data to read." );
									SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
								}
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: testing finished, read data first." );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail, testing unavailable!" );
								break;
		}

		return;
	}

	//--------------------------------------------- MEASURE
	if( strstr( ptrmessage, "MEASURE" ) )
	{

		return;
	}

	//--------------------------------------------- READ STATUS
	if( strstr( ptrmessage, "READ STATUS" ) )
	{

		return;
	}

	//--------------------------------------------- READ RTESULTS
	if( strstr( ptrmessage, "READ RESULTS" ) )
	{

		return;
	}

	//--------------------------------------------- READ SETTINGS
	if( strstr( ptrmessage, "READ SETTINGS" ) )
	{

		return;
	}

	//--------------------------------------------- SET Test Time
	ptr = strstr( ptrmessage, "Set TT=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------	SET Measure Period
	ptr = strstr( ptrmessage, "Set MP=" );
	if( ptr )
	{

		return;
	}

	//--------------------------------------------- Set Test Voltage
	ptr = strstr( ptrmessage, "Set VT=" );
	if( ptr )
	{

		return;
	}

	//---------------------------------------------	Set Measure Voltage
	ptr = strstr( ptrmessage, "Set VM=" );
	if( ptr )
	{

		return;
	}

	//--------------------------------------------- SET Current Amplifier Ki
	ptr = strstr( ptrmessage, "SET KI=" );
	if( ptr )
	{

		return;
	}

	//--------------------------------------------- SET High Voltage divider Kd
	ptr = strstr( ptrmessage, "SET KD=" );
	if( ptr )
	{

		return;
	}

	//--------------------------------------------- SET Time of Discharge Capasitors Td
	ptr = strstr( ptrmessage, "SET TD=" );
	if( ptr )
	{

		return;
	}

	//--------------------------------------------- SET Real Time Counter
	ptr = strstr( ptrmessage, "SET RTC=" );
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
