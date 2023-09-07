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
extern 	sysCfg_t				systemConfigEprom;
extern 	__IO sysCfg_t				systemConfig;

const 	char * helpStrings[] = {

		"Start - start testing process\r\n",
		"Stop - terminate testing process\r\n",
		"Measure - take the measurement immediately and show a result\r\n",
		"Read Status - display the current state of the system\r\n",
		"Read Data - display measurement results\r\n",
		"Read Settings - display settings values\r\n",
		"Set Tt=<value> - enter <value> of test time in Hours\r\n",
		"Set Mp=<value> - enter <value> of measure period in Minutes\r\n",
		"Set Vt=<value> - enter <value> of test voltage in Volts\r\n",
		"Set Vm=<value> - enter <value> of measure voltage in Volts\r\n",
		"Set Ki=<value> - enter factor of current amplifier\r\n",
		"Set Kd=<value> - enter division factor of HV\r\n",
		"Set Td=<value> - enter <value> of discharge time in mSec\r\n",
		"Set RTC=<YYYY:MM:DD:HH:MM> - enter current time\r\n",
		"Echo On - switch on echo of entered command\r\n",
		"Echo Off - switch off echo of entered command\r\n",
		"Help - list available commands\r\n",
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
		SEND_CDC_MESSAGE( "Start Capacitor Testing System\r\nSW version: 0.0.1\r\n" );

		switch(systemConfig.sysStatus)
		{
		case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready\r\n\r\n" );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: test in progress\r\n\r\n" );
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: test finished\r\n\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail!\r\n\r\n" );
								break;

		default:				SAVE_SYSTEM_CNF( &systemConfig.sysStatus, NO_CONFIG_STATUS );

		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured!\r\n\r\n" );
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
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured - set configuration first!\r\n\r\n" );
								break;

		case READY_STATUS:		SEND_CDC_MESSAGE( "Test process start\r\n" );
								sprintf( usb_message, "Test voltage: %lu Volts, Measure voltage: %lu Volts\r\nTesting time: %lu Hours, Measure period: %lu Minutes\r\n\r\n",
																systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
								SEND_CDC_MESSAGE( usb_message );
								SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ACTIVE_STATUS );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: test already started!\r\n\r\n" );
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: test finished, read data first\r\n\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail, test unavailable!\r\n\r\n" );
								break;
		}

		return;
	}

	//--------------------------------------------- STOP
	if( strstr( ptrmessage, "STOP" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured - set configuration first\r\n\r\n" );
								break;

		case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready, nothing to stop\r\n\r\n" );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: testing terminated!" );
								if(systemConfig.measureSavedPoints)
								{
									sprintf( usb_message, "There are data to read: %lu Point(s)\r\n\r\n", systemConfig.measureSavedPoints );
									SEND_CDC_MESSAGE( usb_message );
									SAVE_SYSTEM_CNF( &systemConfig.sysStatus, FINISH_STATUS );
								}
								else
								{
									SEND_CDC_MESSAGE( "There are no data to read.\r\n\r\n" );
									SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
								}
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: testing finished, read data first\r\n\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail, testing unavailable!\r\n\r\n" );
								break;
		}

		return;
	}

	//--------------------------------------------- MEASURE
	if( strstr( ptrmessage, "MEASURE" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured - set configuration first\r\n\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail, testing unavailable!\r\n\r\n" );
								break;
		}
		return;
	}

	//--------------------------------------------- READ STATUS
	if( strstr( ptrmessage, "READ STATUS" ) )
	{
		sprintf( usb_message, "Testing voltage: %lu Volts, Measure voltage: %lu Volts\r\nTesting time: %lu Hours, Measure period: %lu Minutes\r\n\r\n",
								systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);

		switch(systemConfig.sysStatus)
		{
		case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready to test\r\n" );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: test started\r\n" );
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: test finished\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail\r\n" );
								break;

		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured\r\n" );
								break;
		}

		SEND_CDC_MESSAGE( usb_message );
		return;
	}

	//--------------------------------------------- READ RTESULTS
	if( strstr( ptrmessage, "READ DATA" ) )
	{
		switch(systemConfig.sysStatus)
		{

		case FINISH_STATUS:		SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
		case READY_STATUS:
		case ACTIVE_STATUS:
								if(systemConfig.measureSavedPoints)
								{
									sprintf( usb_message, "There are %lu Point(s)\r\n", systemConfig.measureSavedPoints );
									SEND_CDC_MESSAGE( usb_message );
// TODO
								}
								else
								{
									SEND_CDC_MESSAGE( "There are no data to read.\r\n\r\n" );
								}
								break;


		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail\r\n\r\n" );
								break;

		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured\r\n\r\n" );
								break;
		}

		return;
	}

	//--------------------------------------------- READ SETTINGS
	if( strstr( ptrmessage, "READ SETTINGS" ) )
	{
		SEND_CDC_MESSAGE( "System settings:\r\n" );
		sprintf( usb_message, "Testing voltage: %lu Volts, Measure voltage: %lu Volts\r\nTesting time: %lu Hours, Measure period: %lu Minutes\r\n",
										systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
		SEND_CDC_MESSAGE( usb_message );
		sprintf( usb_message, "Amplifier factor Ki: %lu, Division factor Kd: %lu, Discharge time: %lu mSec\r\n\r\n",
							systemConfig.kiAmplifire, systemConfig.kdDivider, systemConfig.dischargePreMeasureTimeMs);
		SEND_CDC_MESSAGE( usb_message );
		return;
	}

	//--------------------------------------------- SET Test Time
	ptr = strstr( ptrmessage, "SET TT=" );
	if( ptr )
	{
		uint32_t 	testTime 	= 0;
		int 		res			= sscanf( ptr, "SET TT=%lu", &testTime );

		if( res )
		{
			if( testTime > 7*24 ) SEND_CDC_MESSAGE( "Testing time must be less then 168 hours\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.testingTimeSec, testTime * 3600 );

				if(CheckSysCnf())
				{
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
				}

				SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
			}
		}
		else
		{
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );
		}

		return;
	}

	//---------------------------------------------	SET Measure Period
	ptr = strstr( ptrmessage, "SET MP=" );
	if( ptr )
	{
		uint32_t 	measPeriod 	= 0;
		int 		res			= sscanf( ptr, "SET MP=%lu", &measPeriod );

		if( res )
		{
			if( measPeriod < 30 ) SEND_CDC_MESSAGE( "Measuring period must be more then 30 minutes\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.measuringPeriodSec, measPeriod * 60 );

				if(CheckSysCnf())
				{
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
				}
				SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
			}
		}
		else
		{
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );
		}
		return;
	}

	//--------------------------------------------- Set Test Voltage
	ptr = strstr( ptrmessage, "SET VT=" );
	if( ptr )
	{
		uint32_t 	testVol 	= 0;
		int 		res			= sscanf( ptr, "SET VT=%lu", &testVol );

		if( res )
		{
			if( testVol > 280 ) SEND_CDC_MESSAGE( "Test voltage  must be less then 280 Volts\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.uTestVol, testVol );

				if(CheckSysCnf())
				{
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
				}
				SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
			}
		}
		else
		{
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );
		}
		return;
	}

	//---------------------------------------------	Set Measure Voltage
	ptr = strstr( ptrmessage, "SET VM=" );
	if( ptr )
	{
		uint32_t 	measVol 	= 0;
		int 		res			= sscanf( ptr, "SET VM=%lu", &measVol );

		if( res )
		{
			if( measVol > 280 ) SEND_CDC_MESSAGE( "Measure voltage  must be less then 280 Volts\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.uMeasureVol, measVol );

				if(CheckSysCnf())
				{
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
				}
				SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
			}
		}
		else
		{
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );
		}
		return;
	}

	//--------------------------------------------- SET Current Amplifier Ki
	ptr = strstr( ptrmessage, "SET KI=" );
	if( ptr )
	{
		uint32_t 	Ki 	= 0;
		int 		res	= sscanf( ptr, "SET KI=%lu", &Ki );

		if( res )
		{
			if( Ki > 10000000 || Ki < 100 ) SEND_CDC_MESSAGE( "Current amplifier factor must be 100...10000000\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.kiAmplifire, Ki );

				if(CheckSysCnf())
				{
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
				}
				SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
			}
		}
		else
		{
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );
		}
		return;
	}

	//--------------------------------------------- SET High Voltage divider Kd
	ptr = strstr( ptrmessage, "SET KD=" );
	if( ptr )
	{
		uint32_t 	Kd 	= 0;
		int 		res			= sscanf( ptr, "SET KD=%lu", &Kd );

		if( res )
		{
			if( Kd > 200 || Kd < 10 ) SEND_CDC_MESSAGE( "HV divider factor must be 10...200\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.kdDivider, Kd );

				if(CheckSysCnf())
				{
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
				}
				SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
			}
		}
		else
		{
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );
		}
		return;
	}

	//--------------------------------------------- SET Time of Discharge Capacitors Td
	ptr = strstr( ptrmessage, "SET TD=" );
	if( ptr )
	{
		uint32_t 	Td 	= 0;
		int 		res			= sscanf( ptr, "SET TD=%lu", &Td );

		if( res )
		{
			if( Td > 10000 || Td < 10 ) SEND_CDC_MESSAGE( "Time of Discharge must be 10...10000 mSec\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.dischargePreMeasureTimeMs, Td );

				if(CheckSysCnf())
				{
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
				}
				SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
			}
		}
		else
		{
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );
		}
		return;
	}

	//--------------------------------------------- SET Real Time Counter
	ptr = strstr( ptrmessage, "SET RTC=" );
	if( ptr )
	{
		DateTime_t  date = { 0 };
		int 		res	 = sscanf( ptr, "SET RTC=%4hu:%2hu:%2hu:%2hu:%2hu", &date.year, &date.month, &date.day, &date.hours, &date.minutes );

		if( res == 5 )
		{

			if(CheckSysCnf())
			{
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
			}
			SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
		}
		else
		{
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );
		}
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
