/**
  ******************************************************************************
  * @file    stm32l152c_discovery_cts.h
  * @author  Epta
  * @brief   This file contains definitions for STM32L152C-Discovery & CTS
  ******************************************************************************

  ******************************************************************************
  */
  
/** @addtogroup BSP
  * @{
  */ 

/** @addtogroup STM32L152C_DISCOVERY
  * @{
  */ 
  
/** @addtogroup STM32L152C_Discovery_Common
  * @{
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L152C_DISCOVERY_CTS_H
#define __STM32L152C_DISCOVERY_CTS_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"
#include "stm32l152c_discovery_cts.h"

  
typedef enum 
{
  LED3 = 0,
  LED4 = 1,
  
  LED_GREEN = LED3,
  LED_BLUE = LED4
    
} Led_TypeDef;

typedef enum {
	LineAD0 = 0,
	LineAD1,
	LineAD2,
	LineAD3,
	LineAD4,
	LineAD5,
	LineAD6,
	LineAD7,
	LineAD8,
	LineAD9,
	LineAD10,
	LineAD11,
	LineAD12,
	LineAD13,
	LineAD14,
	LineAD15,
	AllLineAD

} Line_NumDef;


typedef enum
{
  Line_ZV = 0,
  Line_HV

} Line_StateDef;


typedef enum
{
  Opto_Close = 0,
  Opto_Open

} Opto_StateDef;

typedef enum
{
  Mux_1_8  = 0,
  Mux_9_16

} RMux_StateDef;


#if !defined (USE_STM32L152C_DISCO)
 #define USE_STM32L152C_DISCO
#endif
  
/** @addtogroup STM32L152C_DISCOVERY_LED LED Constants
  * @{
  */
#define LEDn                             2

#define LED3_PIN                         GPIO_PIN_7           /* PB.07 */
#define LED3_GPIO_PORT                   GPIOB
#define LED3_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()  
#define LED3_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOB_CLK_DISABLE()  

#define LED4_PIN                         GPIO_PIN_6           /* PB.06 */
#define LED4_GPIO_PORT                   GPIOB
#define LED4_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()  
#define LED4_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOB_CLK_DISABLE()  

#define LEDx_GPIO_CLK_ENABLE(__INDEX__)   do { if ((__INDEX__) == 0) LED3_GPIO_CLK_ENABLE(); else LED4_GPIO_CLK_ENABLE();} while(0)

#define LEDx_GPIO_CLK_DISABLE(__INDEX__)  (((__INDEX__) == 0) ? LED3_GPIO_CLK_DISABLE() : LED4_GPIO_CLK_DISABLE())
/**
  * @}
  */

/** CTS signals
  * @{
  */
#define	ADLINEn							MATRIX_LINEn

#define RAN_PIN                        	GPIO_PIN_13
#define RAN_GPIO_PORT                  	GPIOC
#define OPTO_PIN                        GPIO_PIN_13
#define OPTO_GPIO_PORT                  GPIOB
#define MUX_8_16_PIN                    GPIO_PIN_12
#define MUX_8_16_GPIO_PORT              GPIOB
#define CS_PIN                        	GPIO_PIN_11
#define CS_GPIO_PORT                  	GPIOB

#define R0_PIN                     		GPIO_PIN_0
#define R0_GPIO_PORT               		GPIOC
#define R1_PIN                     		GPIO_PIN_1
#define R1_GPIO_PORT               		GPIOC
#define R2_PIN                     		GPIO_PIN_2
#define R2_GPIO_PORT               		GPIOC
#define R3_PIN                     		GPIO_PIN_3
#define R3_GPIO_PORT               		GPIOC
#define R4_PIN                     		GPIO_PIN_0
#define R4_GPIO_PORT               		GPIOA
#define R5_PIN                     		GPIO_PIN_1
#define R5_GPIO_PORT               		GPIOA
#define R6_PIN                     		GPIO_PIN_2
#define R6_GPIO_PORT               		GPIOA
#define R7_PIN                     		GPIO_PIN_3
#define R7_GPIO_PORT               		GPIOA

#define DAC_PIN                    		GPIO_PIN_4
#define DAC_GPIO_PORT              		GPIOA

#define VX_PIN                     		GPIO_PIN_14
#define VX_GPIO_PORT               		GPIOB
#define VR_PIN                     		GPIO_PIN_15
#define VR_GPIO_PORT               		GPIOB

#define AD0_PIN                    		GPIO_PIN_6
#define AD0_GPIO_PORT              		GPIOC
#define AD1_PIN                    		GPIO_PIN_7
#define AD1_GPIO_PORT              		GPIOC
#define AD2_PIN                    		GPIO_PIN_8
#define AD2_GPIO_PORT              		GPIOC
#define AD3_PIN                    		GPIO_PIN_9
#define AD3_GPIO_PORT              		GPIOC

#define AD4_PIN                    		GPIO_PIN_8
#define AD4_GPIO_PORT              		GPIOA
#define AD5_PIN                    		GPIO_PIN_9
#define AD5_GPIO_PORT              		GPIOA
#define AD6_PIN                    		GPIO_PIN_10
#define AD6_GPIO_PORT              		GPIOA

#define AD7_PIN                    		GPIO_PIN_10
#define AD7_GPIO_PORT              		GPIOC
#define AD8_PIN                    		GPIO_PIN_11
#define AD8_GPIO_PORT              		GPIOC
#define AD9_PIN                    		GPIO_PIN_12
#define AD9_GPIO_PORT              		GPIOC

#define AD10_PIN                   		GPIO_PIN_2
#define AD10_GPIO_PORT             		GPIOD

#define AD11_PIN                    	GPIO_PIN_6
#define AD11_GPIO_PORT              	GPIOB
#define AD12_PIN                    	GPIO_PIN_7
#define AD12_GPIO_PORT              	GPIOB
#define AD13_PIN                    	GPIO_PIN_8
#define AD13_GPIO_PORT              	GPIOB
#define AD14_PIN                    	GPIO_PIN_9
#define AD14_GPIO_PORT              	GPIOB
#define AD15_PIN                    	GPIO_PIN_10
#define AD15_GPIO_PORT              	GPIOB

/** @addtogroup STM32152C_DISCOVERY_LED_Functions
  * @{
  */ 
void      	BSP_LED_Init(Led_TypeDef Led);
void      	BSP_LED_On(Led_TypeDef Led);
void      	BSP_LED_Off(Led_TypeDef Led);
void      	BSP_LED_Toggle(Led_TypeDef Led);

/**
  * @}
  */
uint32_t 	BSP_PAN_GetState( void );
void 		BSP_SET_CS( uint8_t cs );
void 		BSP_SET_OPTO( Opto_StateDef Ostate);
void 		BSP_SET_RMUX( RMux_StateDef Mstate);
void 		BSP_CTS_Init(void);
void		BSP_CTS_SetAnyLine( Line_NumDef line, Line_StateDef Lstate, Opto_StateDef Ostate);
void 		BSP_CTS_SetSingleLine( Line_NumDef line );
void 		LoadSysCnf(void);
bool 		CheckSysCnf(void);

/**
  * @}
  */

/**
  * @}
  */ 
  
#ifdef __cplusplus
}
#endif
  
#endif /* __STM32L152C_DISCOVERY_H */
