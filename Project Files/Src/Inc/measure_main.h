/**
  ******************************************************************************
  * @file    measure_main.h
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MEASURE_MAIN_H
#define __MEASURE_MAIN_H

/* Includes ------------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

#define MEASURE_NOERROR							0
#define MEASURE_HV_ERROR						(1 << 0)
#define MEASURE_CHANEL_ERROR					(1 << 1)
#define MEASURE_HV_UNSTABLE_ERROR				(1 << 2)
#define MEASURE_HV_ZERO_ERROR					(1 << 3)

#define MEASURE_GET_ERROR_CODE(ERROR)			((ERROR) >> 0) & 0xFF
#define MEASURE_GET_ERROR_LINE(ERROR)			((ERROR) >> 8) & 0xFF

#define MEASURE_SET_ERROR_CODE(ERROR)			( ERROR & 0xFF )
#define MEASURE_SET_ERROR_LINE(LINE)			(((LINE) << 8)  & 0xFF00 )


/* ## Definition of ADC related resources ################################### */
/* Definition of ADCx clock resources */
#define ADCx                            		ADC1
#define ADCx_CLK_ENABLE()               		__HAL_RCC_ADC1_CLK_ENABLE()

#define ADCx_FORCE_RESET()              		__HAL_RCC_ADC1_FORCE_RESET()
#define ADCx_RELEASE_RESET()            		__HAL_RCC_ADC1_RELEASE_RESET()

/* Definition of ADCx channels */
#define ADCx_CHANNELa                   		ADC_CHANNEL_4

/* Definition of ADCx channels pins */
#define ADCx_CHANNELa_GPIO_CLK_ENABLE() 		__HAL_RCC_GPIOA_CLK_ENABLE()
#define ADCx_CHANNELa_GPIO_PORT         		GPIOA
#define ADCx_CHANNELa_PIN               		GPIO_PIN_4

/* Definition of ADCx DMA resources */
#define ADCx_DMA_CLK_ENABLE()           		__HAL_RCC_DMA1_CLK_ENABLE()
#define ADCx_DMA                        		DMA1_Channel1

#define ADCx_DMA_IRQn                   		DMA1_Channel1_IRQn
#define ADCx_DMA_IRQHandler             		DMA1_Channel1_IRQHandler

/* Definition of ADCx NVIC resources */
#define ADCx_IRQn                       		ADC1_IRQn
#define ADCx_IRQHandler                 		ADC1_IRQHandler

/* ## Definition of TIM related resources ################################### */
/* Definition of TIMx clock resources */
#define TIMx                            		TIM6
#define TIMx_CLK_ENABLE()               		__HAL_RCC_TIM6_CLK_ENABLE()

#define TIMx_FORCE_RESET()              		__HAL_RCC_TIM6_FORCE_RESET()
#define TIMx_RELEASE_RESET()            		__HAL_RCC_TIM6_RELEASE_RESET()

#define DAC_EXTERNALTRIGCONV_Tx_TRGO    		DAC_TRIGGER_T6_TRGO

/* ## Definition of DAC related resources ################################### */
/* Definition of DACx clock resources */
#define DACx                            		DAC
#define DACx_CLK_ENABLE()               		__HAL_RCC_DAC_CLK_ENABLE()
#define DACx_CHANNEL_GPIO_CLK_ENABLE()  		__HAL_RCC_GPIOA_CLK_ENABLE()

#define DACx_FORCE_RESET()              		__HAL_RCC_DAC_FORCE_RESET()
#define DACx_RELEASE_RESET()            		__HAL_RCC_DAC_RELEASE_RESET()

/* Definition of DACx channels */
#define DACx_CHANNEL           					DAC_CHANNEL_1

/* Definition of DACx channels pins */
#define DACx_CHANNEL_PIN        				GPIO_PIN_4
#define DACx_CHANNEL_GPIO_PORT  				GPIOA

/* ## Definition of SPIx related resources ################################### */
/* Definition for SPIx clock resources */
#define SPIx                             		SPI1
#define SPIx_CLK_ENABLE()                		__HAL_RCC_SPI1_CLK_ENABLE()
#define SPIx_SCK_GPIO_CLK_ENABLE()       		__HAL_RCC_GPIOA_CLK_ENABLE()
#define SPIx_MISO_GPIO_CLK_ENABLE()      		__HAL_RCC_GPIOB_CLK_ENABLE()
#define SPIx_MOSI_GPIO_CLK_ENABLE()      		__HAL_RCC_GPIOB_CLK_ENABLE()

#define SPIx_FORCE_RESET()               		__HAL_RCC_SPI1_FORCE_RESET()
#define SPIx_RELEASE_RESET()             		__HAL_RCC_SPI1_RELEASE_RESET()

/* Definition for SPIx Pins */
#define SPIx_SCK_PIN                     		GPIO_PIN_5
#define SPIx_SCK_GPIO_PORT               		GPIOA
#define SPIx_SCK_AF                      		GPIO_AF5_SPI1
#define SPIx_MISO_PIN                    		GPIO_PIN_4
#define SPIx_MISO_GPIO_PORT              		GPIOB
#define SPIx_MISO_AF                     		GPIO_AF5_SPI1
#define SPIx_MOSI_PIN                    		GPIO_PIN_5
#define SPIx_MOSI_GPIO_PORT              		GPIOB
#define SPIx_MOSI_AF                     		GPIO_AF5_SPI1

/* Exported types ------------------------------------------------------------*/
typedef enum {

	stopMode = 0,
	pauseMode,
	testMode,
	measureMode,
	checkMode,
	errorMode

} currentMeasureMode_t;

/* Exported functions ------------------------------------------------------- */
uint32_t * 										getRawAdc		(void);
uint32_t * 										getMeasureData	(void);
int32_t    										getVrefmV		(void);
uint32_t   										getErrorCode	(void);
uint32_t   										getHighVoltagemV(void);
currentMeasureMode_t   							getCurrentMeasureMode(void);
/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void 		MeasureThread(const void *argument);

#endif /* __MAIN_H */
