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

extern 	__IO sysCfg_t				systemConfig;
extern  __IO dataAttribute_t		dataAttribute[];
extern 	__IO dataMeasure_t			dataMeasure[];

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ADC_DMA_ARRAY_LEN			10

#define ADC_MEAN_FACTOR				16

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

#define VREFINT_CAL      			*((uint16_t*)0x1FF800F8)
#define RANGE_12BITS                ((uint32_t) 4095)    /* Max digital value with a full range of 12 bits */

#define MEASURE_TICK_TIME_MS		100
#define HV_SETTING_TIME_LIMIT_MS	10 * MEASURE_TICK_TIME_MS
/* Private macro -------------------------------------------------------------*/
#define CLEAN_MEAN_MEASURE			for( uint32_t i = 0; i < ADC_DMA_ARRAY_LEN; i++ ) 	adcMeanMeasure[i] = 0
#define CLEAN_MEAN_ZERO				for( uint32_t i = 0; i < ADC_DMA_ARRAY_LEN; i++ ) 	adcMeanZero[i] = 0
/* Private variables ---------------------------------------------------------*/
static __IO uint16_t   				adcDMABuffer[ADC_DMA_ARRAY_LEN];
static uint32_t						adcMeanMeasure[ADC_DMA_ARRAY_LEN];
static uint32_t						adcMeanZero[ADC_DMA_ARRAY_LEN];
static uint32_t						Vref_mV = 3000;
static int32_t						HighVoltage_mV;
static int32_t						TaskHighVoltage_mV;
static uint32_t						hvSettingTimer_mS;
static int32_t 						dacValue;
static uint32_t						errorCode = MEASURE_NOERROR;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/


/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
void MeasureThread(const void *argument)
{
	bool 		testMode 		= false;
	bool 		measureMode 	= false;
	uint32_t	lineNumSetOn	= 0;

	/* set adc & dma */
	HAL_ADC_Start_DMA( &AdcHandle, (uint32_t *)adcDMABuffer, ADC_DMA_ARRAY_LEN );
	osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );

	/* continue test if terminated by reset */
	if(systemConfig.sysStatus == ACTIVE_STATUS)
	{
		testMode 			= true;
		TaskHighVoltage_mV 	= systemConfig.uTestVol * 1000;
		osDelay(100);
	}

	/* measure circle */
	for(;;)
	{
		/* measure Vref & HighVoltage */
		{
			CLEAN_MEAN_MEASURE;
			for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
			{
				HAL_ADC_Start( &AdcHandle );
				osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );

				if( event.value.signals == MEASURE_THREAD_CONVCMPLT_Evt )
				{
					i++;
					adcMeanMeasure[ADC_DMA_ARRAY_DAC] 	  += adcDMABuffer[ADC_DMA_ARRAY_DAC];
					adcMeanMeasure[ADC_DMA_ARRAY_VINTREF] += adcDMABuffer[ADC_DMA_ARRAY_VINTREF];
				}
			}

			Vref_mV 	   += (10 * 3000 * (adcMeanMeasure[ADC_DMA_ARRAY_VINTREF] / ADC_MEAN_FACTOR) / VREFINT_CAL - 10 * Vref_mV) / 20 ; // dig filter k = 0.5
			HighVoltage_mV	=  Vref_mV * (adcMeanMeasure[ADC_DMA_ARRAY_DAC] / ADC_MEAN_FACTOR) * systemConfig.kdDivider / RANGE_12BITS;
		}

		/* wait for events or MEASURE_TICK_TIME_MS */
		osEvent event = osSignalWait( MEASURE_THREAD_STARTTEST_Evt | MEASURE_THREAD_TESTFINISH_Evt | MEASURE_THREAD_STARTMESURE_Evt, MEASURE_TICK_TIME_MS );

		/* process events */
		switch( event.value.signals )
		{
		case MEASURE_THREAD_STARTTEST_Evt:	testMode 			= true;
											TaskHighVoltage_mV 	= systemConfig.uTestVol * 1000;
											hvSettingTimer_mS 	= 0;
											lineNumSetOn		= 0;
											BSP_CTS_SetAnyLine( ADLINEn, Line_ZV, Opto_Open ); // discharge All capacitors
											break;

		case MEASURE_THREAD_TESTFINISH_Evt:	testMode 			= false;
											TaskHighVoltage_mV 	= 0;
											hvSettingTimer_mS   = 0;
											lineNumSetOn		= 0;
											BSP_CTS_SetAnyLine( ADLINEn, Line_ZV, Opto_Open ); // discharge All capacitors
											break;

		case MEASURE_THREAD_STARTMESURE_Evt:
											measureMode 		= true;
											TaskHighVoltage_mV 	= systemConfig.uMeasureVol * 1000;
											hvSettingTimer_mS   = 0;
											lineNumSetOn		= 0;

											/* get zero shift */
											CLEAN_MEAN_ZERO;
											BSP_CTS_SetAnyLine( ADLINEn, Line_ZV, Opto_Open ); 	// discharge All capacitors
											osDelay( systemConfig.dischargePreMeasureTimeMs );
											BSP_SET_OPTO( Opto_Close );							// disconnect All capacitors from HV driver

											/* channels 1..8 */
											BSP_SET_RMUX(Mux_1_8);
											osDelay( 10 );

											for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
											{
												HAL_ADC_Start( &AdcHandle );
												osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );

												if( event.value.signals == MEASURE_THREAD_CONVCMPLT_Evt )
												{
													for(uint32_t j = ADC_DMA_ARRAY_R0_8; j < (ADC_DMA_ARRAY_R7_15 + 1); j++ ) adcMeanMeasure[j] += adcDMABuffer[j];
													i++;
												}
											}

											/* channels 9..16 */
											BSP_SET_RMUX(Mux_9_16);
											osDelay( 10 );

											for( uint32_t i = 0; i < ADC_MEAN_FACTOR; )
											{
												HAL_ADC_Start( &AdcHandle );
												osEvent event = osSignalWait( MEASURE_THREAD_CONVCMPLT_Evt | MEASURE_THREAD_CONVERROR_Evt, osWaitForever );

												if( event.value.signals == MEASURE_THREAD_CONVCMPLT_Evt )
												{
													for(uint32_t j = ADC_DMA_ARRAY_R0_8; j < (ADC_DMA_ARRAY_R7_15 + 1); j++ ) adcMeanMeasure[j + ADC_DMA_ARRAY_R7_15 + 1] += adcDMABuffer[j];
													i++;
												}
											}

											break;

		}

		/* get HV error */
		int32_t errHV_mV = TaskHighVoltage_mV - HighVoltage_mV;

		/*  process MEASURE TICK  */
		if( event.status == osEventTimeout )
		{
			/*  measure Mode  */
			if( measureMode )
			{
				/* check HV set	*/
				if( abs(errHV_mV) > systemConfig.MaxErrorHV_mV )
				{
					hvSettingTimer_mS += MEASURE_TICK_TIME_MS;

					if( hvSettingTimer_mS > HV_SETTING_TIME_LIMIT_MS )
					{
						/* Error */
						measureMode = testMode = false;
						errorCode   = MEASURE_HV_ERROR;
						osSignalSet( USB_THREAD_TESTSTARTED_Evt, USB_THREAD_MEASUREERROR_Evt );
					}
				}
				else
				{
					/*	do measure	*/
					/*
					 *
					 * TODO test process
					 *
					 */
					measureMode 		= false;
					BSP_CTS_SetAnyLine( ADLINEn, Line_ZV, Opto_Open ); // discharge All

					if( testMode )
					{
						hvSettingTimer_mS   = 0;
						TaskHighVoltage_mV 	= systemConfig.uTestVol * 1000;
					}
					else
					TaskHighVoltage_mV 	= 0;
				}
			}
			else
			/*  test Mode  */
			if( testMode )
			{
				/* check HV set	*/
				if( abs(errHV_mV) > systemConfig.MaxErrorHV_mV )
				{
					hvSettingTimer_mS += MEASURE_TICK_TIME_MS;

					if( hvSettingTimer_mS > HV_SETTING_TIME_LIMIT_MS )
					{
						/* Error */
						measureMode = testMode = false;
						errorCode   = MEASURE_HV_ERROR;
						osSignalSet( USB_THREAD_TESTSTARTED_Evt, USB_THREAD_MEASUREERROR_Evt );
					}
				}
				else
				{
					/* do test mode */
					hvSettingTimer_mS   = 0;
					if( lineNumSetOn != ADLINEn)
					{
						if( !(systemConfig.measureMask & (1<<lineNumOn)) ) BSP_CTS_SetAnyLine( ADLINEn, lineNumOn, Opto_Open );
						lineNumSetOn++;
						if( lineNumSetOn == ADLINEn ) osSignalSet( USB_THREAD_TESTSTARTED_Evt, USB_THREAD_TESTSTARTED_Evt );
					}
				}
			}
			else
			{

			}
		} /*   process MEASURE TICK  */

		/* HV control */
		if( abs(errHV_mV) > systemConfig.MaxErrorHV_mV / 2 )
		{
			int32_t err_dac = errHV_mV * RANGE_12BITS / (systemConfig.kdDivider * Vref_mV) ;

			if( (dacValue + err_dac) > RANGE_12BITS ) 	dacValue  = RANGE_12BITS;
			else if( (dacValue + err_dac) < 0 ) 		dacValue  = 0;
			else										dacValue += err_dac;

			HAL_DAC_SetValue( &DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, dacValue );
		}
	} // for(;;)
}
