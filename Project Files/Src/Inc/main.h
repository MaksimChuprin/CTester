/**
  ******************************************************************************
  * @file    USB_Device/CDC_Standalone/Inc/main.h
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "stdbool.h"
#include "stm32l152c_discovery_cts.h"
#include "stm32l1xx_hal.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_interface.h"
#include "usb_main.h"

/* Exported types ------------------------------------------------------------*/
typedef struct {

	uint32_t 	sysStatus;
	uint32_t	uTestVol;
	uint32_t	uMeasureVol;
	uint32_t	kiAmplifire;
	uint32_t	kdDivider;
	uint32_t 	testingTimeSec;
	uint32_t 	measuringPeriodSec;
	uint32_t 	dischargePreMeasureTimeMs;
	uint32_t 	measureSavedPoints;

} sysCfg_t;

/* Exported constants --------------------------------------------------------*/

#define EEPROM_CNF_ADR					(FLASH_EEPROM_BASE + 0x0000)

#define	NO_CONFIG_STATUS				0
#define	READY_STATUS					1
#define	ACTIVE_STATUS					2
#define	FINISH_STATUS					3

#define	ERROR_STATUS					0xffffffff

/* ## Definition of ADC related resources ################################### */
/* Definition of ADCx clock resources */
#define ADCx                            ADC1
#define ADCx_CLK_ENABLE()               __HAL_RCC_ADC1_CLK_ENABLE()

#define ADCx_FORCE_RESET()              __HAL_RCC_ADC1_FORCE_RESET()
#define ADCx_RELEASE_RESET()            __HAL_RCC_ADC1_RELEASE_RESET()

/* Definition of ADCx channels */
#define ADCx_CHANNELa                   ADC_CHANNEL_4

/* Definition of ADCx channels pins */
#define ADCx_CHANNELa_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define ADCx_CHANNELa_GPIO_PORT         GPIOA
#define ADCx_CHANNELa_PIN               GPIO_PIN_4

/* Definition of ADCx DMA resources */
#define ADCx_DMA_CLK_ENABLE()           __HAL_RCC_DMA1_CLK_ENABLE()
#define ADCx_DMA                        DMA1_Channel1

#define ADCx_DMA_IRQn                   DMA1_Channel1_IRQn
#define ADCx_DMA_IRQHandler             DMA1_Channel1_IRQHandler

/* Definition of ADCx NVIC resources */
#define ADCx_IRQn                       ADC1_IRQn
#define ADCx_IRQHandler                 ADC1_IRQHandler

/* ## Definition of TIM related resources ################################### */
/* Definition of TIMx clock resources */
#define TIMx                            TIM2
#define TIMx_CLK_ENABLE()               __HAL_RCC_TIM2_CLK_ENABLE()

#define TIMx_FORCE_RESET()              __HAL_RCC_TIM2_FORCE_RESET()
#define TIMx_RELEASE_RESET()            __HAL_RCC_TIM2_RELEASE_RESET()

#define ADC_EXTERNALTRIGCONV_Tx_TRGO    ADC_EXTERNALTRIGCONV_T2_TRGO

/* ## Definition of DAC related resources ################################### */
/* Definition of DACx clock resources */
#define DACx                            DAC
#define DACx_CLK_ENABLE()               __HAL_RCC_DAC_CLK_ENABLE()
#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define DACx_FORCE_RESET()              __HAL_RCC_DAC_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC_RELEASE_RESET()

/* Definition of DACx channels */
#define DACx_CHANNEL_TO_ADCx_CHANNELa            DAC_CHANNEL_1

/* Definition of DACx channels pins */
#define DACx_CHANNEL_TO_ADCx_CHANNELa_PIN        GPIO_PIN_4
#define DACx_CHANNEL_TO_ADCx_CHANNELa_GPIO_PORT  GPIOA

/* Exported macro ------------------------------------------------------------*/
#define SAVE_SYSTEM_CNF_ENTIRE_STRUCT		for( uint32_t i = 0, adr = EEPROM_CNF_ADR, *ptrD = (uint32_t *) &systemConfig; i < (sizeof(systemConfig) + 3) / 4; i++, ptrD++ ) \
																				HAL_FLASHEx_DATAEEPROM_Program( FLASH_TYPEPROGRAMDATA_WORD, adr, *ptrD );

#define SAVE_SYSTEM_CNF(ADR, DATA)			HAL_FLASHEx_DATAEEPROM_Program( FLASH_TYPEPROGRAMDATA_WORD, (uint32_t)(ADR), (DATA) )
/* Exported functions ------------------------------------------------------- */
void Error_Handler(void);

#endif /* __MAIN_H */
