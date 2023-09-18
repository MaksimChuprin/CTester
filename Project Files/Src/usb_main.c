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
		"Measure - take measurement immediately and show a result\r\n",
		"Read Status - show the current state of the system\r\n",
		"Read Data - show measurement results\r\n",
		"Read Settings - show settings values\r\n",
		"Set Vt=<value> - set <value> of test voltage in Volts\r\n",
		"Set Vm=<value> - set <value> of measure voltage in Volts\r\n",
		"Set Ve=<value> - set <value> of acceptable HV error in mVolts\r\n",
		"Set Tt=<value> - set <value> of test time in Hours\r\n",
		"Set Tp=<value> - set <value> of measure period in Minutes\r\n",
		"Set Td=<value> - set <value> of discharge time in mSec\r\n",
		"Set Ta=<value> - set <value> of amplifier settle time in mSec\r\n",
		"Set Th=<value> - set <value> of max HV settle time in mSec\r\n",
		"Set Ki=<value> - set factor of current amplifier\r\n",
		"Set Kd=<value> - set division factor of HV\r\n",
		"Set Km=<value> - set ADC mean factor\r\n",
		"Set RTC=<YYYY:MM:DD:HH:MM> - set current time\r\n",
		"Set default - set system to default\r\n",
		"Echo On - switch echo on\r\n",
		"Echo Off - switch echo off\r\n",
		"Help - list available commands\r\n",
		0
};

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static char 				usb_message[APP_CDC_DATA_SIZE];
/* Private function prototypes -----------------------------------------------*/
//static bool 				isCableConnected	( void );
static void 				messageDecode		( void );
static void 				UpperCase			( char * ptrmessage );
static void					sendSystemTime		( void );
static void					sendSystemTemperature( void );
static void					sendRealHV			(void);
static void					sendSystemStatus	( void );
static void					sendSystemSettings	( void );
static void					sendMemoryStatus	( void );
static void					sendMeasureResult   (uint32_t * data);
static void					sendMeasureError	(uint8_t line, uint32_t * dataMeasure);
static void					sendVDDA			( void );
static void					sendTestTimePass	( void );
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
		SEND_CDC_MESSAGE( "Start Capacitor Termo-Testing System\r\nSW version: 0.0.1\r\n" );
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
											USB_THREAD_TESTSTARTED_Evt | USB_THREAD_TESTPAUSED_Evt | USB_THREAD_MEASUREERROR_Evt, 100 );

			CLEAR_ALL_EVENTS;

			/*  measure error event */
			if ( event.value.signals == USB_THREAD_MEASUREERROR_Evt )
			{
				switch( MEASURE_GET_ERROR_CODE( getErrorCode() ) )
				{
				case  MEASURE_HV_ERROR:		SEND_CDC_MESSAGE( "************* Fail set High Voltage *****************\r\n" );
											SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ERROR_STATUS );
											event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
											break;

				case  MEASURE_CHANEL_ERROR: SEND_CDC_MESSAGE( "****************** CHANEL fail **********************\r\n" );
											{
												uint32_t   errline = MEASURE_GET_ERROR_LINE( getErrorCode() );
												sendMeasureError( errline, getRawAdc() );
											}
											if( systemConfig.sysStatus == ACTIVE_STATUS )
											{
												SAVE_SYSTEM_CNF( &systemConfig.sysStatus, PAUSE_STATUS );
												event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
											}
											break;

				case  MEASURE_HV_UNSTABLE_ERROR:
											SEND_CDC_MESSAGE( "********* Detected unstable High Voltage **********\r\n" );
											break;

				default:					SEND_CDC_MESSAGE( "******************* UNKNOWN ERROR *****************\r\n" );
				}

				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				sendMemoryStatus();
				sendVDDA();
				sendRealHV();
				if( (systemConfig.sysStatus == ACTIVE_STATUS) || (systemConfig.sysStatus == PAUSE_STATUS) ) sendTestTimePass();
				SEND_CDC_MESSAGE( "\r\n" );
			}

			/*  measure ready event */
			if ( event.value.signals & USB_THREAD_MEASUREREADY_Evt )
			{
				/* save data */
				if( (systemConfig.sysStatus == ACTIVE_STATUS) && systemConfig.measureSavedPoints < STAT_ARRAY_SIZE )
				{
					SAVE_MESURED_DATA( &dataMeasure[systemConfig.measureSavedPoints], getMeasureData() );
					SAVE_SYSTEM_CNF( &dataAttribute[systemConfig.measureSavedPoints].timeMeasure, getRTC() );
					SAVE_SYSTEM_CNF( &dataAttribute[systemConfig.measureSavedPoints].voltageMeasure, systemConfig.uMeasureVol );
					SAVE_SYSTEM_CNF( &dataAttribute[systemConfig.measureSavedPoints].temperatureMeasure, (uint32_t)getTemperature() );
					uint32_t newSavePoint = systemConfig.measureSavedPoints + 1;
					SAVE_SYSTEM_CNF( &systemConfig.measureSavedPoints, newSavePoint );
				}

				SEND_CDC_MESSAGE( "***************** BEGIN OF DATA ******************\r\n" );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				if( (systemConfig.sysStatus == ACTIVE_STATUS) || (systemConfig.sysStatus == PAUSE_STATUS) )
				{
					sendMemoryStatus();
					sendTestTimePass();
				}
				sendMeasureResult( getMeasureData() );
				SEND_CDC_MESSAGE( "****************** END OF DATA *******************\r\n\r\n" );
			}

			/*  test stop event */
			if ( event.value.signals & USB_THREAD_TESTSTOPPED_Evt )
			{
				if( systemConfig.measureSavedPoints )
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, FINISH_STATUS );
				else
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );

				SEND_CDC_MESSAGE( "***************** Test finished  ****************\r\n" );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				sendMemoryStatus();
				sendRealHV();
				sendTestTimePass();
				SEND_CDC_MESSAGE( "\r\n" );
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/*  test pause event */
			if ( event.value.signals & USB_THREAD_TESTPAUSED_Evt )
			{
				SAVE_SYSTEM_CNF( &systemConfig.sysStatus, PAUSE_STATUS );
				SEND_CDC_MESSAGE( "***************** Test paused  ****************\r\n" );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				sendMemoryStatus();
				sendRealHV();
				sendTestTimePass();
				SEND_CDC_MESSAGE( "\r\n" );
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/*  test start event */
			if ( event.value.signals & USB_THREAD_TESTSTARTED_Evt )
			{
				switch( systemConfig.sysStatus )
				{
				case READY_STATUS:
				case ERROR_STATUS: 	SEND_CDC_MESSAGE( "***************** Test started ****************\r\n" );
									SAVE_SYSTEM_CNF( &systemConfig.measureSavedPoints, 0 );
									break;

				case PAUSE_STATUS:
				case ACTIVE_STATUS: SEND_CDC_MESSAGE( "***************** Test continued ****************\r\n" );
									break;
				}

				SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ACTIVE_STATUS );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				sendMemoryStatus();
				sendVDDA();
				sendRealHV();
				sendTestTimePass();
				SEND_CDC_MESSAGE( "\r\n" );
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/* message got event  */
			if (event.value.signals & USB_THREAD_MESSAGEGOT_Evt)
			{
				messageDecode();
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
bool isCableConnected(void)
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
	static bool echoOFF = true;
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

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Command ignored - test already run\r\n\r\n" );
								break;

		case FINISH_STATUS:		SEND_CDC_MESSAGE( "Command ignored - data of finished test not read yet\r\n\r\n" );
								break;

		case ERROR_STATUS:
		case READY_STATUS:		SEND_CDC_MESSAGE( "Starting test...\r\n" );
								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTTEST_Evt );
								break;

		case PAUSE_STATUS:		SEND_CDC_MESSAGE( "Continuation test...\r\n" );
								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTTEST_Evt );
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

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Pausing test...\r\n" );
								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_PAUSETEST_Evt );
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
		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Terminating test...\r\n" );
								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STOPTEST_Evt );
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
								SEND_CDC_MESSAGE( "Take measure...\r\n" );
								break;
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
		if( (systemConfig.sysStatus == ACTIVE_STATUS) || (systemConfig.sysStatus == PAUSE_STATUS) ) sendTestTimePass();
		sendRealHV();
		sendVDDA();
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
		case PAUSE_STATUS:
		case ACTIVE_STATUS:
								if(systemConfig.measureSavedPoints)
								{
									SEND_CDC_MESSAGE( "*********** BEGIN OF DATA ************\r\n" );

									for( uint16_t p = 0, len = 0; p < systemConfig.measureSavedPoints; p++, len = 0 )
									{
										DateTime_t  date = {0};

										convertUnixTimeToDate ( dataAttribute[p].timeMeasure, &date );
										len += sprintf( &usb_message[len], "Point: %3hu, Time: %04hd-%02hd-%02hd %02hd:%02hd, Measure voltage: %3lu V,",
																	p + 1, date.year, date.month, date.day, date.hours, date.minutes, dataAttribute[p].voltageMeasure );
										if( dataAttribute[p].temperatureMeasure != SENSOR_NOT_CONNECTED )
											len += sprintf( &usb_message[len], "Temperature: %i oC\r\n", (int16_t)dataAttribute[p].temperatureMeasure );
										else
											len += sprintf( &usb_message[len], "Temperature: --- oC\r\n" );
										SEND_CDC_MESSAGE( usb_message );
										sendMeasureResult( &dataMeasure[p].resistanceValMOhm[0][0] );
										SEND_CDC_MESSAGE( "\r\n" );
									}

									SEND_CDC_MESSAGE( "************ END OF DATA *************\r\n" );
								}
								else
									SEND_CDC_MESSAGE( "No data to read.\r\n\r\n" );
								break;
		}
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
	ptr = strstr( usb_message, "SET TP=" );
	if( ptr )
	{
		uint32_t 	measPeriod 	= 0;
		int 		res			= sscanf( ptr, "SET TP=%lu", &measPeriod );

		if( res )
		{
			if( measPeriod < 1 ) SEND_CDC_MESSAGE( "Measuring period must be more then 30 minutes\r\n\r\n" )
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
				SAVE_SYSTEM_CNF( &systemConfig.dischargeTimeMs, Td );

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

	//--------------------------------------------- SET amplifier settle time Ta
	ptr = strstr( usb_message, "SET TA=" );
	if( ptr )
	{
		uint32_t 	Ta 	= 0;
		int 		res			= sscanf( ptr, "SET TA=%lu", &Ta );

		if( res )
		{
			if( Ta > 1000 || Ta < 10 ) SEND_CDC_MESSAGE( "Time of amplifier settle time must be 10...1000 mSec\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.IAmplifierSettleTimeMs, Ta );

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

	//--------------------------------------------- SET HV max settle time Th
	ptr = strstr( usb_message, "SET TH=" );
	if( ptr )
	{
		uint32_t 	Th 	= 0;
		int 		res			= sscanf( ptr, "SET TH=%lu", &Th );

		if( res )
		{
			if( Th > 5000 || Th < 300 ) SEND_CDC_MESSAGE( "Time of HV max settle time must be 300...5000 mSec\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.HVMaxSettleTimeMs, Th );

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

	//--------------------------------------------- SET HV max settle time Th
	ptr = strstr( usb_message, "SET KM=" );
	if( ptr )
	{
		uint32_t 	Km 	= 0;
		int 		res			= sscanf( ptr, "SET KM=%lu", &Km );

		if( res )
		{
			if( Km > 16384 || Km < 16 ) SEND_CDC_MESSAGE( "ADC mean factor must be must be 16...16384\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.HVMaxSettleTimeMs, Km );

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

	//--------------------------------------------- SET default values of system
	ptr = strstr( usb_message, "SET DEFAULT" );
	if( ptr )
	{
		const uint32_t 	Km 	= 256;	//
		const uint32_t 	Th 	= 1000;	// msec
		const uint32_t 	Ta 	= 50;	// msec
		const uint32_t 	Td 	= 500;	// msec
		const uint32_t 	Tm 	= 60;	// minutes
		const uint32_t 	Tt 	= 168;	// hours
		const uint32_t 	Kd 	= 101;
		const uint32_t 	Ki 	= 1000000;
		const uint32_t 	Ve 	= 500;	// mV
		const uint32_t 	Vm 	= 25;	// V
		const uint32_t 	Vt 	= 50;	// V


		SAVE_SYSTEM_CNF( &systemConfig.uTestVol, Vt );
		SAVE_SYSTEM_CNF( &systemConfig.uMeasureVol, Vm );
		SAVE_SYSTEM_CNF( &systemConfig.MaxErrorHV_mV, Ve );
		SAVE_SYSTEM_CNF( &systemConfig.kiAmplifire, Ki );
		SAVE_SYSTEM_CNF( &systemConfig.kdDivider, Kd );
		SAVE_SYSTEM_CNF( &systemConfig.testingTimeSec, Tt * 3600 );
		SAVE_SYSTEM_CNF( &systemConfig.measuringPeriodSec, Tm * 60 );
		SAVE_SYSTEM_CNF( &systemConfig.dischargeTimeMs, Td );
		SAVE_SYSTEM_CNF( &systemConfig.IAmplifierSettleTimeMs, Ta );
		SAVE_SYSTEM_CNF( &systemConfig.HVMaxSettleTimeMs, Th );
		SAVE_SYSTEM_CNF( &systemConfig.adcMeanFactor, Km );

		if( CheckSysCnf() && (systemConfig.sysStatus == NO_CONFIG_STATUS) )
						SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );

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
	sprintf( usb_message, "Memory contains: %lu Point(s) of Data\r\n", systemConfig.measureSavedPoints );
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

	if( t != SENSOR_NOT_CONNECTED )	sprintf( usb_message, "System temperature, oC: %i\r\n", t);
	else							sprintf( usb_message, "System temperature: sensor not connected\r\n");
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendRealHV(void)
{
	uint32_t hv = getHighVoltagemV();

	sprintf( usb_message, "Current High Voltage: %3lu.%03lu V\r\n", hv / 1000, hv % 1000);
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendVDDA(void)
{
	uint32_t vdda = getVrefmV();

	sprintf( usb_message, "VDDA Voltage: %3lu.%03lu V\r\n", vdda / 1000, vdda % 1000);
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendTestTimePass(void)
{
	uint32_t pass_time = getTestTimePass();
	uint16_t  days    = pass_time / 86400;
	uint16_t  hours   = (pass_time - days * 86400) / 3600;
	uint16_t  minutes = (pass_time - hours * 3600) / 60;

	sprintf( usb_message, "Testing time passed: %02u:%02u:%02u <DD>:<HH>:<MM>\r\n", days, hours, minutes);
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

	sprintf( usb_message, "Test voltage: %lu V\r\nMeasure voltage: %lu V\r\nTest time: %lu hours\r\nMeasure period: %lu minutes\r\n",
							systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
	SEND_CDC_MESSAGE( usb_message );

	sprintf( usb_message, "Amplifier factor Ki: %lu\r\nDivision factor Kd: %lu\r\nDischarge time: %lu msec\r\nAcceptable HV error: %lu mV\r\n",
						systemConfig.kiAmplifire, systemConfig.kdDivider, systemConfig.dischargeTimeMs, systemConfig.MaxErrorHV_mV );
	SEND_CDC_MESSAGE( usb_message );

	sprintf( usb_message, "Amplifier settle time: %lu msec\r\nMax HV settle time: %lu msec\r\n",
						systemConfig.IAmplifierSettleTimeMs, systemConfig.HVMaxSettleTimeMs );
	SEND_CDC_MESSAGE( usb_message );

	SEND_CDC_MESSAGE( "\r\n" );
}

/*
 *
 */
static void	sendMeasureResult(uint32_t * dataMeasure)
{

	for( uint16_t i = 0, len = 0; i < MATRIX_LINEn; i++, len = 0 )
	{
		len += sprintf( &usb_message[len], "Line %u, Raw  1 - 8, Resistance value, MOhm\r\n", i + 1 );

		for( uint8_t j = 0; j < 8; j++ )
			 len += sprintf( &usb_message[len], "%*lu  ", 5, dataMeasure[i * MATRIX_RAWn + j] );
		len += sprintf( &usb_message[len], "\r\n" );

		len += sprintf( &usb_message[len], "Line %u, Raw  9 - 16, Resistance value, MOhm\r\n", i + 1 );

		for( uint8_t j = 8; j < 16; j++ )
					 len += sprintf( &usb_message[len], "%*lu  ", 5, dataMeasure[i * MATRIX_RAWn + j] );
		len += sprintf( &usb_message[len], "\r\n" );

		SEND_CDC_MESSAGE( usb_message );
	}
}

/*
 *
 */
static void	sendMeasureError(uint8_t line, uint32_t * dataMeasure)
{
	sprintf( usb_message, "Error Line %u, Raw 1 - 16, State: Ok or Er\r\n", line + 1 );
	SEND_CDC_MESSAGE( usb_message );

	for( uint8_t j = 0; j < MATRIX_RAWn; j++ )
		{ sprintf( usb_message, "%s  ", dataMeasure[j] < 4000 ? "Ok" : "Er" ); SEND_CDC_MESSAGE( usb_message ); }
	SEND_CDC_MESSAGE( "\r\n" );
}
