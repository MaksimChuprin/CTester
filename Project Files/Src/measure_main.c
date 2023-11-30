/**
  ******************************************************************************
  * @file    measure_main.c
  * @author  Epta
  * @brief   CTS application measure_main file
  ****************************************************************************** */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Global variables ---------------------------------------------------------*/
extern 	osThreadId					USBThreadHandle;
extern 	ADC_HandleTypeDef    		AdcHandle;
extern 	volatile sysCfg_t			systemConfig;

/* Private typedef -----------------------------------------------------------*/

typedef enum {

	hvStabT = 0,
	checkLines,
	checkCapacitanceTrsh,
	runTest

} testModeState_t;

typedef enum {

	measureZeroShift = 0,
	hvStabM,
	measureResistance

} measureModeState_t;


typedef enum {

	checkHVStab = 0,
	checkLinesAndLikage,
	checkCap,

} checkModeState_t;


/* Private define ------------------------------------------------------------*/
#define ADC_DMA_ARRAY_LEN			10
#define ADC_MEAN_ARRAY_LEN			18
#define VREF_MEAN_FACTOR			32
#define	TRIANGLE_MEAN_FACTOR		75

#define ADC_DMA_ARRAY_R0_8			0
#define ADC_DMA_ARRAY_R1_9			1
#define ADC_DMA_ARRAY_R2_10			2
#define ADC_DMA_ARRAY_R3_11			3
#define ADC_DMA_ARRAY_R4_12			4
#define ADC_DMA_ARRAY_R5_13			5
#define ADC_DMA_ARRAY_R6_14			6
#define ADC_DMA_ARRAY_R7_15			7
#define ADC_DMA_ARRAY_DAC			8
#define ADC_DMA_ARRAY_VINTREF		9

#define ADC_MEAN_ARRAY_R0			0
#define ADC_MEAN_ARRAY_R1			1
#define ADC_MEAN_ARRAY_R2			2
#define ADC_MEAN_ARRAY_R3			3
#define ADC_MEAN_ARRAY_R4			4
#define ADC_MEAN_ARRAY_R5			5
#define ADC_MEAN_ARRAY_R6			6
#define ADC_MEAN_ARRAY_R7			7
#define ADC_MEAN_ARRAY_R8			8
#define ADC_MEAN_ARRAY_R9			9
#define ADC_MEAN_ARRAY_R10			10
#define ADC_MEAN_ARRAY_R11			11
#define ADC_MEAN_ARRAY_R12			12
#define ADC_MEAN_ARRAY_R13			13
#define ADC_MEAN_ARRAY_R14			14
#define ADC_MEAN_ARRAY_R15			15
#define ADC_MEAN_ARRAY_DAC			16
#define ADC_MEAN_ARRAY_VINTREF		17

#define VREFINT_CAL      			*((int16_t*)0x1FF800F8)
#define RANGE_12BITS                ((int32_t) 4095)    /* Max digital value with a full range of 12 bits */

#define MEASURE_TICK_TIME_MS		100
#define HV_ZERO_MAX_TIME_MS			10 * MEASURE_TICK_TIME_MS

#define VOLTS_2_mVOLTS				1000

/* Private macro -------------------------------------------------------------*/
#define mVOLTS_TO_DAC_CODE(mV)		(mV) * RANGE_12BITS / (int32_t)systemConfig.kdDivider / Vref_mV
#define DAC_CODE_TO_VOLTS(code)		(float)systemConfig.kdDivider * Vref_mV * (code) / RANGE_12BITS / 1000.

#define CLEAN_MEAN_MEASURE			memset( adcMeanMeasure, 0, ADC_MEAN_ARRAY_LEN * sizeof(adcMeanMeasure[0]) )
#define CLEAN_MEAN_ZERO				memset( adcMeanZero, 0, ADC_MEAN_ARRAY_LEN * sizeof(adcMeanZero[0]) )
#define DAC_FIRST_APPROACH			{ \
										dacValue     = mVOLTS_TO_DAC_CODE(TaskHighVoltage_mV); \
										dacMinValue  = (dacValue *  95 ) / 100; \
										dacMaxValue  = (dacValue * 105 ) / 100; \
										HV_PowerGood = false; \
										setValDACx( dacValue ); \
									}

/* Private variables ---------------------------------------------------------*/
static __IO int16_t   				adcDMABuffer[ADC_DMA_ARRAY_LEN];
static uint32_t						mainMeasureArray[MATRIX_LINEn][MATRIX_RAWn];
static uint32_t						rawAdcCode[MATRIX_RAWn];
static int32_t						adcMeanMeasure[ADC_MEAN_ARRAY_LEN];
static int32_t						adcMeanZero[ADC_MEAN_ARRAY_LEN];
static int32_t						Vref_mV = 3000;
static int32_t						HighVoltage_mV;
static int32_t						TaskHighVoltage_mV;
static int32_t 						dacValue;
static int32_t 						dacMinValue;
static int32_t 						dacMaxValue;
static uint32_t						errorCode = MEASURE_NOERROR;
static uint32_t						errorDetect_debug;
static bool							HV_PowerGood = false;
static bool							doCheck = false;
currentMeasureMode_t 				currentMode;

/* Private function prototypes -----------------------------------------------*/
static void 						getRawAdcCode( void );
static void 						getVrefHV( void );
static void							getAmplifireZero( void );
static void 						getCurrentByLine( Line_NumDef LineNum );
static void 						setHV( void );
static void 						getCapacitanceByLine( Line_NumDef LineNum, RMux_StateDef mux );
static currentMeasureMode_t 		testModeProcess( bool * p_firstStep );
static currentMeasureMode_t 		measureModeProcess( bool * firstStep );
static currentMeasureMode_t 		checkModeProcess( bool * p_firstStep );

/* Private functions ---------------------------------------------------------*/

uint32_t * 							getMeasureData(void) 			{ return  &mainMeasureArray[0][0]; }
uint32_t * 							getRawAdc(void) 				{ return  rawAdcCode; }
int32_t    							getVrefmV(void) 				{ return  Vref_mV; }
uint32_t   							getErrorCode(void) 				{ return  errorCode; }
uint32_t   							getHighVoltagemV(void) 			{ return  (uint32_t)HighVoltage_mV; }
uint32_t   							getDMAErrorCounter(void) 		{ return  errorDetect_debug; }
currentMeasureMode_t   				getCurrentMeasureMode(void) 	{ return  currentMode; }



/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
void MeasureThread(const void *argument)
{
	bool					firstModeStep 			= false;
	static uint32_t 		MeasureThreadCounter 	= 0;

	// Vref dFilter load
	for(uint8_t i = 0; i < 10; i++, getVrefHV()) ;

	/* continue test if terminated by reset */
	if( systemConfig.sysStatus == ACTIVE_STATUS )
	{
		currentMode		= testMode;
		firstModeStep 	= true;
	}
	else
	{
		currentMode		= stopMode;
		firstModeStep 	= false;
	}


	/* measure circle */
	for( TickType_t startTime = 0, measTickDelayMs = 0, passTime = 0; ; passTime = xTaskGetTickCount() - startTime )
	{
		/* wait for events or MEASURE_TICK_TIME_MS */
		measTickDelayMs = (MEASURE_TICK_TIME_MS > pdTick_to_MS(passTime)) ? MEASURE_TICK_TIME_MS - pdTick_to_MS(passTime) : 1;
		osEvent event   = osSignalWait( MEASURE_THREAD_STARTTEST_Evt | MEASURE_THREAD_STOPTEST_Evt |
										MEASURE_THREAD_PAUSETEST_Evt | MEASURE_THREAD_STARTMESURE_Evt |
										MEASURE_THREAD_CHECK_Evt, measTickDelayMs );

		startTime       = xTaskGetTickCount();

		CLEAR_ALL_EVENTS;

		/* process events */

		/* STOP */
		if( event.value.signals & MEASURE_THREAD_STOPTEST_Evt )
		{
			event.value.signals 	=	0;
			MeasureThreadCounter	=	0;
			firstModeStep 			= 	true;
			currentMode				= 	stopMode;
		}

		/* PAUSE */
		if( event.value.signals & MEASURE_THREAD_PAUSETEST_Evt )
		{
			event.value.signals 	=	0;
			MeasureThreadCounter	=	0;
			firstModeStep 			=  	true;
			currentMode				=	pauseMode;
		}

		/* START TEST */
		if( event.value.signals & MEASURE_THREAD_STARTTEST_Evt )
		{
			event.value.signals 	=	0;
			MeasureThreadCounter	=	0;
			firstModeStep 			=  	true;
			doCheck					=   true;
			currentMode				=	testMode;

			errorDetect_debug		=	0;
		}

		/* START MEASURE MANUAL */
		if( event.value.signals & MEASURE_THREAD_STARTMESURE_Evt )
		{
			event.value.signals 	=	0;
			MeasureThreadCounter	=	0;


			if( currentMode	!=	measureMode) // if already measuring
			{
				firstModeStep 		= 	true;
				currentMode			=	measureMode;
			}
		}

		/* CHECK */
		if( event.value.signals & MEASURE_THREAD_CHECK_Evt )
		{
			event.value.signals 	=	0;
			MeasureThreadCounter	=	0;

			if( currentMode	!=	checkMode) // if already measuring
			{
				firstModeStep 		= 	true;
				currentMode			=	checkMode;
			}
		}

		/*  process MEASURE TICK  */
		if( event.status == osEventTimeout )
		{
			MeasureThreadCounter   +=	MEASURE_TICK_TIME_MS;

			/* measure Vref & HighVoltage */
			getVrefHV();

			/* stab HV error */
			setHV();

			currentMeasureMode_t 	newMode;

			/*   process modes */
			switch(currentMode)
			{
				case stopMode:	if( firstModeStep )
								{
									firstModeStep 			= 	false;
									TaskHighVoltage_mV 		= 	0 * VOLTS_2_mVOLTS;
									DAC_FIRST_APPROACH;
									BSP_CTS_SetAllLineDischarge();
									osSignalSet( USBThreadHandle, USB_THREAD_TESTSTOPPED_Evt );
								}

								if( (MeasureThreadCounter == HV_ZERO_MAX_TIME_MS ) && (HighVoltage_mV > 5000) )
								{
									errorCode     =  MEASURE_HV_ZERO_ERROR;
									osSignalSet( USBThreadHandle, USB_THREAD_MEASUREERROR_Evt );
								}
								break;

				case pauseMode:	if( firstModeStep )
								{
									firstModeStep 			= 	false;
									TaskHighVoltage_mV 		= 	0;
									DAC_FIRST_APPROACH;
									BSP_CTS_SetAllLineDischarge();
									osSignalSet( USBThreadHandle, USB_THREAD_TESTPAUSED_Evt );
								}

								if( (MeasureThreadCounter == HV_ZERO_MAX_TIME_MS ) && (HighVoltage_mV > 5000) )
								{
									errorCode     =  MEASURE_HV_ZERO_ERROR;
									osSignalSet( USBThreadHandle, USB_THREAD_MEASUREERROR_Evt );
								}
								break;


				case testMode:
								newMode = testModeProcess( &firstModeStep );
								if( currentMode != newMode )
								{
									MeasureThreadCounter	=	0;
									currentMode 			= 	newMode;
								}
								break;

				case checkMode:
								newMode = checkModeProcess( &firstModeStep );
								if( currentMode != newMode )
								{
									MeasureThreadCounter	=	0;
									currentMode 			= 	newMode;
								}
								break;

				case measureMode:
								newMode = measureModeProcess( &firstModeStep );
								if( currentMode != newMode )
								{
									MeasureThreadCounter	=	0;
									currentMode 			= 	newMode;
								}
								break;

				case idleMode:	if( firstModeStep )
								{
									firstModeStep 			= 	false;
									TaskHighVoltage_mV 		= 	0;
									DAC_FIRST_APPROACH;
									BSP_CTS_SetAllLineDischarge();
									osSignalSet( USBThreadHandle, USB_THREAD_IDLEMODE_Evt );
								}

								if( (MeasureThreadCounter == HV_ZERO_MAX_TIME_MS ) && (HighVoltage_mV > 5000) )
								{
									errorCode     =  MEASURE_HV_ZERO_ERROR;
									osSignalSet( USBThreadHandle, USB_THREAD_MEASUREERROR_Evt );
								}
								break;

				case errorMode:	if( firstModeStep )
								{
									firstModeStep 			=	false;
									TaskHighVoltage_mV 		=	0 * VOLTS_2_mVOLTS;
									DAC_FIRST_APPROACH;
									BSP_CTS_SetAllLineDischarge();
									osSignalSet( USBThreadHandle, USB_THREAD_MEASUREERROR_Evt );
								}

								if( (MeasureThreadCounter == HV_ZERO_MAX_TIME_MS ) && (HighVoltage_mV > 5000) )
								{
									errorCode  =  MEASURE_HV_ZERO_ERROR;
									osSignalSet( USBThreadHandle, USB_THREAD_MEASUREERROR_Evt );
								}
								break;
			}
		}
	}
}


/*
 *
 */
static currentMeasureMode_t testModeProcess( bool * p_firstStep )
{
	static testModeState_t 	testModeState;
	static uint32_t 		testModeCounter;
	static Line_NumDef		LineNum;
	static bool				testStartedF = false;

	/* test mode timer */
	testModeCounter +=	MEASURE_TICK_TIME_MS;

	/* test mode ini */
	if( *p_firstStep )
	{
		*p_firstStep 		= false;

		testStartedF 		= false;
		testModeState 		= hvStabT;
		testModeCounter 	= 0;
		TaskHighVoltage_mV	= systemConfig.uTestVol * VOLTS_2_mVOLTS;
		DAC_FIRST_APPROACH;
		BSP_CTS_SetAllLineDischarge();
	}

	/* test mode process */
	switch( testModeState )
	{
	case hvStabT:	if( testModeCounter <  systemConfig.HVMaxSettleTimeMs ) return testMode;

					if( HV_PowerGood )
					{
						// signal check HV ready
						osSignalSet( USBThreadHandle, USB_THREAD_CHECKHV_Evt );

						// zero of amplifier
						getAmplifireZero();

						testModeCounter = 0;
						LineNum 		= LineAD0;

						if(doCheck)
						{
							doCheck 			= false;
							BSP_CTS_SetSingleLine( LineNum );

							testModeState		= checkLines;
						}
						else
						{
							BSP_CTS_SetAnyLine( AllLineAD, Line_HV, Opto_Open );
							testModeState		= runTest;
						}
						return testMode;
					}

					// system error - unable set HV without load
					// set error code
					errorCode 			= MEASURE_HV_ERROR;
					// to error mode
					*p_firstStep 		= true;
					return errorMode;

	case checkLines:
					if( testModeCounter < systemConfig.dischargeTimeMs ) return testMode;

					if( HV_PowerGood )
					{
						// measure resistance LineNum
						getCurrentByLine(LineNum);

						if( ++LineNum == ADLINEn )
						{
							// signal check current max trsh ready
							osSignalSet( USBThreadHandle, USB_THREAD_CHECKCURRENT_Evt );

							// go to check cap mode
							TaskHighVoltage_mV 	= 0;
							DAC_FIRST_APPROACH

							testModeCounter 	= 0;
							LineNum 			= LineAD0;
							BSP_CTS_SetAllLineDischarge();

							testModeState	= checkCapacitanceTrsh;
							return testMode;
						}

						testModeCounter = 0;
						BSP_CTS_SetSingleLine( LineNum );
						return testMode;
					}

					/* Raw break detect */
					getRawAdcCode();
					// set error code
					errorCode 			= MEASURE_SET_ERROR_CODE(MEASURE_CHANEL_ERROR) | MEASURE_SET_ERROR_LINE(LineNum);
					// to error mode
					*p_firstStep 		= true;
					return errorMode;

	case checkCapacitanceTrsh:
					if( testModeCounter < systemConfig.dischargeTimeMs ) return testMode;

					// select line
					BSP_CTS_SetSingleLine   ( LineNum );
					osDelay					( 50 );

					// start triangle for Mux_1_8
					BSP_SET_RMUX( Mux_1_8 );
					configDACxTriangleMode	( 0 , systemConfig.dacTriangleAmplitude );
					osDelay					( 5 );
					getCapacitanceByLine	( LineNum, Mux_1_8 );  // ~ 10 ms

					// discharge HV
					configDACxStatMode		( 0 );
					osDelay					( 50 );

					// start triangle for Mux_9_16
					BSP_SET_RMUX( Mux_9_16 );
					configDACxTriangleMode	( 0 , systemConfig.dacTriangleAmplitude );
					osDelay					( 5 );
					getCapacitanceByLine 	( LineNum, Mux_9_16 ); // ~ 10 ms

					// discharge HV
					configDACxStatMode		( 0 );

					if( ++LineNum == ADLINEn )
					{
						// signal check cap min trsh ready
						osSignalSet( USBThreadHandle, USB_THREAD_CHECKCAP_Evt );

						// go to test mode
						testModeCounter 	= 0;
						TaskHighVoltage_mV 	= systemConfig.uTestVol * VOLTS_2_mVOLTS;
						DAC_FIRST_APPROACH;
						BSP_CTS_SetAnyLine( AllLineAD, Line_HV, Opto_Open );

						testModeState		= runTest;

						return testMode;
					}

					return testMode;


	case runTest:	if( testModeCounter < systemConfig.dischargeTimeMs * MATRIX_LINEn ) return testMode;

					if( !HV_PowerGood )
					{
						// signal error
						errorCode    = MEASURE_SET_ERROR_CODE(MEASURE_HV_UNSTABLE_ERROR);
						osSignalSet( USBThreadHandle, USB_THREAD_MEASUREERROR_Evt );
						// restart test mode to find error line/raw
						*p_firstStep = true;
						doCheck      = true;
					}
					else
					{
						if( !testStartedF )
						{
							testStartedF 		= true;
							errorCode 			= MEASURE_NOERROR;
							testModeState		= runTest;
							osSignalSet( USBThreadHandle, USB_THREAD_TESTSTARTED_Evt );
						}
					}
					return testMode;
	}

	return testMode;
}

/*
 *
 */
static currentMeasureMode_t checkModeProcess( bool * p_firstStep )
{
	static checkModeState_t 	checkModeState;
	static uint32_t 			checkModeCounter;
	static Line_NumDef			LineNum;

	/* mode timer */
	checkModeCounter +=	MEASURE_TICK_TIME_MS;

	/* test mode ini */
	if( *p_firstStep )
	{
		*p_firstStep 		= false;

		checkModeState 		= checkHVStab;
		checkModeCounter 	= 0;
		TaskHighVoltage_mV	= systemConfig.uTestVol * VOLTS_2_mVOLTS;
		DAC_FIRST_APPROACH;
		BSP_CTS_SetAllLineDischarge();
	}

	/* test mode process */
	switch( checkModeState )
	{
	case checkHVStab:
					if( checkModeCounter <  systemConfig.HVMaxSettleTimeMs ) return checkMode;

					if( HV_PowerGood )
					{
						// signal check HV ready
						osSignalSet( USBThreadHandle, USB_THREAD_CHECKHV_Evt );

						// zero of amplifier
						getAmplifireZero();

						checkModeCounter = 0;
						LineNum 		 = LineAD0;
						checkModeState	 = checkLinesAndLikage;
						BSP_CTS_SetSingleLine( LineNum );
						return checkMode;
					}

					// system error - unable set HV without load
					// set error code
					errorCode 			= MEASURE_HV_ERROR;
					// to error mode
					*p_firstStep 		= true;
					return errorMode;

	case checkLinesAndLikage:
					if( checkModeCounter < systemConfig.dischargeTimeMs ) return checkMode;

					if( HV_PowerGood )
					{
						// measure resistance LineNum
						getCurrentByLine(LineNum);

						if( ++LineNum == ADLINEn )
						{
							// signal check current max trsh ready
							osSignalSet( USBThreadHandle, USB_THREAD_CHECKCURRENT_Evt );

							// go to check cap mode
							TaskHighVoltage_mV 	= 0 * VOLTS_2_mVOLTS;
							DAC_FIRST_APPROACH

							checkModeCounter 	= 0;
							LineNum 			= LineAD0;
							BSP_CTS_SetAllLineDischarge();

							checkModeState	= checkCapacitanceTrsh;
							return checkMode;
						}

						checkModeCounter = 0;
						BSP_CTS_SetSingleLine( LineNum );
						return checkMode;
					}

					/* Raw break detect */
					getRawAdcCode();
					// set error code
					errorCode 			= MEASURE_SET_ERROR_CODE(MEASURE_CHANEL_ERROR) | MEASURE_SET_ERROR_LINE(LineNum);
					// to error mode
					*p_firstStep 		= true;
					return errorMode;

	case checkCapacitanceTrsh:
					if( checkModeCounter < systemConfig.dischargeTimeMs ) return checkMode;

					// select line
					BSP_CTS_SetSingleLine( LineNum );
					osDelay					( 50 );

					// start triangle for Mux_1_8
					BSP_SET_RMUX( Mux_1_8 );
					configDACxTriangleMode	( 0 , systemConfig.dacTriangleAmplitude );
					osDelay					( 5 );
					getCapacitanceByLine	( LineNum, Mux_1_8 );  // ~ 10 ms

					// discharge HV
					configDACxStatMode		( 0 );
					osDelay					( 50 );

					// start triangle for Mux_9_16
					BSP_SET_RMUX( Mux_9_16 );
					configDACxTriangleMode	( 0 , systemConfig.dacTriangleAmplitude );
					osDelay					( 5 );
					getCapacitanceByLine 	( LineNum, Mux_9_16 ); // ~ 10 ms

					// discharge HV
					configDACxStatMode		( 0 );

					if( ++LineNum == ADLINEn )
					{
						// signal check cap min trsh ready
						osSignalSet( USBThreadHandle, USB_THREAD_CHECKCAP_Evt );

						//  return to previous mode
						if( systemConfig.sysStatus == ACTIVE_STATUS )
						{
							*p_firstStep 	=	true;
							return testMode;
						}
						else
						{
							TaskHighVoltage_mV = 0 * VOLTS_2_mVOLTS;
							DAC_FIRST_APPROACH;
							return stopMode;
						}
					}

					return checkMode;
	}

	return checkMode;
}

/*
 *
 */
static currentMeasureMode_t measureModeProcess( bool * p_firstStep )
{
	static measureModeState_t 	measureModeState;
	static uint32_t 			measureModeCounter;
	static Line_NumDef			LineNum;

	/* measure mode timer */
	measureModeCounter +=	MEASURE_TICK_TIME_MS;

	/* measure mode ini */
	if( *p_firstStep )
	{
		*p_firstStep 		= 	false;

		measureModeState 	= 	measureZeroShift;
		measureModeCounter	= 	0;
		TaskHighVoltage_mV 	=	0 * VOLTS_2_mVOLTS;
		DAC_FIRST_APPROACH;
		BSP_CTS_SetAllLineDischarge();
	}

	/* measure mode process */
	switch( measureModeState )
	{
	case measureZeroShift:
					if( measureModeCounter < systemConfig.dischargeTimeMs ) return measureMode;

					// zero of amplifier
					getAmplifireZero();

					// set measure Voltage
					measureModeCounter 	= 	0;
					TaskHighVoltage_mV 	=	systemConfig.uMeasureVol * VOLTS_2_mVOLTS;
					DAC_FIRST_APPROACH;
					measureModeState	= 	hvStabM;
					BSP_CTS_SetAllLineDischarge();
					return measureMode;

	case hvStabM:	if( measureModeCounter <  systemConfig.HVMaxSettleTimeMs ) return measureMode;

					if( HV_PowerGood )
					{
						// signal check HV ready
						osSignalSet( USBThreadHandle, USB_THREAD_CHECKHV_Evt );
						osDelay(10);

						measureModeCounter 	= 0;
						LineNum 			= LineAD0;
						measureModeState	= measureResistance;
						BSP_CTS_SetSingleLine( LineNum );
						// signal measure started
						errorCode 		= 	MEASURE_NOERROR;
						osSignalSet( USBThreadHandle, USB_THREAD_MEASURESTARTED_Evt );
						return measureMode;
					}

					// system error - unable set HV without load
					// set error code
					errorCode 			= MEASURE_HV_ERROR;
					// to error mode
					*p_firstStep 		= true;
					return errorMode;

	case measureResistance:
					if( measureModeCounter < systemConfig.dischargeTimeMs ) break;

					// check HV Power
					if( HV_PowerGood )
					{
						// measure resistance LineNum
						getCurrentByLine(LineNum);

						if( ++LineNum == ADLINEn )
						{
							// signal measure end
							errorCode 		= 	MEASURE_NOERROR;
							osSignalSet( USBThreadHandle, USB_THREAD_MEASUREREADY_Evt );
							//  return to previous mode
							if( systemConfig.sysStatus == ACTIVE_STATUS )
							{
								*p_firstStep 	=	true;
								return testMode;
							}
							else
							{
								*p_firstStep 		 =	true;
								return idleMode;
							}
						}

						// set HV on next line, discharge others
						measureModeCounter 	= 0;
						BSP_CTS_SetSingleLine( LineNum );
						return measureMode;
					}

					/* Raw break detect */
					getRawAdcCode();
					// set error code
					errorCode 			= MEASURE_SET_ERROR_CODE(MEASURE_CHANEL_ERROR) | MEASURE_SET_ERROR_LINE(LineNum);
					// to error mode
					*p_firstStep 		= true;
					return errorMode;
		}

	return measureMode;
}


/* HV control */
static void setHV(void)
{
	static uint8_t HW_GoodCounter  = 0;
	static uint8_t HW_BadCounter   = 0;

	/* HV_PowerGood flag detect */
	if( abs( TaskHighVoltage_mV - HighVoltage_mV ) <  systemConfig.MaxErrorHV_mV )
	{
		if( ++HW_GoodCounter > 5)
		{
			HV_PowerGood 	= true;
			HW_GoodCounter 	= 5;
			HW_BadCounter	= 0;
		}
	}
	else
	{
		if( ++HW_BadCounter > 5)
		{
			HV_PowerGood 	= false;
			HW_GoodCounter 	= 0;
			HW_BadCounter	= 5;
		}
	}

	/* HV stab procedure
	 *
	if( abs( TaskHighVoltage_mV - HighVoltage_mV ) > systemConfig.MaxErrorHV_mV / 2 )
	{
		int32_t		err_dac = ( TaskHighVoltage_mV - HighVoltage_mV ) * RANGE_12BITS / (int32_t)systemConfig.kdDivider / Vref_mV;

		if( (dacValue + err_dac) > dacMaxValue ) 		dacValue  = dacMaxValue;
		else if( (dacValue + err_dac) < dacMinValue ) 	dacValue  = dacMinValue;
		else											dacValue += err_dac;

		setValDACx( dacValue );
	}
	*/
}

/* measure Vref & HighVoltage */
static void getVrefHV(void)
{
	CLEAN_MEAN_MEASURE;

	for( uint32_t i = 0; i < VREF_MEAN_FACTOR; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		HAL_ADC_Stop( &AdcHandle );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			adcMeanMeasure[ADC_MEAN_ARRAY_DAC] 	   += adcDMABuffer[ADC_DMA_ARRAY_DAC];
			adcMeanMeasure[ADC_MEAN_ARRAY_VINTREF] += adcDMABuffer[ADC_DMA_ARRAY_VINTREF];
			i++;
		}
	}

	/* calc Vref & HV */

	// Vref_mV
	float f_val		= VREF_MEAN_FACTOR * 3000. * VREFINT_CAL / adcMeanMeasure[ADC_MEAN_ARRAY_VINTREF];
	f_val   	   += 0.5 * ( f_val - Vref_mV); // dig filter k = 0.5
	Vref_mV 	    = f_val + 0.5;

	// HighVoltage_mV
	f_val			= (float) adcMeanMeasure[ADC_MEAN_ARRAY_DAC] / VREF_MEAN_FACTOR / RANGE_12BITS * systemConfig.kdDivider * f_val;
	HighVoltage_mV	=  f_val + 0.5;
}


/* measure overload of ampl */
static void getRawAdcCode(void)
{
	CLEAN_MEAN_MEASURE;
	/* channels 1..8 */
	BSP_SET_RMUX(Mux_1_8);
	osDelay( systemConfig.IAmplifierSettleTimeMs );

	for( uint32_t i = 0; i < systemConfig.adcMeanFactor; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		HAL_ADC_Stop( &AdcHandle );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[j * 2] += adcDMABuffer[j];
			i++;
		}
	}

	/* channels 9..16 */
	BSP_SET_RMUX(Mux_9_16);
	osDelay( systemConfig.IAmplifierSettleTimeMs );

	for( uint32_t i = 0; i < systemConfig.adcMeanFactor; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		HAL_ADC_Stop( &AdcHandle );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[j * 2 + 1] += adcDMABuffer[j];
			i++;
		}
	}

	for(uint32_t i = 0; i < MATRIX_RAWn; i++)
	{
		rawAdcCode[i] = adcMeanMeasure[i] / systemConfig.adcMeanFactor;
	}
}

/* get zero shift */
static void	getAmplifireZero( void )
{
	CLEAN_MEAN_ZERO;
	BSP_SET_OPTO( Opto_Close );	// disconnect All ZV capacitors from HV driver
	osDelay( systemConfig.optoSignalSettleTimeMs );

	/* channels 1..8 */
	BSP_SET_RMUX(Mux_1_8);
	osDelay( systemConfig.IAmplifierSettleTimeMs );

	for( uint32_t i = 0; i < systemConfig.adcMeanFactor; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		HAL_ADC_Stop( &AdcHandle );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanZero[j * 2] += adcDMABuffer[j];
			i++;
		}
		else
			errorDetect_debug++;
	}

	/* channels 9..16 */
	BSP_SET_RMUX(Mux_9_16);
	osDelay( systemConfig.IAmplifierSettleTimeMs );

	for( uint32_t i = 0; i < systemConfig.adcMeanFactor; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		HAL_ADC_Stop( &AdcHandle );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanZero[j * 2 + 1] += adcDMABuffer[j];
			i++;
		}
		else
			errorDetect_debug++;
	}
}

/* get current - one line*/
static void getCurrentByLine( Line_NumDef LineNum )
{
	// if( systemConfig.measureMask & (1 << LineNum) ) return;

	CLEAN_MEAN_MEASURE;
	BSP_SET_OPTO( Opto_Close ); // disconnect All ZV capacitors from HV driver
	osDelay( systemConfig.optoSignalSettleTimeMs );

	/* channels 1..8 */
	BSP_SET_RMUX(Mux_1_8);
	osDelay( systemConfig.IAmplifierSettleTimeMs );

	for( uint32_t i = 0; i < systemConfig.adcMeanFactor; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		HAL_GPIO_WritePin( GPIOA, GPIO_PIN_15, GPIO_PIN_SET );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		HAL_ADC_Stop( &AdcHandle );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[j * 2] += adcDMABuffer[j];
			i++;
		}
		else
			errorDetect_debug++;
	}

	/* channels 9..16 */
	BSP_SET_RMUX(Mux_9_16);
	osDelay( systemConfig.IAmplifierSettleTimeMs );

	for( uint32_t i = 0; i < systemConfig.adcMeanFactor; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		HAL_GPIO_WritePin( GPIOA, GPIO_PIN_15, GPIO_PIN_SET );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		HAL_ADC_Stop( &AdcHandle );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[j * 2 + 1] += adcDMABuffer[j];
			i++;
		}
		else
			errorDetect_debug++;
	}

	// BSP_SET_RMUX(Mux_1_8);
	/* correct zero shift and calc */
	float ki_V_nA    = 1e9 / systemConfig.kiAmplifire;
	float adc_code_V = Vref_mV / 1000. / RANGE_12BITS;

	for(uint32_t i = 0; i < MATRIX_RAWn; i++)
	{
		if( adcMeanMeasure[i] > adcMeanZero[i] ) 	adcMeanMeasure[i]  = (adcMeanMeasure[i] - adcMeanZero[i]);
		else										adcMeanMeasure[i]  = 0;

		float current_nA = adcMeanMeasure[i] * adc_code_V * ki_V_nA / systemConfig.adcMeanFactor + 0.1;

		mainMeasureArray[LineNum][i] = current_nA + 0.5;
	}
}

/* get capacitance - one line, one mux stage */
static void getCapacitanceByLine( Line_NumDef LineNum, RMux_StateDef mux )
{

	BSP_SET_OPTO( Opto_Close ); // disconnect All ZV capacitors from amplifiers
	osDelay( systemConfig.optoSignalSettleTimeMs );

	CLEAN_MEAN_MEASURE;

	float amplitude_V	 = DAC_CODE_TO_VOLTS(systemConfig.dacTriangleAmplitude);
	float slopeTime_s	 = (float)(systemConfig.dacTriangleAmplitude + 1) * systemConfig.dacTrianglePeriodUs / 1000000.;
	float ki_V_pA    	 = 1e12 / systemConfig.kiAmplifire;
	float adc_code_V 	 = Vref_mV / 1000. / RANGE_12BITS;
	float mCoeff		 = adc_code_V * ki_V_pA / TRIANGLE_MEAN_FACTOR;
	uint32_t muxFactor	 = (mux == Mux_9_16 ? 1 : 0);

	// for( uint32_t i = 0; i < systemConfig.adcMeanFactor; )
	for( uint32_t i = 0; i < TRIANGLE_MEAN_FACTOR; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		HAL_ADC_Stop( &AdcHandle );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[j * 2 + muxFactor] += adcDMABuffer[j];
			i++;
		}
	}

	/* correct zero shift and calc */
	for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++)
	{
		uint32_t zeroShift	=  adcMeanZero[j * 2 + muxFactor] * TRIANGLE_MEAN_FACTOR / systemConfig.adcMeanFactor;

		if( adcMeanMeasure[j * 2 + muxFactor] > zeroShift )
			adcMeanMeasure[j * 2 + muxFactor]  = (adcMeanMeasure[j * 2 + muxFactor] - zeroShift);
		else
			adcMeanMeasure[j * 2 + muxFactor]  = 0;

		float current_pA     = adcMeanMeasure[j * 2 + muxFactor] * mCoeff;

		mainMeasureArray[LineNum][j * 2 + muxFactor] = current_pA / amplitude_V * slopeTime_s + .5; // capacitance pF
	}
}
