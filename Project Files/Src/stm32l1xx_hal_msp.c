/**
  ******************************************************************************
  * @file    USB_Device/CDC_Standalone/Src/stm32l1xx_hal_msp.c
  * @author  MCD Application Team
  * @brief   HAL MSP module.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup USBD_USER
* @{
*/

/** @defgroup USBD_USR_MAIN
  * @brief This file is the CDC application main file
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief SPI MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	if(hspi->Instance == SPIx)
	{
		/*##-1- Enable peripherals and GPIO Clocks #################################*/
		/* Enable GPIO TX/RX clock */
		SPIx_SCK_GPIO_CLK_ENABLE();
		SPIx_MISO_GPIO_CLK_ENABLE();
		SPIx_MOSI_GPIO_CLK_ENABLE();
		/* Enable SPI clock */
		SPIx_CLK_ENABLE();

		/*##-2- Configure peripheral GPIO ##########################################*/
		/* SPI SCK GPIO pin configuration  */
		GPIO_InitStruct.Pin       	= SPIx_SCK_PIN;
		GPIO_InitStruct.Mode      	= GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      	= GPIO_PULLDOWN;
		GPIO_InitStruct.Speed     	= GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate 	= SPIx_SCK_AF;
		HAL_GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStruct);

		/* SPI MISO GPIO pin configuration  */
		/* GPIO_InitStruct.Pin 		= SPIx_MISO_PIN;
		GPIO_InitStruct.Pull      	= GPIO_PULLUP;
		GPIO_InitStruct.Alternate 	= SPIx_MISO_AF;
		HAL_GPIO_Init(SPIx_MISO_GPIO_PORT, &GPIO_InitStruct); */

		/* SPI MOSI GPIO pin configuration  */
		GPIO_InitStruct.Pin 		= SPIx_MOSI_PIN;
		GPIO_InitStruct.Pull      	= GPIO_PULLUP;
		GPIO_InitStruct.Mode      	= GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate 	= SPIx_MOSI_AF;
		HAL_GPIO_Init(SPIx_MOSI_GPIO_PORT, &GPIO_InitStruct);
	}
}

/**
  * @brief SPI MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO configuration to its default state
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
  if(hspi->Instance == SPIx)
  {
    /*##-1- Reset peripherals ##################################################*/
    SPIx_FORCE_RESET();
    SPIx_RELEASE_RESET();

    /*##-2- Disable peripherals and GPIO Clocks ################################*/
    /* Configure SPI SCK as alternate function  */
    HAL_GPIO_DeInit(SPIx_SCK_GPIO_PORT, SPIx_SCK_PIN);
    /* Configure SPI MISO as alternate function  */
    HAL_GPIO_DeInit(SPIx_MISO_GPIO_PORT, SPIx_MISO_PIN);
    /* Configure SPI MOSI as alternate function  */
    HAL_GPIO_DeInit(SPIx_MOSI_GPIO_PORT, SPIx_MOSI_PIN);
  }
}

/**
  * @brief RTC MSP Initialization
  *        This function configures the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  *
  * @note  Care must be taken when HAL_RCCEx_PeriphCLKConfig() is used to select
  *        the RTC clock source; in this case the Backup domain will be reset in
  *        order to modify the RTC Clock source, as consequence RTC registers (including
  *        the backup registers) and RCC_BDCR register are set to their reset values.
  *
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
  RCC_OscInitTypeDef        RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

  /*##-1- Enables the PWR Clock and Enables access to the backup domain ###################################*/
  /* To change the source clock of the RTC feature (LSE, LSI), You have to:
     - Enable the power clock using __HAL_RCC_PWR_CLK_ENABLE()
     - Enable write access using HAL_PWR_EnableBkUpAccess() function before to
       configure the RTC clock source (to be done once after reset).
     - Reset the Back up Domain using __HAL_RCC_BACKUPRESET_FORCE() and
       __HAL_RCC_BACKUPRESET_RELEASE().
     - Configure the needed RTC clock source */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  /*##-2- Configure LSE/LSI as RTC clock source ###############################*/
#ifdef RTC_CLOCK_SOURCE_LSE

  RCC_OscInitStruct.OscillatorType 	= RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState 	= RCC_PLL_NONE;
  RCC_OscInitStruct.LSEState 		= RCC_LSE_ON;
  RCC_OscInitStruct.LSIState 		= RCC_LSI_OFF;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInitStruct.PeriphClockSelection 	= RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection 	= RCC_RTCCLKSOURCE_LSE;
  if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
#elif defined (RTC_CLOCK_SOURCE_LSI)
  RCC_OscInitStruct.OscillatorType 	= RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState 	= RCC_PLL_NONE;
  RCC_OscInitStruct.LSIState 		= RCC_LSI_ON;
  RCC_OscInitStruct.LSEState 		= RCC_LSE_OFF;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
#else
#error Please select the RTC Clock source inside the main.h file
#endif /*RTC_CLOCK_SOURCE_LSE*/

  /*##-2- Enable RTC peripheral Clocks #######################################*/
  /* Enable RTC Clock */
  __HAL_RCC_RTC_ENABLE();

}

/**
  * @brief RTC MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  * @param hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
  /*##-1- Reset peripherals ##################################################*/
   __HAL_RCC_RTC_DISABLE();
}

/**
  * @brief ADC MSP initialization
  *        This function configures the hardware resources used in this example:
  *          - Enable clock of ADC peripheral
  *          - Configure the GPIO associated to the peripheral channels
  *          - Configure the DMA associated to the peripheral
  *          - Configure the NVIC associated to the peripheral interruptions
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  static DMA_HandleTypeDef  DmaHandle;
  RCC_OscInitTypeDef        RCC_OscInitStructure;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable clock of GPIO associated to the peripheral channels */
  ADCx_CHANNELa_GPIO_CLK_ENABLE();

  /* Enable clock of ADCx peripheral */
  ADCx_CLK_ENABLE();

  /* Note: STM32L1 ADC is using a dedicated asynchronous clock derived        */
  /*       from HSI RC oscillator 16MHz.                                      */
  /*       The clock source has to be enabled at RCC top level using function */
  /*       "HAL_RCC_OscConfig()" (see comments in stm32l1_hal_adc.c header)   */

  /* Enable asynchronous clock source of ADCx */
  HAL_RCC_GetOscConfig(&RCC_OscInitStructure);
  RCC_OscInitStructure.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStructure.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStructure.HSIState = RCC_HSI_ON;
  HAL_RCC_OscConfig(&RCC_OscInitStructure);

  /* Enable clock of DMA associated to the peripheral */
  ADCx_DMA_CLK_ENABLE();

  /*##-3- Configure the DMA streams ##########################################*/
  /* Configure DMA parameters */
  DmaHandle.Instance = ADCx_DMA;

  DmaHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  DmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
  DmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
  DmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;   /* Transfer from ADC by half-word to match with ADC resolution 10 or 12 bits */
  DmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;   /* Transfer to memory by half-word to match with buffer variable type: half-word */
  DmaHandle.Init.Mode                = DMA_CIRCULAR;
  DmaHandle.Init.Priority            = DMA_PRIORITY_HIGH;

  /* Deinitialize  & Initialize the DMA for new transfer */
  HAL_DMA_DeInit(&DmaHandle);
  HAL_DMA_Init(&DmaHandle);

  /* Associate the initialized DMA handle to the ADC handle */
  __HAL_LINKDMA(hadc, DMA_Handle, DmaHandle);

  /*##-4- Configure the NVIC #################################################*/

  /* NVIC configuration for DMA interrupt (transfer completion or error) */
  /* Priority: high-priority */
  HAL_NVIC_SetPriority(ADCx_DMA_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(ADCx_DMA_IRQn);


  /* NVIC configuration for ADC interrupt */
  /* Priority: high-priority */
  HAL_NVIC_SetPriority(ADCx_IRQn, 14, 0);
  HAL_NVIC_EnableIRQ(ADCx_IRQn);
}

/**
  * @brief ADC MSP de-initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable clock of ADC peripheral
  *          - Revert GPIO associated to the peripheral channels to their default state
  *          - Revert DMA associated to the peripheral to its default state
  *          - Revert NVIC associated to the peripheral interruptions to its default state
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
  /*##-1- Reset peripherals ##################################################*/
  ADCx_FORCE_RESET();
  ADCx_RELEASE_RESET();

  /*##-3- Disable the DMA streams ############################################*/
  /* De-Initialize the DMA associated to the peripheral */
  if(hadc->DMA_Handle != NULL)
  {
    HAL_DMA_DeInit(hadc->DMA_Handle);
  }

  /*##-4- Disable the NVIC ###################################################*/
  /* Disable the NVIC configuration for DMA interrupt */
  HAL_NVIC_DisableIRQ(ADCx_DMA_IRQn);

  /* Disable the NVIC configuration for ADC interrupt */
  HAL_NVIC_DisableIRQ(ADCx_IRQn);
}

/**
  * @brief TIM MSP initialization
  *        This function configures the hardware resources used in this example:
  *          - Enable clock of peripheral
  * @param htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  /* TIM peripheral clock enable */
  TIMx_CLK_ENABLE();
}

/**
  * @brief TIM MSP de-initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable clock of peripheral
  * @param htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
  /*##-1- Reset peripherals ##################################################*/
  TIMx_FORCE_RESET();
  TIMx_RELEASE_RESET();
}

/**
  * @brief DAC MSP initialization
  *        This function configures the hardware resources used in this example:
  *          - Enable clock of peripheral
  *          - Configure the GPIO associated to the peripheral channels
  *          - Configure the NVIC associated to the peripheral interruptions
  * @param hdac: DAC handle pointer
  * @retval None
  */
void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
  GPIO_InitTypeDef          GPIO_InitStruct;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO clock */
  DACx_CHANNEL_GPIO_CLK_ENABLE();
  /* DAC peripheral clock enable */
  DACx_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* Configure GPIO pin of the selected DAC channel */
  GPIO_InitStruct.Pin  = DACx_CHANNEL_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DACx_CHANNEL_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief DAC MSP de-initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable clock of peripheral
  *          - Revert GPIO associated to the peripheral channels to their default state
  *          - Revert NVIC associated to the peripheral interruptions to its default state
  * @param hadc: DAC handle pointer
  * @retval None
  */
void HAL_DAC_MspDeInit(DAC_HandleTypeDef *hdac)
{
  /*##-1- Reset peripherals ##################################################*/
  DACx_FORCE_RESET();
  DACx_RELEASE_RESET();
}


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
