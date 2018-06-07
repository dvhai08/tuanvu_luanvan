#include "system_config.h"
#include "nrf_error.h"
#include "app_error.h"
#include "nrf_log.h"
#include "lib\sys_tick.h"

extern ble_gap_addr_t my_addr;

uint8_t sysResetMcuFlag = MCU_RESET_NONE;
static Timeout_Type tMcuResetTimeout;

system_config_t sysCfg;
bool system_config_change = false;
static pstorage_handle_t m_syscfg_handle;

static void pstorage_callback_handler(pstorage_handle_t * p_handle,
                                      uint8_t             op_code,
                                      uint32_t            result,
                                      uint8_t           * p_data,
                                      uint32_t            data_len)
{      
	NRF_LOG_PRINTF("\r\npstorage_callback_handler %d\r\n",op_code);
}


uint32_t system_config_is_valid(system_config_t *p_settings)
{
	return 1;
}


uint32_t system_config_calc_checksum(system_config_t *sysCfg)
{
	uint32_t u32Temp = 0;
	int16_t i;
	uint8_t *u8Pt;
	u8Pt = (uint8_t *)sysCfg;	
	for(i = 0;i < sizeof(sysCfg)-sizeof(sysCfg->crc);i++)
	{
		u32Temp += u8Pt[i];
	}
	u32Temp = ~u32Temp;
	u32Temp += 1;
	return u32Temp;
}

static void system_config_init(void)
{
    uint32_t                err_code;
    pstorage_module_param_t storage_params;

		storage_params.cb          = pstorage_callback_handler;
		storage_params.block_size  = sizeof(system_config_t);
		storage_params.block_count = 1;
		
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);   
	
    err_code = pstorage_register(&storage_params, &m_syscfg_handle);
    APP_ERROR_CHECK(err_code);
}		

static void system_config_save(system_config_t * p_settings)
{
		pstorage_handle_t block_handle;
		uint32_t err_code;
	
		//Get the block handle.
    err_code = pstorage_block_identifier_get(&m_syscfg_handle, 0, &block_handle);
		APP_ERROR_CHECK(err_code);
		    
		err_code = pstorage_clear(&block_handle, sizeof(system_config_t));
		APP_ERROR_CHECK(err_code);		

    err_code = pstorage_store(&block_handle,(uint8_t *)p_settings,sizeof(system_config_t),0);
    APP_ERROR_CHECK(err_code);
}

static void system_config_load(system_config_t * p_settings)
{
    uint32_t err_code;    

    err_code = pstorage_load((uint8_t *)p_settings,
                              &m_syscfg_handle,
                              sizeof(system_config_t),
                              0);
    APP_ERROR_CHECK(err_code);
}


void CFG_Reload(void)
{
	system_config_load(&sysCfg);
}

void CFG_Load(void)
{	
	uint32_t saveFlag = 0;
	
	system_config_init();
	
	system_config_load(&sysCfg);
			
	if(system_config_is_valid(&sysCfg) == 0)
	{		
//		sysCfg.updateCnt = 0;
		saveFlag = 1;
	}
	
	if(sysCfg.DeviceName[0] == 0xFF)
	{
		memset(sysCfg.DeviceName,0x00,sizeof(sysCfg.DeviceName));
		sprintf((char *)sysCfg.DeviceName,"%s #%d",DEFAULT_DEVICE_NAME,((uint16_t) my_addr.addr[4] << 8) | (my_addr.addr[5]));
		saveFlag = 1;
	}
	
	if(sysCfg.Password[0] == 0xff)
	{
		memset(sysCfg.Password,0,sizeof(sysCfg.Password));
		memcpy((char *)sysCfg.Password,DEFAULT_PASSWORD,sizeof(sysCfg.Password));
		saveFlag = 1;
	}
	
	if(saveFlag)
	{
		NRF_LOG_PRINTF("CFG_Load> saveFlag\r\n");
		system_config_save(&sysCfg);
	}
}



void CFG_Save_Task(void)
{
	uint32_t err_code,count;
	
	if(system_config_change)
	{
			// Verify if there are any pending flash operations. If so, delay save config until
			// the flash operations are complete.
			err_code = pstorage_access_status_get(&count);
			
			if (err_code == NRF_ERROR_INVALID_STATE)
			{
					// Pstorage is not initialized, i.e. not in use.
					count = 0;
			}
			else if (err_code != NRF_SUCCESS)
			{
					return;
			}

			if (count != 0)
			{
					return;
			}
			system_config_change = false;
			sysCfg.crc = system_config_calc_checksum(&sysCfg);
			system_config_save(&sysCfg);
	}
}

void ResetMcuSet(uint8_t resetType)
{
	sysResetMcuFlag = resetType;
	InitTimeout(&tMcuResetTimeout,SYSTICK_TIME_SEC(1));
}

void ResetMcuTask(void)
{
		switch(sysResetMcuFlag)
		{
			case MCU_RESET_NONE:

				break;
			
			case MCU_RESET_IMMEDIATELY:
				NVIC_SystemReset();
				break;
			
			case MCU_RESET_AFTER_1_SEC:
				InitTimeout(&tMcuResetTimeout,SYSTICK_TIME_SEC(1));
				sysResetMcuFlag = MCU_RESET_IS_WAITING;
				break;
			
			case MCU_RESET_AFTER_2_SEC:
				InitTimeout(&tMcuResetTimeout,SYSTICK_TIME_SEC(2));
				sysResetMcuFlag = MCU_RESET_IS_WAITING;
				break;
			
			case MCU_RESET_AFTER_5_SEC:
				InitTimeout(&tMcuResetTimeout,SYSTICK_TIME_SEC(5));
				sysResetMcuFlag = MCU_RESET_IS_WAITING;
				break;
			
			case MCU_RESET_AFTER_10_SEC:
				InitTimeout(&tMcuResetTimeout,SYSTICK_TIME_SEC(10));
				sysResetMcuFlag = MCU_RESET_IS_WAITING;
				break; 
			
			case MCU_RESET_AFTER_30_SEC:
				InitTimeout(&tMcuResetTimeout,SYSTICK_TIME_SEC(30));	
				sysResetMcuFlag = MCU_RESET_IS_WAITING;
				break;
			
			case MCU_RESET_IS_WAITING:
				if(CheckTimeout(&tMcuResetTimeout) == SYSTICK_TIMEOUT)
				{
					NVIC_SystemReset();
				}
				break;
				
			default:
				NVIC_SystemReset();
				break;
		}
}

