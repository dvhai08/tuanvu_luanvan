#include "io_control.h"
#include "crc8.h"
#include "nrf_log.h"
#include "IKYProtocol\iky_ble_app.h"
#include "system_config.h"

IO_TOGGLE_TYPE io_led_red_1;
IO_TOGGLE_TYPE io_buzzer;

IO_FILTER_TYPE io_btn_1;
IO_FILTER_TYPE io_btn_2;
IO_FILTER_TYPE io_btn_3;

IO_FILTER_TYPE io_proximity_1;


void (*io_status_on_ram_update_callback)(uint8_t status);

void IO_ToggleSetStatus(IO_TOGGLE_TYPE *ioCtrl,uint32_t onTime,uint32_t offTime,uint32_t enable,uint32_t times)		
{
		ioCtrl->onTime = onTime;
		ioCtrl->offTime = offTime;
		ioCtrl->counter = 0;
		ioCtrl->enable = enable;
		ioCtrl->times = times;
		ioCtrl->timesSave = times;
}													
		

uint8_t IO_ToggleProcess(IO_TOGGLE_TYPE *ioCtrl, uint32_t preodic)	
{
	if(ioCtrl->enable == IO_TOGGLE_ENABLE) 
	{
			if(ioCtrl->counter > preodic)
				ioCtrl->counter -= preodic;
			else ioCtrl->counter = 0;
				
			if(ioCtrl->counter == 0) 
			{
				if(ioCtrl->times) 
				{
					ioCtrl->times--;
					ioCtrl->counter = ioCtrl->offTime + ioCtrl->onTime;
					ioCtrl->status = IO_STATUS_ON;
				}
				else
				{
					ioCtrl->enable = IO_TOGGLE_DISABLE;
					ioCtrl->status = IO_STATUS_OFF;
				}
			}
			
			if(ioCtrl->counter <= ioCtrl->offTime) 
				ioCtrl->status = IO_STATUS_OFF;
	}
	else
	{
		ioCtrl->status = IO_STATUS_OFF;
	}
	return ioCtrl->status;
}

void IOFilter_Init(IO_FILTER_TYPE *IOFilter,uint8_t filter_cnt,uint8_t status,void (*callback)(uint8_t status))
{
	memset(IOFilter,0,sizeof(IO_FILTER_TYPE));
	IOFilter->filterCnt = filter_cnt;
	IOFilter->updateCallback = callback;	
	IOFilter->bitOld = status;
	IOFilter->bitNew = status;
	IOFilter->newUpdate = 0;
	IOFilter->updateHighCnt = 0;
	IOFilter->updateLowCnt = 0;
}

void IOFilter_Update(IO_FILTER_TYPE *IOFilter,uint8_t status)
{
	if (status)
	{
		IOFilter->highCnt++;
		IOFilter->lowCnt = 0;
	}
	else
	{
		IOFilter->highCnt = 0;
		IOFilter->lowCnt++;
	}
	if (IOFilter->highCnt >= IOFilter->filterCnt)
	{
			IOFilter->bitNew = 1;
			IOFilter->highCnt = 0;
	}
	else if (IOFilter->lowCnt >= IOFilter->filterCnt)
	{
			IOFilter->lowCnt = 0;
			IOFilter->bitNew = 0;
	}
	
	if (IOFilter->bitNew != IOFilter->bitOld)
	{
			IOFilter->newUpdate = 1;
			if(IOFilter->bitNew == 0)
			{
				IOFilter->updateLowCnt++;
			}
			else
			{
				IOFilter->updateHighCnt++;				
			}
			IOFilter->bitOld = IOFilter->bitNew;
			if (IOFilter->updateCallback) 
			{
				IOFilter->updateCallback(IOFilter->bitNew);
			}
	}
}


uint8_t IO_ReadStatusOnRAM(uint32_t *ioStatus)
{	
	if(*ioStatus != RELAY_ON && *ioStatus != RELAY_OFF)
	{
		*ioStatus = RELAY_OFF;		
	}
	
	return (*ioStatus == RELAY_ON) ? 1 : 0;
}

uint8_t IO_UpdateStatusOnRAM(uint32_t* ioStatus, uint8_t newStatus)
{
	if(newStatus)
	{
		*ioStatus = RELAY_ON;
	}
	else
	{
		*ioStatus = RELAY_OFF;
	}
	
	reportFlag |= OUTPUT_REPORT_FLAG;
	
	return (*ioStatus == RELAY_ON) ? 1 : 0;
}

uint8_t IO_ToggleStatusOnRAM(uint32_t* ioStatus)
{
	
	if(*ioStatus == RELAY_ON)
	{
		*ioStatus = RELAY_OFF;
	}
	else
	{
		*ioStatus = RELAY_ON;
	}
	
	return (*ioStatus == RELAY_ON) ? 1 : 0;
}

uint8_t IO_GetStatusOnRAM(uint32_t ioStatus)
{
	return (ioStatus == RELAY_ON)  ? 1 : 0;
}


