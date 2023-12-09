/**
  ******************************************************************************
  * @file    usb_main.c
  * @author  Epta
  * @brief   USB device CDC application main file
  ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "main.h"

extern 	USBD_HandleTypeDef  		USBD_Device;
extern 	osThreadId					MeasureThreadHandle;

extern 	volatile sysCfg_t			systemConfig;
extern  dataAttribute_t				dataAttribute[];
extern 	dataMeasure_t				dataMeasure[];

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define RANGE_12BITS                ((int32_t) 4095)    /* Max digital value with a full range of 12 bits */
#define RANGE_12BITS_90             ((int32_t) (RANGE_12BITS * 0.9))    /* 0.9 from Max digital value with a full range of 12 bits */
#define HELP_END					0
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static char 				usb_message[APP_CDC_TX_DATA_SIZE];
/* Private function prototypes -----------------------------------------------*/
static void 				messageDecode				( void );
static void 				UpperCase					( char * ptrmessage );
static void					sendSystemTime				( void );
static void					sendSystemTemperature		( void );
static void					sendRealHV					(void);
static void					sendSystemStatus			( void );
static void					sendSystemSettings			( void );
static void					sendMemoryStatus			( void );
static void					sendMeasureResult			( uint32_t * data, uint32_t measVol );
static void					sendCurrentTestResult		(uint32_t * data);
static void					sendCapacitanceTestResult	(uint32_t * data);
static void					sendMeasureError			(uint8_t line, uint32_t * dataMeasure);
static void					sendVDDA					( void );
static void					sendTestTimePass			( void );

/* Constants ---------------------------------------------------------*/
const char * helpStrings[] = {

		"Start - start test process\r\n",
		"Pause - pause test process\r\n",
		"Stop - stop test process\r\n",
		"Check - check High Voltage, Leakage Current, Capacitors Contact\r\n",
		"Measure - take measurement immediately and show a result\r\n",
		"Read Status - show the current state of the system\r\n",
		"Read Data - show measurement results\r\n",
		"Read Settings - show settings values\r\n",
		"Set Result=<R/I> - show result of measure as: <R> - resistance, <I> - current\r\n",
		"Set Vt=<value> - set <value> of test voltage in Volts\r\n",
		"Set Vm=<value> - set <value> of measure voltage in Volts\r\n",
		"Set Ve=<value> - set <value> of acceptable HV error in mVolts\r\n",
		"Set Tt=<value> - set <value> of test time in Hours\r\n",
		"Set Tp=<value> - set <value> of measure period in Minutes\r\n",
		"Set Td=<value> - set <value> of discharge time in mSec\r\n",
		"Set To=<value> - set <value> of <OPTO> signal settle time in mSec\r\n",
		"Set Ta=<value> - set <value> of amplifier settle time in mSec\r\n",
		"Set Th=<value> - set <value> of max HV settle time in mSec\r\n",
		"Set Ki=<value> - set factor of current amplifier\r\n",
		"Set Kd=<value> - set division factor of HV\r\n",
		"Set RTC=<YYYY-MM-DD HH:MM> - set current time\r\n",
		"Set default - set system to default\r\n",
		"Echo On - switch echo on\r\n",
		"Echo Off - switch echo off\r\n",
		"Help - list available commands\r\n",
		"******** fine tuning commands ********\r\n",
		"Set Km=<value> - set ADC mean factor, 1 - 138 us\r\n",
		"Set DAC_P1=<value> - time step of DAC in triangle mode, uSec\r\n",
		"Set DAC_P2=<value> - DAC-code triangle amplitude: 31, 63, 127, 255, 511, 1023, 2047, 4095\r\n",
		"Set Short_I=<value> - set short high current threshold for error detect, nA \r\n",
		"Set Contact_C=<value> - set capacitance low threshold for error detect, pF \r\n",
		HELP_END
};

/* static variables ---------------------------------------------------------*/

/**
  * @brief  UsbCDCThread
  * @param  argument: Not used
  * @retval None
  */
void UsbCDCThread(const void *argument)
{
	// wait Vref, Temperature
	osDelay(1500);

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
		/* */
		osDelay(100);

		/* display sys info */
		SEND_CDC_MESSAGE( "\r\n********************************************\r\n" );
		SEND_CDC_MESSAGE( "Start Capacitor Termo-Testing System\r\nSW version: 0.0.3\r\n" );
		sendSystemTime();
		sendSystemTemperature();
		sendSystemStatus();
		sendMemoryStatus();
		SEND_CDC_MESSAGE( "********************************************\r\n\r\n" );

		// do connection
		for(;;)
		{
			// wait message
			osEvent event = osSignalWait( USB_THREAD_MESSAGEGOT_Evt | USB_THREAD_MEASURESTARTED_Evt | USB_THREAD_MEASUREREADY_Evt | USB_THREAD_TESTSTOPPED_Evt |
											USB_THREAD_TESTSTARTED_Evt | USB_THREAD_TESTPAUSED_Evt | USB_THREAD_MEASUREERROR_Evt |
											USB_THREAD_CHECKCURRENT_Evt | USB_THREAD_CHECKCAP_Evt | USB_THREAD_CHECKHV_Evt | USB_THREAD_IDLEMODE_Evt, 100 );

			CLEAR_ALL_EVENTS;

			/*  measure error event */
			if ( event.value.signals == USB_THREAD_MEASUREERROR_Evt )
			{
				if( MEASURE_GET_ERROR_CODE( getErrorCode() ) & MEASURE_HV_ERROR )
				{
					SEND_CDC_MESSAGE( "\r\n************* Fail set High Voltage <UR> *****************\r\n\r\n" );
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ERROR_STATUS );
					event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
				}

				if( MEASURE_GET_ERROR_CODE( getErrorCode() ) & MEASURE_CHANEL_ERROR )
				{
					SEND_CDC_MESSAGE( "\r\n********** Fail set High Voltage on Line  ****************\r\n" );
					uint32_t   errline = MEASURE_GET_ERROR_LINE( getErrorCode() );
					sendMeasureError( errline, getRawAdc() );

					if( systemConfig.sysStatus == ACTIVE_STATUS )
					{
						SAVE_SYSTEM_CNF( &systemConfig.sysStatus, PAUSE_STATUS );
						event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
					}
					SEND_CDC_MESSAGE( "\r\n" );
				}

				if( MEASURE_GET_ERROR_CODE( getErrorCode() ) & MEASURE_HV_UNSTABLE_ERROR )
				{
					SEND_CDC_MESSAGE( "\r\n********* Detected unstable High Voltage **********\r\n\r\n" );
				}

				if( MEASURE_GET_ERROR_CODE( getErrorCode() ) & MEASURE_HV_ZERO_ERROR )
				{
					SEND_CDC_MESSAGE( "\r\n!!!!   ALARM! Unable switch off High Voltage   !!!!\r\n\r\n" );
				}

				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				sendMemoryStatus();
				sendVDDA();
				if( (systemConfig.sysStatus == ACTIVE_STATUS) || (systemConfig.sysStatus == PAUSE_STATUS) ) sendTestTimePass();
				SEND_CDC_MESSAGE( "\r\n" );
			}

			/*  measure started event */
			if ( event.value.signals & USB_THREAD_MEASURESTARTED_Evt )
			{
				SEND_CDC_MESSAGE( "\r\n***************** Measure started ******************\r\n" );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				if( (systemConfig.sysStatus == ACTIVE_STATUS) || (systemConfig.sysStatus == PAUSE_STATUS) )
				{
					sendMemoryStatus();
					sendTestTimePass();
				}
				SEND_CDC_MESSAGE( "\r\n" );
			}

			/*  measure ready event */
			if ( event.value.signals & USB_THREAD_MEASUREREADY_Evt )
			{
				SEND_CDC_MESSAGE( "\r\n***************** Measure finished ******************\r\n" );

				/* save data */
				if( (systemConfig.sysStatus == ACTIVE_STATUS) && systemConfig.measureSavedPoints < STAT_ARRAY_SIZE )
				{
					SAVE_MESURED_DATA( &dataMeasure[systemConfig.measureSavedPoints], getMeasureData() );
					SAVE_SYSTEM_CNF( &dataAttribute[systemConfig.measureSavedPoints].timeMeasure, getRTC() );
					SAVE_SYSTEM_CNF( &dataAttribute[systemConfig.measureSavedPoints].voltageMeasure, systemConfig.uMeasureVol );
					SAVE_SYSTEM_CNF( &dataAttribute[systemConfig.measureSavedPoints].temperatureMeasure, (uint32_t)getTemperature() );
					uint32_t newSavePoint = systemConfig.measureSavedPoints + 1;
					SAVE_SYSTEM_CNF( &systemConfig.measureSavedPoints, newSavePoint );
					SEND_CDC_MESSAGE( "\r\n*************** Data saved ****************\r\n" );
				}

				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();

				if( (systemConfig.sysStatus == ACTIVE_STATUS) || (systemConfig.sysStatus == PAUSE_STATUS) )
				{
					sendMemoryStatus();
					sendTestTimePass();
				}

				SEND_CDC_MESSAGE( "\r\n***************** BEGIN OF DATA ******************\r\n" );
				sendMeasureResult( getMeasureData(), systemConfig.uMeasureVol );
				SEND_CDC_MESSAGE( "\r\n****************** END OF DATA *******************\r\n\r\n" );
			}

			/*  test stop event */
			if ( event.value.signals & USB_THREAD_TESTSTOPPED_Evt )
			{
				if( systemConfig.measureSavedPoints )
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, FINISH_STATUS );
				else
					SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );

				SEND_CDC_MESSAGE( "\r\n***************** Test finished  ****************\r\n" );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				sendMemoryStatus();
				sendTestTimePass();
				SEND_CDC_MESSAGE( "\r\n" );
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/*  test pause event */
			if ( event.value.signals & USB_THREAD_TESTPAUSED_Evt )
			{
				SAVE_SYSTEM_CNF( &systemConfig.sysStatus, PAUSE_STATUS );
				SEND_CDC_MESSAGE( "\r\n***************** Test paused  ****************\r\n" );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				sendMemoryStatus();
				sendTestTimePass();
				SEND_CDC_MESSAGE( "\r\n" );
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/*  test CHECK CURRENT event */
			if ( event.value.signals & USB_THREAD_CHECKHV_Evt )
			{
				SEND_CDC_MESSAGE( "\r\n*********** High Voltage test - OK ***********\r\n" );
				sendSystemTime();
				SEND_CDC_MESSAGE( "\r\n" );
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/*  test CHECK CURRENT event */
			if ( event.value.signals & USB_THREAD_CHECKCURRENT_Evt )
			{
				SEND_CDC_MESSAGE( "\r\n*********** Leakage threshold test ***********\r\n" );
				sendSystemTime();
				sendCurrentTestResult(getMeasureData());
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/*  test CHECK CURRENT event */
			if ( event.value.signals & USB_THREAD_CHECKCAP_Evt )
			{
				SEND_CDC_MESSAGE( "\r\n*********** Capacitors Contact Test ***********\r\n" );
				sendSystemTime();
				sendCapacitanceTestResult(getMeasureData());

				SEND_CDC_MESSAGE( "\r\n***************** Check finished  ****************\r\n" );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				SEND_CDC_MESSAGE( "\r\n" );
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/*  test start event */
			if ( event.value.signals & USB_THREAD_TESTSTARTED_Evt )
			{
				switch( systemConfig.sysStatus )
				{
				case READY_STATUS:
				case ERROR_STATUS: 	SEND_CDC_MESSAGE( "\r\n***************** Test started ****************\r\n" );
									SAVE_SYSTEM_CNF( &systemConfig.measureSavedPoints, 0 );
									break;

				case PAUSE_STATUS:
				case ACTIVE_STATUS: SEND_CDC_MESSAGE( "\r\n***************** Test continued ****************\r\n" );
									break;
				}

				SAVE_SYSTEM_CNF( &systemConfig.sysStatus, ACTIVE_STATUS );
				sendSystemTime();
				sendSystemTemperature();
				sendSystemStatus();
				sendMemoryStatus();
				sendVDDA();
				sendTestTimePass();
				SEND_CDC_MESSAGE( "\r\n" );
				event.value.signals &= ~USB_THREAD_MESSAGEGOT_Evt;
			}

			/*  test pause event */
			if ( event.value.signals & USB_THREAD_IDLEMODE_Evt )
			{
				SEND_CDC_MESSAGE( "\r\n***************** System Idle  ****************\r\n\r\n" );
				event.value.signals &= ~USB_THREAD_IDLEMODE_Evt;
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
	return true;
	// return BSP_PAN_GetState();
}

/* Private functions ---------------------------------------------------------*/

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
		case READY_STATUS:		SEND_CDC_MESSAGE( "Starting test...\r\n\r\n" );
								osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTTEST_Evt );
								break;

		case PAUSE_STATUS:		SEND_CDC_MESSAGE( "Continue test...\r\n\r\n" );
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

		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Pausing test...\r\n\r\n" );
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
		case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "Terminating test...\r\n\r\n" );
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
								SEND_CDC_MESSAGE( "Take measure...\r\n\r\n" );
								break;
		}
		return;
	}

	//--------------------------------------------- CHECK
	if( strstr( usb_message, "CHECK" ) )
	{
		switch(systemConfig.sysStatus)
		{
		case NO_CONFIG_STATUS:	SEND_CDC_MESSAGE( "Command ignored - system not configured\r\n\r\n" );
								break;

		case ERROR_STATUS:
		case PAUSE_STATUS:
		case READY_STATUS:
		case ACTIVE_STATUS:
		case FINISH_STATUS:		osSignalSet( MeasureThreadHandle, MEASURE_THREAD_CHECK_Evt );
								SEND_CDC_MESSAGE( "Start checking procedure...\r\n\r\n" );
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

		case FINISH_STATUS:		SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS ); // @suppress("No break at end of case")
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
										sendMeasureResult( &dataMeasure[p].resistanceValMOhm[0][0], dataAttribute[p].voltageMeasure );
										SEND_CDC_MESSAGE( "\r\n" );
									}

									SEND_CDC_MESSAGE( "************ END OF DATA *************\r\n\r\n" );
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
			if( Td > 50000 || Td < 10 ) SEND_CDC_MESSAGE( "Time of Discharge must be 10...50000 mSec\r\n\r\n" )
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
			if( Ta > 5000 || Ta < 10 ) SEND_CDC_MESSAGE( "Time of amplifier settle time must be 10...5000 mSec\r\n\r\n" )
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

	//--------------------------------------------- SET <OPTO> signal settle time To
	ptr = strstr( usb_message, "SET TO=" );
	if( ptr )
	{
		uint32_t 	To 	= 0;
		int 		res			= sscanf( ptr, "SET TO=%lu", &To );

		if( res )
		{
			if( To > 10000 || To < 100 ) SEND_CDC_MESSAGE( "<OPTO> signal settle time must be 100...10000 mSec\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.optoSignalSettleTimeMs, To );

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

	//--------------------------------------------- SET mean factor
	ptr = strstr( usb_message, "SET KM=" );
	if( ptr )
	{
		uint32_t 	Km 	= 0;
		int 		res			= sscanf( ptr, "SET KM=%lu", &Km );

		if( res )
		{
			if( Km > 16384 || Km < 16 ) SEND_CDC_MESSAGE( "ADC mean factor must be 16...16384\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.adcMeanFactor, Km );

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

	//--------------------------------------------- SET DAC_P1 in uSec
	ptr = strstr( usb_message, "SET DAC_P1=" );
	if( ptr )
	{
		uint32_t 	DAC_P1 	= 0;
		int 		res		= sscanf( ptr, "SET DAC_P1=%lu", &DAC_P1 );

		if( res )
		{
			if( DAC_P1 > 65535 || DAC_P1 < 16 ) SEND_CDC_MESSAGE( "time step of DAC must be 16...65535\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.dacTrianglePeriodUs, DAC_P1 );

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

	//--------------------------------------------- SET DAC_P2 in DAC-code
	ptr = strstr( usb_message, "SET DAC_P2=" );
	if( ptr )
	{
		uint32_t 	DAC_P2 	= 0;
		int 		res		= sscanf( ptr, "SET DAC_P2=%lu", &DAC_P2 );

		if( res )
		{
			if( (DAC_P2 != 4095) &&
				(DAC_P2 != 2047) &&
				(DAC_P2 != 1023) &&
				(DAC_P2 != 511 ) &&
				(DAC_P2 != 255 ) &&
				(DAC_P2 != 127 ) &&
				(DAC_P2 != 63  ) &&
				(DAC_P2 != 31) 	) SEND_CDC_MESSAGE( "Triangle amplitude of DAC must be 31, 63, 127, 255, 511, 1023, 2047, 4095\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.dacTriangleAmplitude, DAC_P2 );

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

	//--------------------------------------------- SET Short_I in nA
	ptr = strstr( usb_message, "SET SHORT_I=" );
	if( ptr )
	{
		uint32_t 	Short_I 	= 0;
		int 		res		= sscanf( ptr, "SET SHORT_I=%lu", &Short_I );

		if( res )
		{
			if( Short_I > 3000 || Short_I < 100 ) SEND_CDC_MESSAGE( "Value of Short_I must be 100...3000\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.shortErrorTrshCurrent_nA, Short_I );

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

	//--------------------------------------------- SET Contact_C in pF
	ptr = strstr( usb_message, "SET CONTACT_C=" );
	if( ptr )
	{
		uint32_t 	Contact_C 	= 0;
		int 		res		= sscanf( ptr, "SET CONTACT_C=%lu", &Contact_C );

		if( res )
		{
			if( Contact_C > 1000 || Contact_C < 0 ) SEND_CDC_MESSAGE( "Value of Contact_C must be 0...1000\r\n\r\n" )
			else
			{
				SAVE_SYSTEM_CNF( &systemConfig.contactErrorTrshCapacitance_pF, Contact_C );

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

	//---------------------------------------------
	ptr = strstr( usb_message, "SET RESULT=" );
	if( ptr )
	{
		char 		Result 	= 0;
		int 		res		= sscanf( ptr, "SET RESULT=%c", &Result );

		if( res )
		{
			if( Result == 'C' )
				{
					SAVE_SYSTEM_CNF( &systemConfig.resultPresentation, RESULT_AS_CURRENT );
					SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
				}
			else if( Result == 'R' )
				{
					SAVE_SYSTEM_CNF( &systemConfig.resultPresentation, RESULT_AS_RESISTANCE );
					SEND_CDC_MESSAGE( "Ok\r\n\r\n" );
				}
			else SEND_CDC_MESSAGE( "Value of Result must be R or C\r\n\r\n" )
		}
		else
			SEND_CDC_MESSAGE( "Wrong format\r\n\r\n" );

		return;
	}

	//--------------------------------------------- SET default values of system
	ptr = strstr( usb_message, "SET DEFAULT" );
	if( ptr )
	{
		const uint32_t 	Result 		= RESULT_AS_CURRENT;	// show result of measure as current
		const uint32_t 	Contact_C 	= 100;	// capacitance low threshold for error detect, pF
		const uint32_t 	Short_I		= 2500;	// short high current threshold for error detect, nA
		const uint32_t 	DAC_P2 		= 255;	// DAC-code triangle amplitude
		const uint32_t 	DAC_P1 		= 78;	// 78 us DAC period Triangle Mode
		const uint32_t 	Km 			= 530;	// 189 uS one ADC scan, 530 ~ 100 ms (50 Hz)
		const uint32_t 	Th 			= 2000;	// msec
		const uint32_t 	To 			= 2000;	// msec
		const uint32_t 	Ta 			= 100;	// msec
		const uint32_t 	Td 			= 5000;	// msec
		const uint32_t 	Tm 			= 15;	// minutes
		const uint32_t 	Tt 			= 24;	// hours
		const uint32_t 	Kd 			= 101;
		const uint32_t 	Ki 			= 1000000;
		const uint32_t 	Ve 			= 1500;	// mV
		const uint32_t 	Vm 			= 50;	// V
		const uint32_t 	Vt 			= 50;	// V

		SAVE_SYSTEM_CNF( &systemConfig.measureSavedPoints, 0 );
		SAVE_SYSTEM_CNF( &systemConfig.uTestVol, Vt );
		SAVE_SYSTEM_CNF( &systemConfig.uMeasureVol, Vm );
		SAVE_SYSTEM_CNF( &systemConfig.MaxErrorHV_mV, Ve );
		SAVE_SYSTEM_CNF( &systemConfig.kiAmplifire, Ki );
		SAVE_SYSTEM_CNF( &systemConfig.kdDivider, Kd );
		SAVE_SYSTEM_CNF( &systemConfig.testingTimeSec, Tt * 3600 );
		SAVE_SYSTEM_CNF( &systemConfig.measuringPeriodSec, Tm * 60 );
		SAVE_SYSTEM_CNF( &systemConfig.dischargeTimeMs, Td );
		SAVE_SYSTEM_CNF( &systemConfig.IAmplifierSettleTimeMs, Ta );
		SAVE_SYSTEM_CNF( &systemConfig.optoSignalSettleTimeMs, To );
		SAVE_SYSTEM_CNF( &systemConfig.HVMaxSettleTimeMs, Th );
		SAVE_SYSTEM_CNF( &systemConfig.adcMeanFactor, Km );
		SAVE_SYSTEM_CNF( &systemConfig.dacTrianglePeriodUs, DAC_P1 );
		SAVE_SYSTEM_CNF( &systemConfig.dacTriangleAmplitude, DAC_P2 );
		SAVE_SYSTEM_CNF( &systemConfig.shortErrorTrshCurrent_nA, Short_I );
		SAVE_SYSTEM_CNF( &systemConfig.contactErrorTrshCapacitance_pF, Contact_C );
		SAVE_SYSTEM_CNF( &systemConfig.resultPresentation, Result );

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
		int 		res	 = sscanf( ptr, "SET RTC=%4hu-%2hu-%2hu %2hu:%2hu", &date.year, &date.month, &date.day, &date.hours, &date.minutes );

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
			SEND_CDC_MESSAGE("Echo switched ON\r\n\r\n");
			echoOFF = false;
			return;
		}

		if( strstr( ptr, "OFF" ) )
		{
			SEND_CDC_MESSAGE("Echo switched OFF\r\n\r\n");
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
		for( uint32_t i = 0, len = 0; ; )
		{
			if( helpStrings[i] )
			{
				if( (len + strlen(helpStrings[i])) < APP_CDC_TX_DATA_SIZE )
				{
					len += sprintf( &usb_message[len], helpStrings[i] );
					i++;
				}
				else
				{
					SEND_CDC_MESSAGE( usb_message );
					len = 0;
				}
			}
			else
			{
				if(len) SEND_CDC_MESSAGE( usb_message );
				break;
			}
		}
		SEND_CDC_MESSAGE( "\r\n" );
		return;
	}

	//---------------------------------------------
	SEND_CDC_MESSAGE( "Unknown command - enter HELP\r\n" );
}

/*
 *
 */
static void	sendMemoryStatus(void)
{
	sprintf( usb_message, "Memory contains %lu Point(s) of Data\r\n", systemConfig.measureSavedPoints );

	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendSystemTime(void)
{
	DateTime_t  date = {0};

	convertUnixTimeToDate( getRTC(), &date );
	sprintf( usb_message, "System time %04hd-%02hd-%02hd %02hd:%02hd\r\n", date.year, date.month, date.day, date.hours, date.minutes );
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendSystemTemperature(void)
{
	int16_t t = getTemperature();

	if( t != SENSOR_NOT_CONNECTED )	sprintf( usb_message, "System temperature, oC: %i\r\n", t);
	else							sprintf( usb_message, "System temperature - sensor not connected\r\n");
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendRealHV(void)
{
	uint32_t hv = getHighVoltagemV();

	sprintf( usb_message, "Current High Voltage, UR= %3lu.%01lu V\r\n", hv / 1000, ( (hv % 1000) + 50) / 100 );
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendVDDA(void)
{
	uint32_t vdda = getVrefmV();

	sprintf( usb_message, "System Voltage, VDD= %3lu.%03lu V\r\n", vdda / 1000, vdda % 1000);
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

	sprintf( usb_message, "Testing time passed  %u day(s), %u hour(s), %u minute(s)\r\n", days, hours, minutes);
	SEND_CDC_MESSAGE(usb_message);
}

/*
 *
 */
static void	sendSystemStatus(void)
{
	switch(systemConfig.sysStatus)
	{
			case READY_STATUS:		SEND_CDC_MESSAGE( "System status: idle\r\n" );
									break;

			case ACTIVE_STATUS:		SEND_CDC_MESSAGE( "System status: test running\r\n" );
									break;

			case PAUSE_STATUS:		SEND_CDC_MESSAGE( "System status: test paused\r\n" );
									break;

			case FINISH_STATUS:		SEND_CDC_MESSAGE( "System status: test finished\r\n" );
									break;

			case ERROR_STATUS:		SEND_CDC_MESSAGE( "System status: fail\r\n" );
									break;

			default:				SAVE_SYSTEM_CNF( &systemConfig.sysStatus, NO_CONFIG_STATUS ); // @suppress("No break at end of case")


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

	sprintf( usb_message, "Result of measure presented as %s\r\n", systemConfig.resultPresentation == RESULT_AS_RESISTANCE ? "Resistance, MOhm" : "Current, nA" );
	SEND_CDC_MESSAGE( usb_message );

	sprintf( usb_message, "Test voltage, Vt= %lu V\r\nMeasure voltage, Vm= %lu V\r\nTest time, Tt= %lu hours\r\nMeasure period, Tp= %lu minutes\r\n",
							systemConfig.uTestVol, systemConfig.uMeasureVol, systemConfig.testingTimeSec / 3600, systemConfig.measuringPeriodSec / 60);
	SEND_CDC_MESSAGE( usb_message );

	sprintf( usb_message, "Amplifier factor, Ki= %lu\r\nDivision factor, Kd= %lu\r\nDischarge time, Td= %lu msec\r\nAcceptable HV error, Ve= %lu mV\r\n",
						systemConfig.kiAmplifire, systemConfig.kdDivider, systemConfig.dischargeTimeMs, systemConfig.MaxErrorHV_mV );
	SEND_CDC_MESSAGE( usb_message );

	sprintf( usb_message, "Amplifier settle time, Ta= %lu msec\r\nMax HV settle time, Th= %lu msec\r\n<OPTO> signal settle time, To=  %lu msec\r\nADC mean factor, Km= %lu \r\n",
						systemConfig.IAmplifierSettleTimeMs, systemConfig.HVMaxSettleTimeMs, systemConfig.optoSignalSettleTimeMs, systemConfig.adcMeanFactor );
	SEND_CDC_MESSAGE( usb_message );

	sprintf( usb_message, "Step of DAC in triangle mode, DAC_P1= %lu uSec\r\nDAC-code triangle amplitude, DAC_P2= %lu\r\n"
			"Short high current threshold, Short_I= %lu nA \r\nCapacitance low threshold, Contact_C= %lu pF \r\n",
			systemConfig.dacTrianglePeriodUs, systemConfig.dacTriangleAmplitude, systemConfig.shortErrorTrshCurrent_nA, systemConfig.contactErrorTrshCapacitance_pF );
	SEND_CDC_MESSAGE( usb_message );

	SEND_CDC_MESSAGE( "\r\n" );
}

/*
 *
 */
static void	sendMeasureResult( uint32_t * data, uint32_t measVol )
{
	for( uint32_t i = 0, len = 0; i < MATRIX_LINEn; i++, len = 0 )
	{
		if(systemConfig.resultPresentation == RESULT_AS_RESISTANCE )
			len += sprintf( &usb_message[len], "Line %lu, Raw  1 - 16, Resistance value, MOhm\r\n", i + 1 );
		else
			len += sprintf( &usb_message[len], "Line %lu, Raw  1 - 16, Current value, nA\r\n", i + 1 );

		for( uint32_t j = 0; j < 16; j++ )
		{
			uint32_t outData = data[i * MATRIX_RAWn + j];

			if(systemConfig.resultPresentation == RESULT_AS_RESISTANCE )
			{
				if( outData == 0 ) outData = 9999;
				else
				{
					outData = measVol * 1000 / outData;	// MOhm
					if( outData > 9999 ) outData = 9999;
				}
			}

			len += sprintf( &usb_message[len], "%*lu  ", 5, outData );
		}
		// "\r\n"
		len += sprintf( &usb_message[len], "\r\n" );
		SEND_CDC_MESSAGE( usb_message );
	}
}

/*
 *
 */
static void	sendMeasureError(uint8_t line, uint32_t * data)
{
	uint16_t len = 0;

	len = sprintf( usb_message, "Error Line %u, Raw 1 - 16, State: Ok or Er\r\n", line + 1 );
	for( uint32_t j = 0; j < MATRIX_RAWn; j++ )
		{ len += sprintf( &usb_message[len], "%s  ", data[j] < RANGE_12BITS_90 ? "Ok" : "Er" );  }
	SEND_CDC_MESSAGE( usb_message );
	SEND_CDC_MESSAGE( "\r\n" );
}

/*
 *
 */
static void	sendCurrentTestResult(uint32_t * data)
{
	for( uint32_t i = 0, len = 0; i < MATRIX_LINEn; i++, len = 0 )
	{
		len = sprintf( usb_message, "Line %lu, Raw 1 - 16: ", i + 1 );
		//  data
		for( uint32_t j = 0; j < MATRIX_RAWn; j++ )
			len += sprintf( &usb_message[len], "%s  ", data[i * MATRIX_RAWn + j] < systemConfig.shortErrorTrshCurrent_nA ? "Ok" : "Er" );
		len += sprintf( &usb_message[len], "\r\n" );
		SEND_CDC_MESSAGE( usb_message );
	}
	SEND_CDC_MESSAGE( "\r\n" );
}

/*
 *
 */
static void sendCapacitanceTestResult(uint32_t * data)
{
	for( uint32_t i = 0, len = 0; i < MATRIX_LINEn; i++, len = 0 )
	{
		len = sprintf( usb_message, "Line %lu ", i + 1 );
		//  data
		for( uint32_t j = 0; j < MATRIX_RAWn; j++ )
			len += sprintf( &usb_message[len], "%s  ", data[i * MATRIX_RAWn + j] > systemConfig.contactErrorTrshCapacitance_pF ? "Ok" : "Er" );
		len += sprintf( &usb_message[len], "\r\n" );
		SEND_CDC_MESSAGE( usb_message );
	}
	SEND_CDC_MESSAGE( "\r\n" );
}

