/**
  ******************************************************************************
  * @file    measure_main.c
  * @author  Epta
  * @brief   CTS application measure_main file
  ****************************************************************************** */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Global variables ---------------------------------------------------------*/
extern 	USBD_HandleTypeDef  		USBD_Device;
extern 	osThreadId					USBThreadHandle;
extern 	osThreadId					MeasureThreadHandle;
extern 	osThreadId					OneSecThreadHandle;
extern 	DAC_HandleTypeDef    		DacHandle;
extern 	ADC_HandleTypeDef    		AdcHandle;

extern 	volatile sysCfg_t			systemConfig;
extern  dataAttribute_t				dataAttribute[];
extern 	dataMeasure_t				dataMeasure[];

/* Private typedef -----------------------------------------------------------*/
typedef enum {

	stopMode = 0,
	testMode,
	measureMode,
	errorMode

} currentMode_t;

typedef enum {

	hvStabT = 0,
	checkLines,
	runTest,
	seekBadRaw

} testModeState_t;

typedef enum {

	hvStabM = 0,
	measureZeroShift,
	measureLines

} measureModeState_t;

/* Private define ------------------------------------------------------------*/
#define ADC_DMA_ARRAY_LEN			10
#define ADC_MEAN_ARRAY_LEN			18
#define ADC_MEAN_FACTOR				32

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
#define HV_SETTING_TIME_LIMIT_MS	10 * MEASURE_TICK_TIME_MS
/* Private macro -------------------------------------------------------------*/
#define CLEAN_MEAN_MEASURE			memset( adcMeanMeasure, 0, ADC_MEAN_ARRAY_LEN * sizeof(adcMeanMeasure[0]) )
#define CLEAN_MEAN_ZERO				memset( adcMeanZero, 0, ADC_MEAN_ARRAY_LEN * sizeof(adcMeanZero[0]) )
#define DAC_FIRST_APPROACH			{ \
										dacValue = TaskHighVoltage_mV * RANGE_12BITS / (int32_t)systemConfig.kdDivider / Vref_mV; \
										HAL_DAC_SetValue( &DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, dacValue ); \
										dacMinValue = 0; \
										dacMaxValue = (dacValue * 110 ) / 100; \
										HV_PowerGood = false; \
									}
/* Private variables ---------------------------------------------------------*/
static __IO int16_t   				adcDMABuffer[ADC_DMA_ARRAY_LEN];
static uint32_t						resistanceArrayMOhm[MATRIX_LINEn][MATRIX_RAWn];
static uint32_t						rawCommoncurrent_nA[MATRIX_RAWn];
static int32_t						adcMeanMeasure[ADC_MEAN_ARRAY_LEN];
static int32_t						adcMeanZero[ADC_MEAN_ARRAY_LEN];
static int32_t						Vref_mV = 3000;
static int32_t						HighVoltage_mV;
static int32_t						TaskHighVoltage_mV;
static int32_t 						dacValue;
static int32_t 						dacMinValue;
static int32_t 						dacMaxValue;
static uint32_t						errorCode = MEASURE_NOERROR;
static bool							HV_PowerGood = false;

/* Private function prototypes -----------------------------------------------*/
static void 						getVrefHVRawCommonCurrent( void );
static void							getZeroShifts( void );
static void 						getResistanceOneLine( Line_NumDef LineNum );
static void 						setHV( void );
static currentMode_t 				testModeProcess( bool * p_firstStep );
static currentMode_t 				measureModeProcess( bool * firstStep );
/* Private functions ---------------------------------------------------------*/

uint32_t * getMeasureData(void) 	{ return  &resistanceArrayMOhm[0][0]; }
int32_t    getVrefmV(void) 			{ return  Vref_mV; }
uint32_t   getErrorCode(void) 		{ return  errorCode; }
uint32_t   getHighVoltagemV(void) 	{ return  (uint32_t)HighVoltage_mV; }

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
void MeasureThread(const void *argument)
{
	currentMode_t 	currentMode   = stopMode;
	bool			firstModeStep = false;

	/* continue test if terminated by reset */
	if( systemConfig.sysStatus == ACTIVE_STATUS )
	{
		TaskHighVoltage_mV 	 =  systemConfig.uTestVol * 1000;
		currentMode			 =  testMode;
		firstModeStep 		 =  true;
		getVrefHVRawCommonCurrent();
		DAC_FIRST_APPROACH;
	}

	/* measure circle */
	for( TickType_t startTime = 0, measTickDelayMs = 0, passTime = 0; ; passTime = xTaskGetTickCount() - startTime )
	{
		/* wait for events or MEASURE_TICK_TIME_MS */
		measTickDelayMs = (MEASURE_TICK_TIME_MS > pdTick_to_MS(passTime)) ? MEASURE_TICK_TIME_MS - pdTick_to_MS(passTime) : 1;
		osEvent event   = osSignalWait( MEASURE_THREAD_STARTTEST_Evt | MEASURE_THREAD_STOPTEST_Evt | MEASURE_THREAD_STARTMESURE_Evt, measTickDelayMs );
		startTime       = xTaskGetTickCount();

		CLEAR_ALL_EVENTS;

		/* process events */
		if( event.value.signals & MEASURE_THREAD_STARTTEST_Evt )
		{
			TaskHighVoltage_mV	= 	systemConfig.uTestVol * 1000;
			currentMode			=	testMode;
			firstModeStep 		=  	true;
			DAC_FIRST_APPROACH;
		}

		if( event.value.signals & MEASURE_THREAD_STOPTEST_Evt )
		{
			TaskHighVoltage_mV 	= 	0;
			currentMode			= 	stopMode;
			firstModeStep 		= 	true;
			DAC_FIRST_APPROACH;
		}

		if( event.value.signals & MEASURE_THREAD_STARTMESURE_Evt )
		{
			TaskHighVoltage_mV 	=	systemConfig.uMeasureVol * 1000;
			currentMode			=	measureMode;
			firstModeStep 		=  	true;
			DAC_FIRST_APPROACH;
		}

		/*  process MEASURE TICK  */
		if( event.status == osEventTimeout )
		{
			/* measure Vref & HighVoltage */
			getVrefHVRawCommonCurrent();

			/* stab HV error */
			setHV();

			/*   process modes */
			switch(currentMode)
			{
				case stopMode:	if( firstModeStep )
								{
									BSP_CTS_SetAllLineDischarge();
									if( HighVoltage_mV < 5000 )
									{
										osSignalSet( USBThreadHandle, USB_THREAD_TESTSTOPPED_Evt );
										firstModeStep = false;
									}
								}
								break;

				case testMode:	currentMode = testModeProcess( &firstModeStep );
								break;

				case measureMode:
								measureModeProcess( &firstModeStep );
								break;

				case errorMode:	break;
			}
		}
	}
}


/*
 *
 */
static currentMode_t testModeProcess( bool * p_firstStep )
{
	static testModeState_t 	testModeState;
	static uint32_t 		testModeCounter;
	static Line_NumDef		LineNum;

	testModeCounter +=	MEASURE_TICK_TIME_MS;

	if( *p_firstStep )
	{
		*p_firstStep 	= false;
		testModeState 	= hvStabT;
		testModeCounter = 0;
		BSP_CTS_SetAllLineDischarge();
	}

	switch(testModeState)
	{
	case hvStabT:	if( HV_PowerGood )
					{
						testModeCounter = 0;
						LineNum 		= LineAD0;
						BSP_CTS_SetAnyLine( LineNum, Line_HV, Opto_Open );
						testModeState	= checkLines;
						return testMode;
					}

					if( testModeCounter >  HV_SETTING_TIME_LIMIT_MS )
					{
						errorCode 			= MEASURE_HV_ERROR;
						TaskHighVoltage_mV 	= 0;
						DAC_FIRST_APPROACH;
						osSignalSet( USBThreadHandle, USB_THREAD_MEASUREERROR_Evt );
						return errorMode;
					}
					break;

	case checkLines:
					if( testModeCounter < systemConfig.dischargePreMeasureTimeMs ) break;

					testModeCounter = 0;

					if( HV_PowerGood )
					{
						if( ++LineNum == ADLINEn )
						{
							errorCode 		= MEASURE_NOERROR;
							testModeState	= runTest;
							osSignalSet( USBThreadHandle, USB_THREAD_TESTSTARTED_Evt );
							return testMode;
						}

						BSP_CTS_SetAnyLine( LineNum, Line_HV, Opto_Open );
						return testMode;
					}

					getResistanceOneLine(LineNum);
					errorCode 			= MEASURE_SET_ERROR_CODE(MEASURE_CHANEL_ERROR) | MEASURE_SET_ERROR_LINE(LineNum);
					TaskHighVoltage_mV 	= 0;
					DAC_FIRST_APPROACH;
					osSignalSet( USBThreadHandle, USB_THREAD_MEASUREERROR_Evt );
					return errorMode;

	case runTest:	if( !HV_PowerGood )
					{
						*p_firstStep = true;
					}
					return testMode;
	}

	return testMode;
}

/*
 *
 */
static currentMode_t measureModeProcess( bool * firstStep )
{
	return measureMode;
}

/* HV control */
static void setHV(void)
{
	static uint8_t HW_GoodCounter  = 0;
	static uint8_t HW_BadCounter   = 0;

	if( abs( TaskHighVoltage_mV - HighVoltage_mV ) <  systemConfig.MaxErrorHV_mV )
	{
		if( ++HW_GoodCounter > 3)
		{
			HV_PowerGood 	= true;
			HW_GoodCounter 	= 3;
			HW_BadCounter	= 0;
		}
	}
	else
	{
		if( ++HW_BadCounter > 3)
		{
			HV_PowerGood 	= false;
			HW_GoodCounter 	= 0;
			HW_BadCounter	= 3;
		}
	}

	if( abs( TaskHighVoltage_mV - HighVoltage_mV ) >  systemConfig.MaxErrorHV_mV / 2 )
	{
		int32_t		err_dac = ( TaskHighVoltage_mV - HighVoltage_mV ) * RANGE_12BITS / (int32_t)systemConfig.kdDivider / Vref_mV;

		if( (dacValue + err_dac) > dacMaxValue ) 		dacValue  = dacMaxValue;
		else if( (dacValue + err_dac) < dacMinValue ) 	dacValue  = dacMinValue;
		else											dacValue += err_dac;

		HAL_DAC_SetValue( &DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, dacValue );
	}
}

/* measure Vref & HighVoltage */
static void getVrefHVRawCommonCurrent(void)
{
	CLEAN_MEAN_MEASURE;
	/* channels 1..8 */
	BSP_SET_RMUX(Mux_1_8);
	osDelay( 10 );

	for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[j] += adcDMABuffer[j];
			adcMeanMeasure[ADC_MEAN_ARRAY_DAC] 	   += adcDMABuffer[ADC_DMA_ARRAY_DAC];
			adcMeanMeasure[ADC_MEAN_ARRAY_VINTREF] += adcDMABuffer[ADC_DMA_ARRAY_VINTREF];
			i++;
		}
	}

	/* channels 9..16 */
	BSP_SET_RMUX(Mux_9_16);
	osDelay( 10 );

	for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[ADC_MEAN_ARRAY_R8 + j] += adcDMABuffer[j];
			i++;
		}
	}

	/* calc Vref & HV */
	Vref_mV 	   += ( 10 * 3000 * VREFINT_CAL * ADC_MEAN_FACTOR / adcMeanMeasure[ADC_MEAN_ARRAY_VINTREF] - 10 * Vref_mV) / 20 ; // dig filter k = 0.5
	HighVoltage_mV	=  Vref_mV * adcMeanMeasure[ADC_MEAN_ARRAY_DAC] / ADC_MEAN_FACTOR * systemConfig.kdDivider / RANGE_12BITS;

	/* calc raw Common current */
	float ki_V_nA    = systemConfig.kiAmplifire / 1e9;
	float adc_V_step = Vref_mV / 1000. / RANGE_12BITS;

	for(uint32_t i = 0; i < MATRIX_RAWn; i++)
	{
		if( adcMeanMeasure[i] > adcMeanZero[i] ) 	adcMeanMeasure[i]  = (adcMeanMeasure[i] - adcMeanZero[i]) / ADC_MEAN_FACTOR;
		else										adcMeanMeasure[i]  = 0;

		rawCommoncurrent_nA[i] = (uint32_t)(adcMeanMeasure[i] * adc_V_step * ki_V_nA + 0.5);
	}
}


/* get zero shift */
static void	getZeroShifts( void )
{
	CLEAN_MEAN_ZERO;
	BSP_CTS_SetAnyLine( ADLINEn, Line_ZV, Opto_Open ); 	// discharge All capacitors
	osDelay( systemConfig.dischargePreMeasureTimeMs );
	BSP_SET_OPTO( Opto_Close );							// disconnect All capacitors from HV driver

	/* channels 1..8 */
	BSP_SET_RMUX(Mux_1_8);
	osDelay( 10 );

	for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanZero[j] += adcDMABuffer[j];
			i++;
		}
	}

	/* channels 9..16 */
	BSP_SET_RMUX(Mux_9_16);
	osDelay( 10 );

	for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanZero[ADC_MEAN_ARRAY_R8 + j] += adcDMABuffer[j];
			i++;
		}
	}
}

static void getResistanceOneLine( Line_NumDef LineNum )
{
	if( systemConfig.measureMask & (1 << LineNum) ) return;

	CLEAN_MEAN_MEASURE;
	BSP_CTS_SetSingleLine( LineNum );

	/* channels 1..8 */
	BSP_SET_RMUX(Mux_1_8);
	osDelay( 10 );

	for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[j] += adcDMABuffer[j];
			i++;
		}
	}

	/* channels 9..16 */
	BSP_SET_RMUX(Mux_9_16);
	osDelay( 10 );

	for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
	{
		HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
		osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );
		if( event.value.signals & MEASURE_THREAD_CONVCMPLT_Evt )
		{
			for(uint32_t j = ADC_DMA_ARRAY_R0_8; j <= ADC_DMA_ARRAY_R7_15; j++ ) adcMeanMeasure[ADC_MEAN_ARRAY_R8 + j] += adcDMABuffer[j];
			i++;
		}
	}

	/* correct zero shift and calc */
	float ki_V_nA    = systemConfig.kiAmplifire / 1e9;
	float adc_V_step = Vref_mV / 1000. / RANGE_12BITS;

	for(uint32_t i = 0; i < MATRIX_RAWn; i++)
	{
		if( adcMeanMeasure[i] > adcMeanZero[i] ) 	adcMeanMeasure[i]  = (adcMeanMeasure[i] - adcMeanZero[i]) / ADC_MEAN_FACTOR;
		else										adcMeanMeasure[i]  = 1;

		float current_nA = adcMeanMeasure[i] * adc_V_step * ki_V_nA;
		resistanceArrayMOhm[LineNum][i] = HighVoltage_mV / current_nA + 0.5;
		if(resistanceArrayMOhm[LineNum][i] > 99999) resistanceArrayMOhm[LineNum][i] = 99999;
	}
}



