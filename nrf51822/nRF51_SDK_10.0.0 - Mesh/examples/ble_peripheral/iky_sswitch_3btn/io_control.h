#ifndef __IO_CONTROL__H__
#define __IO_CONTROL__H__

#include <string.h>
#include <stdint.h>
#include "boards.h"

#define NORMAL_FILTER_CNT			1
#define PROXIMITY_FILTER_CNT	1

typedef struct {
	uint8_t filterCnt;
	uint8_t bitOld;
	uint8_t bitNew;
	uint8_t highCnt;
	uint8_t lowCnt;
	uint8_t newUpdate;
	uint8_t updateLowCnt;
	uint8_t updateHighCnt;
	void (*updateCallback)(uint8_t status);
}IO_FILTER_TYPE;


typedef struct
{
    uint32_t onTime;
		uint32_t offTime;
		int32_t counter;
		uint8_t status;
		uint8_t enable;
		uint32_t times;
		uint32_t timesSave;
} IO_TOGGLE_TYPE;

typedef enum
{
        LED_PROXIMITY_OFF,
        LED_PROXIMITY_DIM,
        LED_PROXIMITY_ON
}LED_PROXIMITY_STATE;

#define TIMER_PERIOD							1	//ms

#define IO_STATUS_ON							1
#define IO_STATUS_OFF 						0
#define IO_STATUS_NOCONTROL 			2

#define IO_STATUS_ON_TIME_DFG			(500 / TIMER_PERIOD) /*1s */
#define IO_STATUS_OFF_TIME_DFG		(500 / TIMER_PERIOD) /*1s */

#define IO_TOGGLE_ENABLE					1
#define IO_TOGGLE_DISABLE					0


#define IO_MAX_TIMES 0xffffffff
#define IO_MAX_VALUE 0xffffffff

extern IO_FILTER_TYPE io_btn_1;
extern IO_FILTER_TYPE io_btn_2;
extern IO_FILTER_TYPE io_btn_3;
extern IO_FILTER_TYPE io_proximity_1;

extern IO_TOGGLE_TYPE io_led_red_1;
extern IO_TOGGLE_TYPE io_buzzer;

extern uint32_t ioRelay_1_Status;
extern uint32_t ioRelay_2_Status;
extern uint32_t ioRelay_3_Status;
extern uint32_t ioRelay_4_Status;

uint8_t IO_ToggleProcess(IO_TOGGLE_TYPE *ioCtrl, uint32_t preodic);
void IO_ToggleSetStatus(IO_TOGGLE_TYPE *ledCtr,uint32_t onTime,uint32_t offTime,uint32_t enable,uint32_t times);
void IO_ToggleSetStatus_Extend(IO_TOGGLE_TYPE *ioCtrl,uint32_t onTime,uint32_t offTime,uint32_t enable,uint32_t times,uint8_t action);
void IOFilter_Init(IO_FILTER_TYPE *IOFilter,uint8_t filter_cnt,uint8_t status,void (*callback)(uint8_t status));
void IOFilter_Update(IO_FILTER_TYPE *IOFilter,uint8_t status);

uint8_t IO_ReadStatusOnRAM(uint32_t *ioStatus);
uint8_t IO_UpdateStatusOnRAM(uint32_t* ioStatus, uint8_t newStatus);
uint8_t IO_ToggleStatusOnRAM(uint32_t* ioStatus);
uint8_t IO_GetStatusOnRAM(uint32_t ioStatus);
#endif

