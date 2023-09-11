/**
  ******************************************************************************
  * @file    usb_main.c
  * @author  Epta
  * @brief   USB device CDC application main file
  ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "main.h"

extern 	USBD_HandleTypeDef  		USBD_Device;
extern 	osThreadId					USBThreadHandle;
extern 	osThreadId					MeasureThreadHandle;

extern 	__IO sysCfg_t				systemConfig;
extern  __IO dataAttribute_t		dataAttribute[];
extern 	__IO dataMeasure_t			dataMeasure[];

const char * helpStrings[] = {

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
static void 				messageDecode		( void );
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

		/* display sys info */
		SEND_CDC_MESSAGE( "\r\n********************************************\r\n" );
		SEND_CDC_MESSAGE( "Start Capacitor Testing System\r\nSW version: 0.0.1\r\n" );
		/*  time */
		{
			DateTime_t  date = {0};

			convertUnixTimeToDate( getRTC(), &date );
			sprintf( usb_message, "System time: %04hd-%02hd-%02hd %02hd:%02hd\r\n", date.year, date.month, date.day, date.hours, date.minutes );
			SEND_CDC_MESSAGE(usb_message);
		}
		/*  temperature */
		{
			int16_t t = getTemperature();

			if( t != SENSOR_NOT_CONNECTED )	sprintf( usb_message, "System temperature, oC: %3i\r\n", t);
			else							sprintf( usb_message, "System temperature: sensor not connected\r\n");
			SEND_CDC_MESSAGE(usb_message);
		}
		/* sys status */
		switch(systemConfig.sysStatus)
		{
		case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready\r\n" );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: test started\r\n" );
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: test finished\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail\r\n" );
								break;

		default:				SAVE_SYSTEM_CNF( &systemConfig.sysStatus, NO_CONFIG_STATUS );

		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured\r\n" );
								break;
		}

		SEND_CDC_MESSAGE( "********************************************\r\n\r\n" );

		// do connection
		for(;;)
		{
			// wait message
			osEvent event = osSignalWait( USB_THREAD_MESSAGEGOT_Evt, 100 );

			if( event.status == osEventSignal )
			{
				messageDecode();
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
static void messageDecode( void )
{
	static bool echoOFF = false;
	char * ptr;

	getCDCmessage( usb_message );

	if( echoOFF == false ) while( sendCDCmessage(usb_message) ) osDelay(50);

	UpperCase(usb_message);

	//--------------------------------------------- START
	if( strstr( usb_message, "START" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured - set configuration first!\r\n\r\n" );
								break;

		case READY_STATUS:		SEND_CDC_MESSAGE( "Starting..." );

								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTTEST_Evt );
								osSignalWait( USB_THREAD_TESTSTARTED_Evt | USB_THREAD_MEASUREERROR_Evt, osWaitForever );

								// TODO check signals

								SEND_CDC_MESSAGE( "Test process started\r\n" );
								sprintf( usb_message, "Test voltage: %lu Volts, Measure voltage: %lu Volts\r\nTest time: %lu Hours, Measure period: %lu Minutes\r\n\r\n",
																systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
								SEND_CDC_MESSAGE( usb_message );
								SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ACTIVE_STATUS );

								if(systemConfig.measureSavedPoints) SAVE_SYSTEM_CNF( &systemConfig.measureSavedPoints, 0 );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: test already started\r\n\r\n" );
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: test finished, read data first\r\n\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail, test unavailable\r\n\r\n" );
								break;
		}

		return;
	}

	//--------------------------------------------- STOP
	if( strstr( usb_message, "STOP" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured - set configuration first\r\n\r\n" );
								break;

		case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready, nothing to stop\r\n\r\n" );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Terminating..." );

								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_TESTFINISH_Evt );
								osSignalWait( USB_THREAD_TESTSTOPPED_Evt | USB_THREAD_MEASUREERROR_Evt, osWaitForever );

								// TODO check signals

								SEND_CDC_MESSAGE( "Test terminated" );

								if(systemConfig.measureSavedPoints)
								{
									SAVE_SYSTEM_CNF( &systemConfig.sysStatus, FINISH_STATUS );
								}
								else
								{
									SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
								}
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: test finished, read data first\r\n\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail, test unavailable\r\n\r\n" );
								break;
		}

		return;
	}

	//--------------------------------------------- MEASURE
	if( strstr( usb_message, "MEASURE" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured - set configuration first\r\n\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail, measure unavailable\r\n\r\n" );
								break;

		case READY_STATUS:
		case ACTIVE_STATUS:
		case FINISH_STATUS:		osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTMESURE_Evt );
								osSignalWait( USB_THREAD_MEASUREREADY_Evt | USB_THREAD_MEASUREERROR_Evt, osWaitForever );

								// TODO check signals
		}
		return;
	}

	//--------------------------------------------- READ STATUS
	if( strstr( usb_message, "READ STATUS" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready\r\n" );
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

		DateTime_t  date 	= {0};
		int16_t 	t 		= getTemperature();

		convertUnixTimeToDate( getRTC(), &date );
		sprintf( usb_message, "System time: %04hd-%02hd-%02hd %02hd:%02hd\r\n", date.year, date.month, date.day, date.hours, date.minutes );
		SEND_CDC_MESSAGE(usb_message);

		if( t != SENSOR_NOT_CONNECTED )	sprintf( usb_message, "System temperature, oC: %3i\r\n", t);
		else							sprintf( usb_message, "System temperature: sensor not connected\r\n");
		SEND_CDC_MESSAGE(usb_message);

		sprintf( usb_message, "Testing voltage: %lu Volts, Measure voltage: %lu Volts\r\nTesting time: %lu Hours, Measure period: %lu Minutes\r\nSaved measured data point(s): %3lu\r\n\r\n",
										systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60, systemConfig.measureSavedPoints );
		SEND_CDC_MESSAGE( usb_message );
		return;
	}

	//--------------------------------------------- READ RTESULTS
	if( strstr( usb_message, "READ DATA" ) )
	{
		switch(systemConfig.sysStatus)
		{

		case FINISH_STATUS:		SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
		case READY_STATUS:
		case ACTIVE_STATUS:
								if(systemConfig.measureSavedPoints)
								{
									SEND_CDC_MESSAGE( "\r\n*********** BEGIN OF MEASUREMENT DATA ************\r\n" );

									for( uint16_t p = 0; p < systemConfig.measureSavedPoints; p++)
									{
										DateTime_t  date = {0};

										convertUnixTimeToDate ( dataAttribute[p].timeMeasure, &date );
										sprintf( usb_message, "Point: %3hu, Time: %04hd-%02hd-%02hd %02hd:%02hd, Measure voltage, V: %3lu, Temperature, oC: %3i\r\n",
																	p + 1, date.year, date.month, date.day, date.hours, date.minutes, dataAttribute[p].voltageMeasure, (int16_t)dataAttribute[p].temperatureMeasure );
										SEND_CDC_MESSAGE( usb_message );

										for( uint8_t i = 0; i < MATRIX_LINEn; i++ )
										{
											sprintf( usb_message, "\r\nLine %2hhu, Resistance value in MOhm\r\n", i + 1 ); SEND_CDC_MESSAGE( usb_message );
											SEND_CDC_MESSAGE( "Raw  1     2     3     4     5     6     7     8  \r\n   " );

											for(uint8_t j = 0; j < 8; j++ )
											{
												sprintf( usb_message, "%5lu ", dataMeasure[p].resistanceValMOhm[i][j] );
											}

											sprintf( usb_message, "\r\nLine %2hhu, Resistance value in MOhm\r\n", i + 1 ); SEND_CDC_MESSAGE( usb_message );
											SEND_CDC_MESSAGE( "Raw  9    10    11    12    13    14    15    16  \r\n   " );

											for(uint8_t j = 8; j < 16; j++ )
											{
												sprintf( usb_message, "%5lu ", dataMeasure[p].resistanceValMOhm[i][j] ); SEND_CDC_MESSAGE( usb_message );
											}
										}
									}

									SEND_CDC_MESSAGE( "\r\n************ END OF MEASUREMENT DATA *************\r\n" );
								}
								else
									SEND_CDC_MESSAGE( "No data to read.\r\n\r\n" );
								break;

		case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail\r\n\r\n" );
								break;

		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured\r\n\r\n" );
								break;
		}

		return;
	}

	//--------------------------------------------- READ SETTINGS
	if( strstr( usb_message, "READ SETTINGS" ) )
	{
		SEND_CDC_MESSAGE( "System settings:\r\n" );
		sprintf( usb_message, "Test voltage: %lu Volts, Measure voltage: %lu Volts\r\nTesting time: %lu Hours, Measure period: %lu Minutes\r\n",
										systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
		SEND_CDC_MESSAGE( usb_message );
		sprintf( usb_message, "Amplifier factor Ki: %lu, Division factor Kd: %lu, Discharge time: %lu mSec\r\n\r\n",
							systemConfig.kiAmplifire, systemConfig.kdDivider, systemConfig.dischargePreMeasureTimeMs );
		SEND_CDC_MESSAGE( usb_message );
		return;
	}

	//--------------------------------------------- SET Test Time
	ptr = strstr( usb_message, "SET TT=" );
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
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );

		return;
	}

	//---------------------------------------------	SET Measure Period
	ptr = strstr( usb_message, "SET MP=" );
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
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );

		return;
	}

	//--------------------------------------------- Set Test Voltage
	ptr = strstr( usb_message, "SET VT=" );
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
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );

		return;
	}

	//---------------------------------------------	Set Measure Voltage
	ptr = strstr( usb_message, "SET VM=" );
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
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );

		return;
	}

	//--------------------------------------------- SET Current Amplifier Ki
	ptr = strstr( usb_message, "SET KI=" );
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
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );

		return;
	}

	//--------------------------------------------- SET High Voltage divider Kd
	ptr = strstr( usb_message, "SET KD=" );
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
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );

		return;
	}

	//--------------------------------------------- SET Time of Discharge Capacitors Td
	ptr = strstr( usb_message, "SET TD=" );
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
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );

		return;
	}

	//--------------------------------------------- SET Real Time Counter
	ptr = strstr( usb_message, "SET RTC=" );
	if( ptr )
	{
		DateTime_t  date = { 0 };
		int 		res	 = sscanf( ptr, "SET RTC=%4hu:%2hu:%2hu:%2hu:%2hu", &date.year, &date.month, &date.day, &date.hours, &date.minutes );

		if( res == 5 )
		{
			setRTC( &date );
			SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
		}
		else
			SEND_CDC_MESSAGE( "Wrong value or format\r\n\r\n" );

		return;
	}

	//---------------------------------------------
	ptr = strstr( usb_message, "ECHO " );
	if( ptr )
	{
		if( strstr( ptr, "ON" ) )
		{
			SEND_CDC_MESSAGE("Echo switched ON\r\n");
			echoOFF = false;
			return;
		}

		if( strstr( ptr, "OFF" ) )
		{
			SEND_CDC_MESSAGE("Echo switched OFF\r\n");
			echoOFF = true;
			return;
		}
	}

	//---------------------------------------------
	if( strstr( usb_message, "HELP" ) )
	{
		for(uint32_t i = 0; ; i++)
		{
			if( helpStrings[i] )
			{
				strcpy( usb_message, helpStrings[i] );
				SEND_CDC_MESSAGE( usb_message );
			}
			else
				break;
		}
		return;
	}

	//---------------------------------------------
	sendCDCmessage( "Unknown command - enter HELP!\r\n" );
}
