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
#include "date_time.h"
#include "stm32l152c_discovery_cts.h"
#include "stm32l1xx_hal.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_interface.h"
#include "usb_main.h"
#include "measure_main.h"

/* Exported constants --------------------------------------------------------*/
#define EEPROM_CNF_ADR					(FLASH_EEPROM_BASE + 0x0000)

#define	NO_CONFIG_STATUS				0
#define	READY_STATUS					1
#define	ACTIVE_STATUS					2
#define	PAUSE_STATUS					3
#define	FINISH_STATUS					4

#define	ERROR_STATUS					0xffffffff

#define	MATRIX_LINEn					16
#define	MATRIX_RAWn						16

#define	SAVE_ARRAY_SIZE					256

#define	STAT_ARRAY_SIZE					STAT_MEM_SIZE / (MATRIX_RAWn * MATRIX_LINEn * sizeof(float))

#define	USB_THREAD_MESSAGEGOT_Evt		(1<<0)
#define	USB_THREAD_MEASURESTARTED_Evt	(1<<1)
#define	USB_THREAD_MEASUREREADY_Evt		(1<<2)
#define	USB_THREAD_TESTSTARTED_Evt		(1<<3)
#define	USB_THREAD_TESTSTOPPED_Evt		(1<<4)
#define	USB_THREAD_TESTPAUSED_Evt		(1<<5)
#define	USB_THREAD_MEASUREERROR_Evt		(1<<6)
#define	USB_THREAD_HVERRORDETECT_Evt	(1<<7)

#define	MEASURE_THREAD_STARTTEST_Evt	(1<<0)
#define	MEASURE_THREAD_STOPTEST_Evt		(1<<1)
#define	MEASURE_THREAD_PAUSETEST_Evt	(1<<2)
#define	MEASURE_THREAD_STARTMESURE_Evt	(1<<3)
#define	MEASURE_THREAD_CONVCMPLT_Evt	(1<<4)
#define	MEASURE_THREAD_CONVERROR_Evt	(1<<5)
#define	MEASURE_THREAD_TESTCAP_Evt	    (1<<6)

#define	VALID_MARK						0xA5E5E5A5

#define	SENSOR_NOT_CONNECTED			666

#ifdef RTC_CLOCK_SOURCE_LSI
#define RTC_ASYNCH_PREDIV    			0x7F
#define RTC_SYNCH_PREDIV     			0x0120
#endif

#ifdef RTC_CLOCK_SOURCE_LSE
#define RTC_ASYNCH_PREDIV  				0x7F
#define RTC_SYNCH_PREDIV   				0x00FF
#endif

/* Exported types ------------------------------------------------------------*/
typedef struct {

	uint32_t 	sysStatus;
	uint32_t	uTestVol;
	uint32_t	uMeasureVol;
	uint32_t	kiAmplifire;
	uint32_t	kdDivider;
	uint32_t 	testingTimeSec;
	uint32_t 	measuringPeriodSec;
	uint32_t 	dischargeTimeMs;
	uint32_t 	measureSavedPoints;
	uint32_t 	measureMask;
	uint32_t 	MaxErrorHV_mV;
	uint32_t 	IAmplifierSettleTimeMs;
	uint32_t 	HVMaxSettleTimeMs;
	uint32_t 	adcMeanFactor;
} sysCfg_t;

typedef struct {

	uint32_t	resistanceValMOhm[MATRIX_LINEn][MATRIX_RAWn];
} dataMeasure_t;

typedef struct {

	uint32_t 	timeMeasure;
	uint32_t	voltageMeasure;
	int32_t		temperatureMeasure;
} dataAttribute_t;

/* Exported macro ------------------------------------------------------------*/
#define CLEAR_ALL_EVENTS					{ \
											if( (event.status == osEventTimeout)  &&  (event.value.signals != 0) ) \
											do { uint32_t x; xTaskNotifyWait( 0xFFFFFFFF, 0, &x, 0 ); } while(0); \
											}

#define pdTick_to_MS(tick)					(tick) * ( ( TickType_t ) 1000 / configTICK_RATE_HZ )

#define SAVE_SYSTEM_CNF(ADR,DATA)			do { \
												HAL_FLASHEx_DATAEEPROM_Unlock ( ); \
												HAL_FLASHEx_DATAEEPROM_Erase  ( FLASH_TYPEERASEDATA_WORD, (uint32_t)(ADR) ); \
												HAL_FLASHEx_DATAEEPROM_Program( FLASH_TYPEPROGRAMDATA_FASTWORD, (uint32_t)(ADR), (DATA) ); \
											} while(0)

#define CLEAR_EEPROM(ADR,NUM)			    do { \
												HAL_FLASHEx_DATAEEPROM_Unlock ( ); \
												for( uint32_t i = 0; i < (NUM); i++ ) HAL_FLASHEx_DATAEEPROM_Erase( FLASH_TYPEERASEDATA_WORD, (uint32_t)(ADR) + i * 4 ); \
												HAL_FLASHEx_DATAEEPROM_Lock   ( ); \
											} while(0)

#define SAVE_MESURED_DATA(ADR,pDATA)		do { \
												FLASH_EraseInitTypeDef EraseInit = {		\
												.PageAddress = (uint32_t)(ADR),				\
												.NbPages	 = 4,							\
												.TypeErase   = FLASH_TYPEERASE_PAGES };		\
												uint32_t 	PageError;						\
												uint32_t	adr = (uint32_t)(ADR);			\
												HAL_FLASH_Unlock ( ); 						\
												HAL_FLASHEx_Erase( &EraseInit, &PageError); \
												for( uint32_t i = 0; i < 256; i++, adr += 4 ) HAL_FLASH_Program( FLASH_TYPEPROGRAM_WORD, adr, (pDATA)[i] ); \
												HAL_FLASH_Lock   ( ); \
											} while(0)

/* Exported functions ------------------------------------------------------- */
void 				Error_Handler			( void );
void 				configDACxStatMode		( uint32_t dacVal );
void 				configDACxTriangleMode	( uint32_t dacVal, uint32_t triangleAmpl );
uint32_t			setValDACx 				( uint32_t dacVal );
void 				setRTC					( DateTime_t * date );
systime_t 			getRTC					( void );
int16_t 			getTemperature			( void );
uint32_t 			getTestTimePass			( void );

#endif /* __MAIN_H */
