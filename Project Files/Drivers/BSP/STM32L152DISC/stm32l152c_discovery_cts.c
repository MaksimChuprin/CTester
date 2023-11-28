/**
  ******************************************************************************
  * @file    stm32l152c_discovery_cts.c
  * @author  MCD Application Team
  * @brief   This file provides
  *            - set of firmware functions to manage Led and push-button
  *          available on STM32L152C-Discovery board from STMicroelectronics.  
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

extern __IO sysCfg_t				systemConfig;

/** @defgroup STM32L152C_DISCOVERY_Private_Variables Private Variables
  * @{
  */ 
GPIO_TypeDef* GPIO_PORT[LEDn] = {LED3_GPIO_PORT,
                                 LED4_GPIO_PORT};

const uint16_t GPIO_PIN[LEDn] = {LED3_PIN,
                                 LED4_PIN};

typedef struct {

	uint16_t			pin;
	GPIO_TypeDef *		port;

} PinPortAD_t;

const PinPortAD_t Line_AD[ADLINEn] = {

		{ .pin 	= AD0_PIN,	.port	= AD0_GPIO_PORT },
		{ .pin 	= AD1_PIN,	.port	= AD1_GPIO_PORT },
		{ .pin 	= AD2_PIN,	.port	= AD2_GPIO_PORT },
		{ .pin 	= AD3_PIN,	.port	= AD3_GPIO_PORT },
		{ .pin 	= AD4_PIN,	.port	= AD4_GPIO_PORT },
		{ .pin 	= AD5_PIN,	.port	= AD5_GPIO_PORT },
		{ .pin 	= AD6_PIN,	.port	= AD6_GPIO_PORT },
		{ .pin 	= AD7_PIN,	.port	= AD7_GPIO_PORT },
		{ .pin 	= AD8_PIN,	.port	= AD8_GPIO_PORT },
		{ .pin 	= AD9_PIN,	.port	= AD9_GPIO_PORT },
		{ .pin 	= AD10_PIN,	.port	= AD10_GPIO_PORT },
		{ .pin 	= AD11_PIN,	.port	= AD11_GPIO_PORT },
		{ .pin 	= AD12_PIN,	.port	= AD12_GPIO_PORT },
		{ .pin 	= AD13_PIN,	.port	= AD13_GPIO_PORT },
		{ .pin 	= AD14_PIN,	.port	= AD14_GPIO_PORT },
		{ .pin 	= AD15_PIN,	.port	= AD15_GPIO_PORT }
};


/**
  * @brief  Configures LED GPIO.
  * @param  Led: Specifies the Led to be configured. 
  *   This parameter can be one of following parameters:
  *     @arg LED3
  *     @arg LED4
  * @retval None
  */
void BSP_LED_Init(Led_TypeDef Led)
{
  GPIO_InitTypeDef  gpioinitstruct = {0};
  
  /* Enable the GPIO_LED Clock */
  LEDx_GPIO_CLK_ENABLE(Led);

  /* Configure the GPIO_LED pin */
  gpioinitstruct.Pin   = GPIO_PIN[Led];
  gpioinitstruct.Mode  = GPIO_MODE_OUTPUT_PP;
  gpioinitstruct.Pull  = GPIO_NOPULL;
  gpioinitstruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIO_PORT[Led], &gpioinitstruct);

  /* Reset PIN to switch off the LED */
  HAL_GPIO_WritePin(GPIO_PORT[Led],GPIO_PIN[Led], GPIO_PIN_RESET);
}

/**
  * @brief  Turns selected LED On.
  * @param  Led: Specifies the Led to be set on. 
  *   This parameter can be one of following parameters:
  *     @arg LED3
  *     @arg LED4  
  * @retval None
  */
void BSP_LED_On(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET); 
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: Specifies the Led to be set off. 
  *   This parameter can be one of following parameters:
  *     @arg LED3
  *     @arg LED4 
  * @retval None
  */
void BSP_LED_Off(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET); 
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: Specifies the Led to be toggled. 
  *   This parameter can be one of following parameters:
  *     @arg LED3
  *     @arg LED4  
  * @retval None
  */
void BSP_LED_Toggle(Led_TypeDef Led)
{
  HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
}

/**
  * @brief  Returns the selected Button state.
  * @retval RAN_PIN state.
  */
uint32_t BSP_PAN_GetState(void)
{
  return HAL_GPIO_ReadPin( RAN_GPIO_PORT, RAN_PIN );
}

/**
  * @brief  SET OPTO signal
  * @retval NONE.
  */
void BSP_SET_CS( uint8_t cs )
{
	HAL_GPIO_WritePin( CS_GPIO_PORT, CS_PIN, ( cs ) ? GPIO_PIN_SET : GPIO_PIN_RESET );
}

/**
  * @brief  SET OPTO signal
  * @retval NONE.
  */
void BSP_SET_OPTO( Opto_StateDef Ostate)
{
	HAL_GPIO_WritePin( OPTO_GPIO_PORT, OPTO_PIN, ( Ostate == Opto_Close ) ? GPIO_PIN_RESET : GPIO_PIN_SET );
}

/**
  * @brief  SET 8/16 signal
  * @retval NONE.
  */
void BSP_SET_RMUX( RMux_StateDef Mstate)
{
	HAL_GPIO_WritePin( MUX_8_16_GPIO_PORT, MUX_8_16_PIN, ( Mstate == Mux_1_8 ) ? GPIO_PIN_RESET : GPIO_PIN_SET );
}


/**
  * @brief  SET AD, OPTO signals
  * @retval None
  */
void BSP_CTS_SetAnyLine( Line_NumDef line, Line_StateDef Lstate, Opto_StateDef Ostate )
{
	BSP_SET_OPTO( Ostate );

	if( line != AllLineAD )
	  HAL_GPIO_WritePin( Line_AD[line].port,  Line_AD[line].pin , ( Lstate == Line_HV ) ? GPIO_PIN_SET : GPIO_PIN_RESET );
	else
	{
		for(uint32_t i = 0; i < ADLINEn; i++ )
			HAL_GPIO_WritePin( Line_AD[i].port,  Line_AD[i].pin , ( Lstate == Line_HV ) ? GPIO_PIN_SET : GPIO_PIN_RESET );
	}
}

/**
  * @brief  SET line in HV state for measure
  * @retval None
  */
void BSP_CTS_SetSingleLine( Line_NumDef line )
{
	BSP_SET_OPTO( Opto_Open );

	for(uint32_t i = 0; i < ADLINEn; i++ )
			HAL_GPIO_WritePin( Line_AD[i].port,  Line_AD[i].pin , ( line == i ) ? GPIO_PIN_SET : GPIO_PIN_RESET );
}

/**
  * @brief  SET All line in zero state for discharge
  * @retval None
  */
void BSP_CTS_SetAllLineDischarge( void )
{
	BSP_CTS_SetAnyLine( AllLineAD, Line_ZV, Opto_Open );
}


/**
  ** @brief init ports
  * @retval None
  */
void BSP_CTS_Init(void)
{
  GPIO_InitTypeDef  gpioinitstruct = {0};

  /* Enable the GPIO Clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* Configure the GPIOA pins */
  gpioinitstruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4; // R4...R7, DAC
  gpioinitstruct.Mode  = GPIO_MODE_ANALOG;
  gpioinitstruct.Speed = GPIO_SPEED_FREQ_LOW;
  gpioinitstruct.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init( GPIOA, &gpioinitstruct);

  gpioinitstruct.Pin   = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_15;
  gpioinitstruct.Mode  = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init( GPIOA, &gpioinitstruct);

  /* Configure the GPIOB pins */
  gpioinitstruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 |
		  	  	  	  	  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15;
  gpioinitstruct.Mode  = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init( GPIOB, &gpioinitstruct);

  gpioinitstruct.Pin   = GPIO_PIN_14; // VX
  gpioinitstruct.Mode  = GPIO_MODE_ANALOG;
  HAL_GPIO_Init( GPIOB, &gpioinitstruct);

  /* Configure the GPIOC pins */
  gpioinitstruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3; // R0...R3
  gpioinitstruct.Mode  = GPIO_MODE_ANALOG;
  HAL_GPIO_Init( GPIOC, &gpioinitstruct);

  gpioinitstruct.Pin   = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  gpioinitstruct.Mode  = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init( GPIOC, &gpioinitstruct);

  /* Configure the GPIOD pins */
  gpioinitstruct.Pin   = GPIO_PIN_2 ;
  gpioinitstruct.Mode  = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init( GPIOD, &gpioinitstruct);

  BSP_CTS_SetAnyLine( AllLineAD, Line_ZV, Opto_Open );
  BSP_SET_RMUX( Mux_1_8 );
  BSP_SET_CS( 1 );
}

/**
  ** @brief load configuration of System
  * @retval None
  */
void LoadSysCnf(void)
{
	CheckSysCnf();
}

/**
  ** @brief check configuration of System
  * @retval None
  */
bool CheckSysCnf(void)
{
	bool status = (systemConfig.kiAmplifire != 0) 	 		 && (systemConfig.dischargeTimeMs != 0) 	&&
				  (systemConfig.kdDivider != 0) 	 		 && (systemConfig.measuringPeriodSec != 0) 	&&
				  (systemConfig.testingTimeSec != 0) 		 && (systemConfig.uMeasureVol != 0) 		&&
				  (systemConfig.uTestVol != 0)       		 && (systemConfig.MaxErrorHV_mV != 0 )     	&&
				  (systemConfig.IAmplifierSettleTimeMs != 0) && (systemConfig.HVMaxSettleTimeMs != 0 ) 	&&
				  (systemConfig.adcMeanFactor  != 0)         && (systemConfig.dacTrianglePeriodUs != 0 ) &&
				  (systemConfig.dacTriangleAmplitude != 0 )  && (systemConfig.shortErrorTrshCurrent_nA != 0 ) &&
				  (systemConfig.contactErrorTrshCapacitance_pF != 0 )  && (systemConfig.optoSignalSettleTimeMs != 0) ;

	if(status == false)
	{
		if(systemConfig.sysStatus != NO_CONFIG_STATUS)	SAVE_SYSTEM_CNF( &systemConfig.sysStatus, NO_CONFIG_STATUS );
	}
	else
	{
		if(systemConfig.sysStatus == NO_CONFIG_STATUS)  SAVE_SYSTEM_CNF( &systemConfig.sysStatus, READY_STATUS );
	}

	return status;
}

