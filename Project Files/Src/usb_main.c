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

extern 	volatile sysCfg_t			systemConfig;
extern  dataAttribute_t				dataAttribute[];
extern 	dataMeasure_t				dataMeasure[];

const char * helpStrings[] = {

		"Start - start test process\r\n",
		"Pause - pause test process\r\n",
		"Stop - stop test process\r\n",
		"Measure - take the measurement immediately and show a result\r\n",
		"Read Status - display the current state of the system\r\n",
		"Read Data - display measurement results\r\n",
		"Read Settings - display settings values\r\n",
		"Set Vt=<value> - enter <value> of test voltage in Volts\r\n",
		"Set Vm=<value> - enter <value> of measure voltage in Volts\r\n",
		"Set Tt=<value> - enter <value> of test time in Hours\r\n",
		"Set Mp=<value> - enter <value> of measure period in Minutes\r\n",
		"Set Ki=<value> - enter factor of current amplifier\r\n",
		"Set Kd=<value> - enter division factor of HV\r\n",
		"Set Td=<value> - enter <value> of discharge time in mSec\r\n",
		"Set Ve=<value> - enter <value> of acceptable HV error in mVolts\r\n",
		"Set RTC=<YYYY:MM:DD:HH:MM> - enter current time\r\n",
		"Echo On - switch echo on\r\n",
		"Echo Off - switch echo off\r\n",
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
static void					sendSystemTime		( void );
static void					sendSystemTemperature( void );
static void					sendSystemStatus	( void );
static void					sendSystemSettings	( void );
static void					sendMemoryStatus	( void );
static void					sendMeasureResult   (uint32_t * data);
static void					sendMeasureError	(uint8_t line, uint32_t * dataMeasure);
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
		sendSystemTime();
		sendSystemTemperature();
		sendSystemStatus();
		sendMemoryStatus();
		SEND_CDC_MESSAGE( "********************************************\r\n\r\n" );

		// do connection
		for(;;)
		{
			// wait message
			osEvent event = osSignalWait( USB_THREAD_MESSAGEGOT_Evt | USB_THREAD_MEASUREREADY_Evt | USB_THREAD_TESTSTOPPED_Evt |
											USB_THREAD_TESTSTARTED_Evt | USB_THREAD_MEASUREERROR_Evt, 100 );

			if( event.status == osEventSignal )
			{
				if (event.value.signals == USB_THREAD_MESSAGEGOT_Evt)
				{
					messageDecode();
				}

				if ( event.value.signals == USB_THREAD_MEASUREREADY_Evt )
				{
					SAVE_MESURED_DATA( systemConfig.measureSavedPoints, getMeasureData() );
					SAVE_SYSTEM_CNF( &systemConfig.measureSavedPoints, systemConfig.measureSavedPoints + 1 );

					SEND_CDC_MESSAGE( "*********** BEGIN OF MEASUREMENT DATA ************\r\n" );
					sendSystemTime();
					sendSystemTemperature();
					sendMeasureResult( getMeasureData() );
					SEND_CDC_MESSAGE( "************ END OF MEASUREMENT DATA *************\r\n\r\n" );
				}

				if ( event.value.signals == USB_THREAD_TESTSTOPPED_Evt )
				{
					SEND_CDC_MESSAGE( "***************** Test finished ******************\r\n" );
					if(systemConfig.measureSavedPoints)
						SAVE_SYSTEM_CNF( &systemConfig.sysStatus, FINISH_STATUS );
					else
						SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
					sendSystemTime();
					sendSystemStatus();
					sendMemoryStatus();
					SEND_CDC_MESSAGE( "\r\n" );
				}

				if ( event.value.signals == USB_THREAD_TESTSTARTED_Evt )
				{
					SEND_CDC_MESSAGE( "******** Test continue after system reset ********\r\n" );
					sendSystemTime();
					sendSystemStatus();
					sendMemoryStatus();
					SEND_CDC_MESSAGE( "\r\n" );
				}

				if ( event.value.signals == USB_THREAD_MEASUREERROR_Evt )
				{
					switch( MEASURE_GET_ERROR_CODE( getErrorCode() ) )
					{
					case  MEASURE_HV_ERROR:		SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ERROR_STATUS );
												SEND_CDC_MESSAGE( "********* SYSTEM ERROR - High Voltage fail **********\r\n" );
												sendSystemTime();
												sendSystemStatus();
												sendMemoryStatus();
												break;

					case  MEASURE_CHANEL_ERROR: SAVE_SYSTEM_CNF( &systemConfig.sysStatus, PAUSE_STATUS );
												SEND_CDC_MESSAGE( "******************** CHANEL fail *******************\r\n" );
												sendSystemTime();
												sendSystemStatus();
												sendMemoryStatus();
												{
													uint32_t   errline = MEASURE_GET_ERROR_LINE( getErrorCode() );
													uint32_t * ptrData = getMeasureData();
													sendMeasureError( errline, &ptrData[errline * MATRIX_RAWn] );
												}
												break;

					default:					SEND_CDC_MESSAGE( "******************** UNKNOWN ERROR ******************\r\n" );
												sendSystemTime();
												sendSystemStatus();
												sendMemoryStatus();
					}
					SEND_CDC_MESSAGE( "\r\n" );
				}
			}

			/* check cable */
			if( !isCableConnected() )
			{
				USBD_DeInit(&USBD_Device);
				break;
			}
		} // for(;;)
	} // for(;; osDelay(500))
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

	if( echoOFF == false ) SEND_CDC_MESSAGE(usb_message);

	UpperCase(usb_message);

	//--------------------------------------------- START
	if( strstr( usb_message, "START" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "Command ignored - system not configured\r\n\r\n" );
								break;

		case ERROR_STATUS:
		case READY_STATUS:		SEND_CDC_MESSAGE( "Starting..." );

								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTTEST_Evt );
								{
									osEvent event = osSignalWait( USB_THREAD_TESTSTARTED_Evt | USB_THREAD_MEASUREERROR_Evt, osWaitForever );

									if( event.value.signals == USB_THREAD_MEASUREERROR_Evt )
									{
										SEND_CDC_MESSAGE( "Fail to set Test Voltage - error system\r\n\r\n" );
										SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ERROR_STATUS );
										break;
									}
								}

								SEND_CDC_MESSAGE( "Test process started\r\n" );
								sprintf( usb_message, "Test voltage: %lu Volts\r\nMeasure voltage: %lu Volts\r\nTest time: %lu Hours\r\nMeasure period: %lu Minutes\r\n\r\n",
														systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
								SEND_CDC_MESSAGE( usb_message );
								SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ACTIVE_STATUS );

								if(systemConfig.measureSavedPoints) SAVE_SYSTEM_CNF( &systemConfig.measureSavedPoints, 0 );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Command ignored - test already run\r\n\r\n" );
								break;

		case PAUSE_STATUS:		SEND_CDC_MESSAGE( "Continue..." );

								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTTEST_Evt );
								{
									osEvent event = osSignalWait( USB_THREAD_TESTSTARTED_Evt | USB_THREAD_MEASUREERROR_Evt, osWaitForever );

									if( event.value.signals == USB_THREAD_MEASUREERROR_Evt )
									{
										SEND_CDC_MESSAGE( "Fail to set Test Voltage - error system\r\n\r\n" );
										SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ERROR_STATUS );
										break;
									}
								}

								SEND_CDC_MESSAGE( "Test process continued\r\n" );
								SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ACTIVE_STATUS );
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "Command ignored - data of finished test not read yet\r\n\r\n" );
								break;
		}

		return;
	}

	//--------------------------------------------- PAUSE
	if( strstr( usb_message, "PAUSE" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "Command ignored - system not configured\r\n\r\n" );
								break;

		case ERROR_STATUS:
		case FINISH_STATUS:
		case PAUSE_STATUS:
		case READY_STATUS:		SEND_CDC_MESSAGE( "Command ignored -  test not run\r\n\r\n" );
								break;

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Pausing..." );

								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STOPTEST_Evt );
								osSignalWait( USB_THREAD_TESTSTOPPED_Evt | USB_THREAD_MEASUREERROR_Evt, osWaitForever );

								SEND_CDC_MESSAGE( "Test paused" );
								SAVE_SYSTEM_CNF( &systemConfig.sysStatus, PAUSE_STATUS );
								break;
		}

		return;
	}

	//--------------------------------------------- STOP
	if( strstr( usb_message, "STOP" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "Command ignored - system not configured\r\n\r\n" );
								break;

		case ERROR_STATUS:
		case FINISH_STATUS:
		case READY_STATUS:		SEND_CDC_MESSAGE( "Command ignored - test not run\r\n\r\n" );
								break;

		case PAUSE_STATUS:
		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Terminating..." );

								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STOPTEST_Evt );
								osSignalWait( USB_THREAD_TESTSTOPPED_Evt | USB_THREAD_MEASUREERROR_Evt, osWaitForever );

								SEND_CDC_MESSAGE( "Test stopped" );

								if(systemConfig.measureSavedPoints)
								{
									SAVE_SYSTEM_CNF( &systemConfig.sysStatus, FINISH_STATUS );
								}
								else
								{
									SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
								}
								break;
		}

		return;
	}

	//--------------------------------------------- MEASURE
	if( strstr( usb_message, "MEASURE" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "Command ignored - system not configured\r\n\r\n" );
								break;

		case ERROR_STATUS:
		case PAUSE_STATUS:
		case READY_STATUS:
		case ACTIVE_STATUS:
		case FINISH_STATUS:		osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTMESURE_Evt );
								{
									osEvent event = osSignalWait( USB_THREAD_MEASUREREADY_Evt | USB_THREAD_MEASUREERROR_Evt, osWaitForever );

									if ( event.value.signals == USB_THREAD_MEASUREREADY_Evt )
									{
										SEND_CDC_MESSAGE( "*********** BEGIN OF MEASUREMENT DATA ************\r\n" );
										sendSystemTime();
										sendSystemTemperature();
										sendMeasureResult( getMeasureData() );
										SEND_CDC_MESSAGE( "************ END OF MEASUREMENT DATA *************\r\n\r\n" );
									}

									if ( event.value.signals == USB_THREAD_MEASUREERROR_Evt )
									{
										switch( MEASURE_GET_ERROR_CODE( getErrorCode() ) )
										{
										case  MEASURE_HV_ERROR:	SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ERROR_STATUS );
																SEND_CDC_MESSAGE( "********* SYSTEM ERROR - High Voltage fail **********\r\n" );
																sendSystemTime();
																sendSystemStatus();
																sendMemoryStatus();
																break;

										case  MEASURE_CHANEL_ERROR: SAVE_SYSTEM_CNF( &systemConfig.sysStatus, PAUSE_STATUS );
																	SEND_CDC_MESSAGE( "******************** CHANEL fail *******************\r\n" );
																	sendSystemTime();
																	sendSystemStatus();
																	sendMemoryStatus();
																	{
																		uint32_t   errline = MEASURE_GET_ERROR_LINE( getErrorCode() );
																		uint32_t * ptrData = getMeasureData();
																		sendMeasureError( errline, &ptrData[errline * MATRIX_RAWn] );
																	}
																	break;

										default:					SEND_CDC_MESSAGE( "******************** UNKNOWN ERROR ******************\r\n" );
																	sendSystemTime();
																	sendSystemStatus();
																	sendMemoryStatus();
										}
										SEND_CDC_MESSAGE( "\r\n" );
									}
								}
		}
		return;
	}

	//--------------------------------------------- READ STATUS
	if( strstr( usb_message, "READ STATUS" ) )
	{
		sendSystemTime();
		sendSystemTemperature();
		sendSystemStatus();
		sendMemoryStatus();
		SEND_CDC_MESSAGE( "\r\n" );
		return;
	}

	//--------------------------------------------- READ SETTINGS
	if( strstr( usb_message, "READ SETTINGS" ) )
	{
		sendSystemSettings();
		SEND_CDC_MESSAGE( "\r\n" );
		return;
	}

	//--------------------------------------------- READ RTESULTS
	if( strstr( usb_message, "READ DATA" ) )
	{
		switch(systemConfig.sysStatus)
		{

		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "Command ignored - system not configured\r\n\r\n" );
								break;

		case FINISH_STATUS:		SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
		case ERROR_STATUS:
		case READY_STATUS:
		case ACTIVE_STATUS:
								if(systemConfig.measureSavedPoints)
								{
									SEND_CDC_MESSAGE( "*********** BEGIN OF MEASUREMENT DATA ************\r\n" );

									for( uint16_t p = 0; p < systemConfig.measureSavedPoints; p++)
									{
										DateTime_t  date = {0};

										convertUnixTimeToDate ( dataAttribute[p].timeMeasure, &date );
										sprintf( usb_message, "Point: %3hu, Time: %04hd-%02hd-%02hd %02hd:%02hd, Measure voltage, V: %3lu, Temperature, oC: %3i\r\n",
																	p + 1, date.year, date.month, date.day, date.hours, date.minutes, dataAttribute[p].voltageMeasure, (int16_t)dataAttribute[p].temperatureMeasure );
										SEND_CDC_MESSAGE( usb_message );
										sendMeasureResult( &dataMeasure[p].resistanceValMOhm[0][0] );
										SEND_CDC_MESSAGE( "\r\n" );
									}

									SEND_CDC_MESSAGE( "\r\n************ END OF MEASUREMENT DATA *************\r\n" );
								}
								else
									SEND_CDC_MESSAGE( "No data to read.\r\n\r\n" );
								break;
		}
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

				if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
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

				if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
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

				if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
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

				if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
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

	//---------------------------------------------	Set HV error limit
	ptr = strstr( usb_message, "SET VE=" );
	if( ptr )
	{
		uint32_t 	errorVol_mV 	= 1000;
		int 		res			    = sscanf( ptr, "SET VE=%lu", &errorVol_mV );

		if( res )
		{
			if( errorVol_mV < 250 ) SEND_CDC_MESSAGE( "Error HV voltage  must be more then 250 mVolts\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.MaxErrorHV_mV, errorVol_mV );

				if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
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

				if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
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

				if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
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

				if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
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

	//--------------------------------------------- force READY STATUS
	ptr = strstr( usb_message, "CLEAR ERROR" );
	if( ptr )
	{
		if( systemConfig.sysStatus == ERROR_STATUS )
		{
			SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
		}
		SEND_CDC_MESSAGE( "Ok\r\n\r\n" );

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
	sendCDCmessage( "Unknown command - enter HELP\r\n" );
}

/*
 *
 */
static void	sendMemoryStatus(void)
{
	sprintf( usb_message, "Memory contains %3lu Point(s) of Data\r\n", systemConfig.measureSavedPoints );
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendSystemTime(void)
{
	DateTime_t  date = {0};

	convertUnixTimeToDate( getRTC(), &date );
	sprintf( usb_message, "System time: %04hd-%02hd-%02hd %02hd:%02hd\r\n", date.year, date.month, date.day, date.hours, date.minutes );
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendSystemTemperature(void)
{
	int16_t t = getTemperature();

	if( t != SENSOR_NOT_CONNECTED )	sprintf( usb_message, "System temperature, oC: %3i\r\n", t);
	else							sprintf( usb_message, "System temperature: sensor not connected\r\n");
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendSystemStatus(void)
{
	switch(systemConfig.sysStatus)
	{
			case READY_STATUS:		SEND_CDC_MESSAGE( "System status: ready\r\n" );
									break;

			case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: test run\r\n" );
									break;

			case PAUSE_STATUS:		SEND_CDC_MESSAGE( "System status: test paused\r\n" );
									break;

			case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: test finished\r\n" );
									break;

			case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail\r\n" );
									break;

			default:				SAVE_SYSTEM_CNF( &systemConfig.sysStatus, NO_CONFIG_STATUS );

			case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "System status: not configured\r\n" );
									break;
	}
}

/*
 *
 */
static void	sendSystemSettings( void )
{
	SEND_CDC_MESSAGE( "System settings:\r\n" );
	sprintf( usb_message, "Test voltage, V: %lu\r\nMeasure voltage, V: %lur\nTesting time, hours: %lu\r\nMeasure period, minutes: %lu\r\n",
							systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
	SEND_CDC_MESSAGE( usb_message );
	sprintf( usb_message, "Amplifier factor Ki: %lu\r\nDivision factor Kd: %lu\r\nDischarge time, msec: %lu mSec\r\nHV error, mV: %lu\r\n",
						systemConfig.kiAmplifire, systemConfig.kdDivider, systemConfig.dischargePreMeasureTimeMs, systemConfig.MaxErrorHV_mV );
	SEND_CDC_MESSAGE( usb_message );
}

/*
 *
 */
static void	sendMeasureResult(uint32_t * dataMeasure)
{

	for( uint8_t i = 0; i < MATRIX_LINEn; i++ )
	{
		sprintf( usb_message, "Line %2hhu, Resistance value in MOhm\r\n", i + 1 );
		SEND_CDC_MESSAGE( usb_message );
		SEND_CDC_MESSAGE( "Raw  1     2     3     4     5     6     7     8  \r\n   " );

		for(uint8_t j = 0; j < 8; j++ )
			sprintf( usb_message, "%5lu ", dataMeasure[i * MATRIX_RAWn + j] );
		SEND_CDC_MESSAGE( usb_message );
		SEND_CDC_MESSAGE( "\r\n" );

		sprintf( usb_message, "Line %2hhu, Resistance value in MOhm\r\n", i + 1 );
		SEND_CDC_MESSAGE( usb_message );
		SEND_CDC_MESSAGE( "Raw  9    10    11    12    13    14    15    16  \r\n   " );

		for(uint8_t j = 8; j < 16; j++ )
			sprintf( usb_message, "%5lu ", dataMeasure[i * MATRIX_RAWn + j] );
		SEND_CDC_MESSAGE( usb_message );
		SEND_CDC_MESSAGE( "\r\n" );
	}
}

/*
 *
 */
static void	sendMeasureError(uint8_t line, uint32_t * dataMeasure)
{
	sprintf( usb_message, "Error Line %2hhu, Raw state: Ok or Er\r\n", line + 1 );
	SEND_CDC_MESSAGE( usb_message );
	SEND_CDC_MESSAGE( "Raw  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 \r\n    " );

	for(uint8_t j = 0; j < MATRIX_RAWn; j++ ) sprintf( usb_message, "%s ", dataMeasure[j] ? "Ok  " : "Er  " );
	SEND_CDC_MESSAGE( usb_message );
	SEND_CDC_MESSAGE( "\r\n" );
}
