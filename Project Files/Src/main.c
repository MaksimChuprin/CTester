/**
  ******************************************************************************
  * @file    main.c
  * @author  Epta
  * @brief   CTS application main file
  ****************************************************************************** */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Global variables ---------------------------------------------------------*/
osThreadId					USBThreadHandle;
osThreadId					MeasureThreadHandle;
osThreadId					OneSecThreadHandle;
USBD_HandleTypeDef 			USBD_Device;
DAC_HandleTypeDef    		DacHandle;
ADC_HandleTypeDef    		AdcHandle;

volatile sysCfg_t			systemConfig 						__attribute__((section(".settings")));
systime_t					rtcSaveArray[SAVE_ARRAY_SIZE]		__attribute__((section(".settings")));
uint32_t					testTimeSaveArray[SAVE_ARRAY_SIZE]	__attribute__((section(".settings")));
dataAttribute_t				dataAttribute[STAT_ARRAY_SIZE]  	__attribute__((section(".settings")));
dataMeasure_t				dataMeasure[STAT_ARRAY_SIZE] 		__attribute__((section(".statistic")));

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define	ONESEC_TICK_TIME_MS	1000
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static RTC_HandleTypeDef 	RtcHandle;
static SPI_HandleTypeDef 	SpiHandle;

static uint32_t				testTimePass 	__attribute__((section(".noinit_rtc")));
static uint32_t				oneSecCounter	__attribute__((section(".noinit_rtc")));
static uint16_t 			rtcPoint		__attribute__((section(".noinit_rtc")));
static uint16_t 			timePoint 		__attribute__((section(".noinit_rtc")));
static uint32_t				validMark 		__attribute__((section(".noinit_rtc")));

static int16_t				temperature = 25;
/* Private function prototypes -----------------------------------------------*/
static void 				SystemClock_Config 	( void );
static void 				OneSecThread		( const void *argument );
static void					iniADCx				( ADC_HandleTypeDef * pAdcHandle );
static void					iniDACx				( DAC_HandleTypeDef * pDacHandle );
static void 				iniRTC				( RTC_HandleTypeDef * RtcHandle );
static void 				iniSPIx				( SPI_HandleTypeDef * SpiHandle );
static int16_t 				getDataTMP121		( void );
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32L1xx HAL library initialization:
       - Configure the Flash prefetch
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
  */
  HAL_Init();

  /* Configure the system clock to 32 MHz */
  SystemClock_Config();

  /* Initialize board */
  BSP_CTS_Init();

  /* Initialize the RTC */
  iniRTC( &RtcHandle );

  /* Initialize the SPIx */
  iniSPIx( &SpiHandle );

  /* Initialize the DACx */
  iniDACx( &DacHandle );

  /* Initialize the DACx */
  iniADCx( &AdcHandle );

  /* Initialize and check system parameters */
  LoadSysCnf();

  /* Create Threads */
  /* USB CDC Threads */
  osThreadDef( USBTask, UsbCDCThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2 );
  USBThreadHandle = osThreadCreate( osThread(USBTask), NULL);

  /*  measure thread  */
  osThreadDef( MesTask, MeasureThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );
  MeasureThreadHandle = osThreadCreate( osThread(MesTask), NULL);

  /*  OneSec thread  */
  osThreadDef( OneSecTask, OneSecThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );
  OneSecThreadHandle = osThreadCreate( osThread(OneSecTask), NULL);

  /* Start scheduler */
  osKernelStart();

  /*   never be here    */
  Error_Handler();
}

/*
 *
 */
static void OneSecThread(const void *argument)
{
	// init  rtcPoint, timePoint,
	if( validMark != VALID_MARK )
	{
		// load saved RTC
		for( rtcPoint = 0; rtcPoint < SAVE_ARRAY_SIZE; rtcPoint++ )
		if( rtcSaveArray[rtcPoint] == 0 )
		{
			DateTime_t  date = { .year = 2023, .month = 9, .day = 30, .hours = 12 };

			if( rtcPoint )
			{
				convertUnixTimeToDate( rtcSaveArray[rtcPoint - 1], &date );
				setRTC( &date );
			}
			else
				setRTC( &date );
			break;
		}

		// load saved test time
		for( timePoint = 0; timePoint < SAVE_ARRAY_SIZE; timePoint++ )
		if( testTimeSaveArray[timePoint] == 0 )
		{
			if( timePoint ) testTimePass = testTimeSaveArray[timePoint - 1];
			else			testTimePass = 0;
			break;
		}

		// init  RAM
		oneSecCounter = testTimePass = 0;
		validMark = VALID_MARK;
	}

	/* one second circle */
	for( TickType_t ctime = xTaskGetTickCount(), oneSecTickDelayMs = 0; ; ctime -= xTaskGetTickCount() )
	{
		/* calc time delay */
		oneSecTickDelayMs = (ONESEC_TICK_TIME_MS > pdTick_to_MS(ctime)) ? pdTick_to_MS(ctime) : 0;
		osDelay(oneSecTickDelayMs);

		// save RTC each 5 min
		if( (++oneSecCounter % 300) == 0)
		{
			if( rtcPoint == SAVE_ARRAY_SIZE )
			{
				rtcPoint = 0;
				CLEAR_EEPROM( rtcSaveArray, SAVE_ARRAY_SIZE ); // clear array on overflow
			}
			SAVE_SYSTEM_CNF( &rtcSaveArray[rtcPoint], getRTC() );
			rtcPoint++;
		}

		// count test time
		if( systemConfig.sysStatus == ACTIVE_STATUS )
		{
			testTimePass++;

			if( (testTimePass % systemConfig.measuringPeriodSec) == 0 )
							osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STARTMESURE_Evt );

			if( testTimePass >= systemConfig.testingTimeSec )
							osSignalSet( MeasureThreadHandle, MEASURE_THREAD_STOPTEST_Evt );

			// save testTimePass each 5 min
			if( (testTimePass % 300) == 0 )
			{
				if( timePoint == SAVE_ARRAY_SIZE )
				{
					timePoint = 0;
					CLEAR_EEPROM( testTimeSaveArray, SAVE_ARRAY_SIZE ); // clear array on overflow
				}
				SAVE_SYSTEM_CNF( &testTimeSaveArray[timePoint], testTimePass );
				timePoint++;
			}
		}
		else
			if( systemConfig.sysStatus != PAUSE_STATUS ) testTimePass = 0;

		/* read temperature */
		temperature = getDataTMP121();
	}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 32000000
  *            HCLK(Hz)                       = 32000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            HSE Frequency(Hz)              = 8000000
  *            HSI Frequency(Hz)              = 16000000
  *            HSE PREDIV                     = 1
  *            PLLMUL                         = 12 (if HSE) or 6 (if HSI)
  *            PLLDIV                         = 3
  *            Flash Latency(WS)              = 1
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef 		RCC_ClkInitStruct 	= {0};
  RCC_OscInitTypeDef 		RCC_OscInitStruct 	= {0};

  /* Set Voltage scale1 as MCU will run at 32MHz */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Poll VOSF bit of in PWR_CSR. Wait until it is reset to 0 */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOS) != RESET) {};

  /* Enable HSE Oscillator and Activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PLLDIV     = RCC_PLL_DIV3;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
  clocks dividers */
  RCC_ClkInitStruct.ClockType      = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler( void )
{
	while (1);
}


/**
  * @brief  Conversion complete callback in non blocking mode
  * @param  AdcHandle : ADC handle
  * @note   This example shows a simple way to report end of conversion
  *         and get conversion result. You can add your own implementation.
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *AdcHandle)
{
	/* Report to main program that ADC sequencer has reached its end */
	osSignalSet( MeasureThreadHandle, MEASURE_THREAD_CONVCMPLT_Evt );
}

/**
  * @brief  Conversion DMA half-transfer callback in non blocking mode
  * @param  hadc: ADC handle
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{

}

/**
  * @brief  ADC error callback in non blocking mode
  *        (ADC conversion with interruption or transfer by DMA)
  * @param  hadc: ADC handle
  * @retval None
  */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
	/* In case of ADC error, call main error handler */
	osSignalSet( MeasureThreadHandle, MEASURE_THREAD_CONVERROR_Evt );
}

/*
 * ADC hw ini
 */
static void	iniADCx	( ADC_HandleTypeDef * pAdcHandle )
{
	ADC_ChannelConfTypeDef   sConfig;

	/* Configuration of AdcHandle init structure: ADC parameters and regular group */
	pAdcHandle->Instance 				   = ADCx;
	pAdcHandle->Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV4;
	pAdcHandle->Init.Resolution            = ADC_RESOLUTION_12B;
	pAdcHandle->Init.DataAlign             = ADC_DATAALIGN_RIGHT;
	pAdcHandle->Init.ScanConvMode          = ADC_SCAN_ENABLE;               /* Sequencer enabled (ADC conversion on several channels, successively, following settings below) */
	pAdcHandle->Init.EOCSelection          = ADC_EOC_SEQ_CONV;
	pAdcHandle->Init.LowPowerAutoWait      = ADC_AUTOWAIT_DISABLE;
	pAdcHandle->Init.LowPowerAutoPowerOff  = ADC_AUTOPOWEROFF_DISABLE;
	pAdcHandle->Init.ChannelsBank          = ADC_CHANNELS_BANK_A;
	pAdcHandle->Init.ContinuousConvMode    = DISABLE;                       /* Continuous mode disabled to have only 1 rank converted at each conversion trig, and because discontinuous mode is enabled */
	pAdcHandle->Init.NbrOfConversion       = 10;                            /* Sequencer of regular group will convert the 3 first ranks: rank1, rank2, rank3 */
	pAdcHandle->Init.DiscontinuousConvMode = DISABLE;                       /* Sequencer of regular group will convert the sequence in several sub-divided sequences */
	pAdcHandle->Init.NbrOfDiscConversion   = 1;                             /* Sequencer of regular group will convert ranks one by one, at each conversion trig */
	pAdcHandle->Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Trig of conversion start done manually by software, without external event */
	pAdcHandle->Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
	pAdcHandle->Init.DMAContinuousRequests = ENABLE;                        /* ADC-DMA continuous requests to match with DMA configured in circular mode */

	if (HAL_ADC_Init( pAdcHandle ) != HAL_OK)
	{
	    /* ADC initialization error */
	    Error_Handler();
	}

	/* R0 */
	sConfig.Channel      = ADC_CHANNEL_10;
	sConfig.Rank         = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_96CYCLES;
	/* R1 */
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	sConfig.Channel      = ADC_CHANNEL_11;
	sConfig.Rank         = ADC_REGULAR_RANK_2;
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	/* R2 */
	sConfig.Channel      = ADC_CHANNEL_12;
	sConfig.Rank         = ADC_REGULAR_RANK_3;
	sConfig.SamplingTime = ADC_SAMPLETIME_96CYCLES;
	/* R3 */
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	sConfig.Channel      = ADC_CHANNEL_13;
	sConfig.Rank         = ADC_REGULAR_RANK_4;
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	/* R4 */
	sConfig.Channel      = ADC_CHANNEL_0;
	sConfig.Rank         = ADC_REGULAR_RANK_5;
	sConfig.SamplingTime = ADC_SAMPLETIME_96CYCLES;
	/* R5 */
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	sConfig.Channel      = ADC_CHANNEL_1;
	sConfig.Rank         = ADC_REGULAR_RANK_6;
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	/* R6 */
	sConfig.Channel      = ADC_CHANNEL_2;
	sConfig.Rank         = ADC_REGULAR_RANK_7;
	sConfig.SamplingTime = ADC_SAMPLETIME_96CYCLES;
	/* R7 */
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	sConfig.Channel      = ADC_CHANNEL_3;
	sConfig.Rank         = ADC_REGULAR_RANK_8;
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	/* VX -> DAC*/
	sConfig.Channel      = ADC_CHANNEL_20;
	sConfig.Rank         = ADC_REGULAR_RANK_9;
	sConfig.SamplingTime = ADC_SAMPLETIME_96CYCLES;
	/* VINTREF */
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
	sConfig.Channel      = ADC_CHANNEL_VREFINT;
	sConfig.Rank         = ADC_REGULAR_RANK_10;
	HAL_ADC_ConfigChannel( pAdcHandle, &sConfig);
}

/*
 * DAC hw ini
 */
static void	iniDACx	( DAC_HandleTypeDef * pDacHandle )
{
	/* Set the DAC parameters */
	pDacHandle->Instance = DACx;

	if (HAL_DAC_Init( pDacHandle ) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	DAC_ChannelConfTypeDef sConfig = { .DAC_Trigger = DAC_TRIGGER_NONE, .DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE };

	if (HAL_DAC_ConfigChannel( pDacHandle, &sConfig, DACx_CHANNEL) != HAL_OK)
	{
	    /* Channel configuration Error */
	    Error_Handler();
	}

	if (HAL_DAC_SetValue( pDacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, 0) != HAL_OK)
	{
		/* Setting value Error */
	    Error_Handler();
	}

	if (HAL_DAC_Start( pDacHandle, DACx_CHANNEL) != HAL_OK)
	{
	    /* Start Error */
	    Error_Handler();
	}
}

/*
 * SPI hw ini
 */
static void	iniSPIx	( SPI_HandleTypeDef * SpiHandle )
{
	/* Set the SPI parameters */
	SpiHandle->Instance               = SPIx;
	SpiHandle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	SpiHandle->Init.Direction         = SPI_DIRECTION_1LINE;
	SpiHandle->Init.CLKPhase          = SPI_PHASE_1EDGE;
	SpiHandle->Init.CLKPolarity       = SPI_POLARITY_LOW;
	SpiHandle->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle->Init.CRCPolynomial     = 7;
	SpiHandle->Init.DataSize          = SPI_DATASIZE_16BIT;
	SpiHandle->Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle->Init.NSS               = SPI_NSS_SOFT;
	SpiHandle->Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle->Init.Mode 			  = SPI_MODE_MASTER;

	if(HAL_SPI_Init( SpiHandle ) != HAL_OK)
	{
	   /* Initialization Error */
	   Error_Handler();
	}
}


/*
 * TMP121 driver
 */
static int16_t getDataTMP121(void)
{
	uint16_t	spi_rx_data = 0;

	BSP_SET_CS( 0 );
	HAL_SPI_Receive( &SpiHandle, (uint8_t *)&spi_rx_data, 1, 2);
	BSP_SET_CS( 1 );

	if( !(spi_rx_data) ) 		return SENSOR_NOT_CONNECTED;	// data line error
	if( spi_rx_data & (1<<2) ) 	return SENSOR_NOT_CONNECTED;	// no TMP121 on spi

	/* convert negative values */
	spi_rx_data >>= 7;
	spi_rx_data  |= ((spi_rx_data & (1<<8)) ? 0xff00 : 0x0);

	return spi_rx_data;
}


/*
 * RTC hw ini
 */
static void iniRTC( RTC_HandleTypeDef * RtcHandle )
{
	RtcHandle->Instance 		   = RTC;
	RtcHandle->Init.HourFormat     = RTC_HOURFORMAT_24;
	RtcHandle->Init.AsynchPrediv   = RTC_ASYNCH_PREDIV;
	RtcHandle->Init.SynchPrediv    = RTC_SYNCH_PREDIV;
	RtcHandle->Init.OutPut         = RTC_OUTPUT_DISABLE;
	RtcHandle->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	RtcHandle->Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

	if (HAL_RTC_Init( RtcHandle ) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}
}

/*
 * set rtc
 */
void setRTC( DateTime_t * date )
{
	RTC_DateTypeDef  sdatestructure;
	RTC_TimeTypeDef  stimestructure;

	/*##-1- Configure the Date #################################################*/
	sdatestructure.Year    			= date->year - 2000;
	sdatestructure.Month   			= date->month;
	sdatestructure.Date    			= date->day;
	sdatestructure.WeekDay 			= date->dayOfWeek;

	if(HAL_RTC_SetDate( &RtcHandle, &sdatestructure, RTC_FORMAT_BIN) != HAL_OK)
	{
	    /* Initialization Error */
	    Error_Handler();
	}

	/*##-2- Configure the Time #################################################*/
	stimestructure.Hours 			= date->hours;
	stimestructure.Minutes 			= date->minutes;
	stimestructure.Seconds 			= 0x00;
	stimestructure.TimeFormat 		= RTC_HOURFORMAT12_AM;
	stimestructure.DayLightSaving 	= RTC_DAYLIGHTSAVING_NONE ;
	stimestructure.StoreOperation 	= RTC_STOREOPERATION_RESET;

	if(HAL_RTC_SetTime( &RtcHandle, &stimestructure, RTC_FORMAT_BIN) != HAL_OK)
	{
		/* Initialization Error */
	    Error_Handler();
	}
}


/*
 * 		get rtc UNIX
 * */
systime_t getRTC( void )
{
	DateTime_t  	 date = {0};
	RTC_DateTypeDef  sdatestructure;
	RTC_TimeTypeDef  stimestructure;

	HAL_RTC_GetTime(&RtcHandle,&stimestructure,RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&RtcHandle,&sdatestructure,RTC_FORMAT_BIN);

	date.year    = 2000 + sdatestructure.Year;
	date.month   = sdatestructure.Month;
	date.day     = sdatestructure.Date;
	date.dayOfWeek = sdatestructure.WeekDay;
	date.hours   = stimestructure.Hours;
	date.minutes = stimestructure.Minutes;
	date.seconds = stimestructure.Seconds;

	return convertDateToUnixTime( &date );
}

/*
 * return Temperature
 */
int16_t getTemperature(void)
{
	return temperature;
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
