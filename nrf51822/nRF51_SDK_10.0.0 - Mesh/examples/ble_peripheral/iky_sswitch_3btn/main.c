
//DFU - what is "Init packet file" (*.dat) and how to get it?
//C:\Program Files (x86)\Nordic Semiconductor\Master Control Panel\3.10.0.14\nrf>
//nrfutil dfu genpkg --application myapp.hex --application-version 0xffffffff --dev-type 0xffff --dev-revision 0xffff --sd-req 0x64,0x45,0x5a myapp.zip
//Zip created at myapp.zip

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_util_platform.h"
#include "main.h"
#include "hw_config.h"
#include "CY8CMBR3xxx_APIs.h"
#include "CY8CMBR3xxx_HostFunctions.h"
#include "CY8CMBR3xxx_Registers.h"
#include "MBR3_Configuration.h"
#include "app_timer_appsh.h"
#include "app_gpiote.h"
#include "app_pwm.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "io_control.h"
#include "system_config.h"
#include "crc8.h"
#include "IKYProtocol\iky_ble_app.h"
#include "i2c_driver.h"
#include "rbc_mesh.h"
#include "mesh_comm.h"

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device. */

#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_ADV_INTERVAL                128                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 80 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define TIMER_INTERVAL     							50
#define TOUCH_SCAN_INTERVAL     				APP_TIMER_TICKS(TIMER_INTERVAL, APP_TIMER_PRESCALER)
#define BUTTON_CLICK_INTERVAL     			APP_TIMER_TICKS(500, APP_TIMER_PRESCALER)
#define SYSTICK_TIMER_INTERVAL					APP_TIMER_TICKS(SYSTICK_INTERVAL, APP_TIMER_PRESCALER)

#define BOOT_MODE_ACTIVE								0x5A5A5A5A
#define BOOT_MODE_INACTIVE							0xA5A5A5A5

#define STOP_CHECK_PROXIMITY_TIMEOUT		250

#define MESH_ACCESS_ADDR        				(RBC_MESH_ACCESS_ADDRESS_BLE_ADV)   /**< Access address for the mesh to operate on. */
#define MESH_INTERVAL_MIN_MS    				(100)                               /**< Mesh minimum advertisement interval in milliseconds. */
#define MESH_CHANNEL            				(38)                                /**< BLE channel to operate on. Single channel only. */
#define MESH_CLOCK_SOURCE       				(NRF_CLOCK_LFCLKSRC_RC_250_PPM_TEMP_4000MS_CALIBRATION)    /**< Clock source used by the Softdevice. For calibrating timeslot time. */


static ble_nus_t                        m_nus;                                      /**< Structure to identify the Nordic UART Service. */
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */
static ble_uuid_t                       m_adv_uuids[] = {{BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}};  /**< Universally unique service identifier. */

uint32_t ioRelay_1_Status __attribute__((at(0x20003F00)));
uint32_t ioRelay_2_Status __attribute__((at(0x20003F04)));
uint32_t ioRelay_3_Status __attribute__((at(0x20003F08)));
uint32_t ioRelay_4_Status __attribute__((at(0x20003F0C)));

uint32_t bootModeFlag __attribute__((at(0x20003FFC)));

//uint8_t bHostInterruptFlag = 0;
bool CY8CMBR3xxx_isOK = false;
APP_PWM_INSTANCE(PWM1,1);

uint8_t buzzer_status = 0;
bool buzzer_ready_flag = false;

ble_gap_addr_t my_addr;

APP_TIMER_DEF(m_touch_timer_id);                                                 /**< Touch timer. */
APP_TIMER_DEF(m_btn_1_timer_id);
APP_TIMER_DEF(m_btn_2_timer_id);
APP_TIMER_DEF(m_systick_timer_id);

CY8CMBR3xxx_SENSORSTATUS sensorStatus;

Timeout_Type tTouchReinitTimeout;
Timeout_Type tStopCheckProximityTimeout;
Timeout_Type tButtonTouchTimeout;

bool m_nus_xfer_done = false;
static volatile bool ready_flag;            // A flag indicating PWM status.
LED_PROXIMITY_STATE ledProximityState = LED_PROXIMITY_OFF;

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of 
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) sysCfg.DeviceName,
                                          strlen((char*)sysCfg.DeviceName));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_nus    Nordic UART Service structure.
 * @param[in] p_data   Data to be send to UART module.
 * @param[in] length   Length of the data.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
			IKY_BLE_Rx_Callback(p_data[i]);
    }
		if(p_data[0] == 'd' && p_data[1] == 'f' && p_data[2] == 'u')
		{
			IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,5);
			bootModeFlag = BOOT_MODE_ACTIVE;	
		}
}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t       err_code;
    ble_nus_init_t nus_init;
    
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;
    
    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;
    
    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
    
    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;
    
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}



/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
		uint32_t               err_code;
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            break;
        case BLE_ADV_EVT_IDLE:
						NRF_LOG_PRINTF("BLE_ADV_EVT_IDLE\r\n");
						err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
						APP_ERROR_CHECK(err_code);
            break;
        default:
            break;
    }
}


/**@brief Function for the Application's S110 SoftDevice event handler.
 *
 * @param[in] p_ble_evt S110 SoftDevice event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
						NRF_LOG_PRINTF("BLE_GAP_EVT_CONNECTED\r\n");
						m_nus_xfer_done = false;
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
						NRF_LOG_PRINTF("BLE_GAP_EVT_DISCONNECTED\r\n");
						m_nus_xfer_done = false;
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            NRF_LOG_PRINTF("BLE_GAP_EVT_SEC_PARAMS_REQUEST\r\n");
						err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
						NRF_LOG_PRINTF("BLE_GATTS_EVT_SYS_ATTR_MISSING\r\n");
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

				case BLE_EVT_TX_COMPLETE:
						NRF_LOG_PRINTF("BLE_EVT_TX_COMPLETE\r\n");
            m_nus_xfer_done = false;
            break;
				
				case BLE_GATTC_EVT_TIMEOUT:
        case BLE_GATTS_EVT_TIMEOUT:
						NRF_LOG_PRINTF("BLE_GATTC_EVT_TIMEOUT\r\n");
            // Disconnect on GATT Server and Client timeout events.
            err_code = sd_ble_gap_disconnect(m_conn_handle,BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
				
        default:
						NRF_LOG_PRINTF("No implementation needed. 0x%x\r\n",p_ble_evt->header.evt_id);
            // No implementation needed.
            break;
    }
}

/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void sys_evt_dispatch(uint32_t event)
{
    pstorage_sys_event_handler(event);
		ble_advertising_on_sys_evt(event);
		rbc_mesh_sd_evt_handler(event);
}

/**@brief Function for dispatching a S110 SoftDevice event to all modules with a S110 SoftDevice 
 *        event handler.
 *
 * @details This function is called from the S110 SoftDevice event interrupt handler after a S110 
 *          SoftDevice event has been received.
 *
 * @param[in] p_ble_evt  S110 SoftDevice event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
		rbc_mesh_ble_evt_handler(p_ble_evt);
}


/**@brief Function for the S110 SoftDevice initialization.
 *
 * @details This function initializes the S110 SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
    
    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_4000MS_CALIBRATION, NULL);

    // Enable BLE stack.
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
#if (defined(S130) || defined(S132))
    ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    // Subscribe for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
	
		err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t scanrsp;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = false;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
		options.ble_adv_whitelist_enabled = BLE_ADV_WHITELIST_DISABLED;
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, &scanrsp, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**
* @brief RBC_MESH framework event handler. Defined in rbc_mesh.h. Handles
*   events coming from the mesh. Sets LEDs according to data
*
* @param[in] evt RBC event propagated from framework
*/
static void rbc_mesh_event_handler(rbc_mesh_event_t* p_evt)
{   
//		static const char cmd[] = {'U', 'C', 'N', 'I', 'T'};
		uint32_t      err_code;
    switch (p_evt->type)
    {
        case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL:
						NRF_LOG_PRINTF("RBC_MESH_EVENT_TYPE_CONFLICTING_VAL %d\r\n",p_evt->params.rx.timestamp_us);
//						err_code = rbc_mesh_value_set(p_evt->params.rx.value_handle,&p_evt->params.rx.p_data[0],1);
//						APP_ERROR_CHECK(err_code);
						break;
				
        case RBC_MESH_EVENT_TYPE_NEW_VAL:
        case RBC_MESH_EVENT_TYPE_UPDATE_VAL:
//						NRF_LOG_PRINTF("addr[%x][%x][%x][%x][%x][%x] \r\n", p_evt->params.rx.ble_adv_addr.addr[0]
//																																, p_evt->params.rx.ble_adv_addr.addr[1]
//																																, p_evt->params.rx.ble_adv_addr.addr[2]
//																																, p_evt->params.rx.ble_adv_addr.addr[3]
//																																, p_evt->params.rx.ble_adv_addr.addr[4]
//																																, p_evt->params.rx.ble_adv_addr.addr[5]);
//						NRF_LOG_PRINTF("event %c handle %d length %d \r\n", cmd[p_evt->type], p_evt->params.rx.value_handle,p_evt->params.rx.data_len);
//            NRF_LOG_PRINTF("%c[%d] \r\n", cmd[p_evt->type], p_evt->params.rx.value_handle);
						if(p_evt->params.rx.data_len == sizeof(mesh_comm_payload_t) && (mesh_comm_address_is_valid(p_evt->params.rx.p_data) == NRF_SUCCESS))
						{							
							mesh_comm_parser_payload((mesh_comm_payload_t *) p_evt->params.rx.p_data);
							
							err_code = rbc_mesh_value_set(p_evt->params.rx.value_handle,(uint8_t*)&mesh_comm_tx_payload,sizeof(mesh_comm_payload_t));
							APP_ERROR_CHECK(err_code);
						}
//						else
//						{
//							err_code = rbc_mesh_value_set(p_evt->params.rx.value_handle,&p_evt->params.rx.p_data[0],1);
//							APP_ERROR_CHECK(err_code);
//						}
            break;

        case RBC_MESH_EVENT_TYPE_TX:
//						NRF_LOG_PRINTF("RBC_MESH_EVENT_TYPE_TX\r\n");
            break;
				
        default:
            break;
    }
}

/**@brief Function for placing the application in low power state while waiting for events.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

static void wdt_init(void)
{
	NRF_WDT->CONFIG = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) | ( WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
	NRF_WDT->CRV = 3*32768;   //ca 3 sek. timout
	NRF_WDT->RREN |= WDT_RREN_RR0_Msk;  //Enable reload register 0
	NRF_WDT->TASKS_START = 1;
}

static void wdt_reload(void)
{
	NRF_WDT->RR[0] = WDT_RR_RR_Reload; //Reload watchdog register 0
}
/**@brief Application main function.
 */
int main(void)
{
    uint32_t 	err_code;
		uint32_t value;
		rbc_mesh_event_t evt;	
	
		bootModeFlag = BOOT_MODE_INACTIVE;
    // Initialize.
		io_control_init();
	
		timers_init();	
    ble_stack_init();
		sd_ble_gap_address_get(&my_addr);
	
		rbc_mesh_init_params_t init_params;

    init_params.access_addr = MESH_ACCESS_ADDR;
    init_params.interval_min_ms = MESH_INTERVAL_MIN_MS;
    init_params.channel = MESH_CHANNEL;
    init_params.lfclksrc = MESH_CLOCK_SOURCE;
    init_params.tx_power = RBC_MESH_TXPOWER_Pos4dBm ;
    
    uint32_t error_code = rbc_mesh_init(init_params);
    APP_ERROR_CHECK(error_code);
	
		CFG_Load();
		wdt_init();
		wdt_reload();
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
		
		NRF_LOG_INIT();
		NRF_LOG_PRINTF("IO INIT %x\r\n",PSTORAGE_DATA_START_ADDR);
		IKY_BLE_TaskInit();
	
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
	
    IOFilter_Init(&io_btn_1,NORMAL_FILTER_CNT,0,button_1_update_event_handler);
		IOFilter_Init(&io_btn_2,NORMAL_FILTER_CNT,0,button_2_update_event_handler);
		IOFilter_Init(&io_btn_3,NORMAL_FILTER_CNT,0,button_3_update_event_handler);
		IOFilter_Init(&io_proximity_1,PROXIMITY_FILTER_CNT,0,proximity_1_update_event_handler);
		
		boards_init();
		twi_init();
		
		NRF_LOG_PRINTF("Fw: %s\r\n",FW_RELEASE_DATE);
		update_mesh_comm_address();
		/* request values for both LEDs on the mesh */
		error_code = rbc_mesh_value_enable(1);
		APP_ERROR_CHECK(error_code);
		error_code = rbc_mesh_tx_event_set(1,true);
		APP_ERROR_CHECK(error_code);
		
		nrf_delay_ms(500);
		
		if (TRUE == CY8CMBR3xxx_Configure(SLAVE_ADDRESS, &CY8CMBR3108_LQXI_configuration[0]))
    {
			NRF_LOG_PRINTF(" OK\r\n");
			CY8CMBR3xxx_isOK = true;		

			IO_ToggleSetStatus(&io_led_red_1,50,50,IO_TOGGLE_ENABLE,1);			
			IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,2);
    }
		else
		{
			NRF_LOG_PRINTF(" ERROR\r\n");	
			CY8CMBR3xxx_isOK = false;
			
			IO_ToggleSetStatus(&io_led_red_1,50,50,IO_TOGGLE_ENABLE,10);
			IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,5);
		}
		InitTimeout(&tTouchReinitTimeout,SYSTICK_TIME_SEC(3));
		NRF_LOG_PRINTF("START\r\n");
		application_timers_start();
		InitTimeout(&tButtonTouchTimeout,SYSTICK_TIME_SEC(10));
    // Enter main loop.
    for (;;)
    {
				wdt_reload();
				if(bootModeFlag == BOOT_MODE_ACTIVE)
				{
					if(io_buzzer.enable != IO_TOGGLE_ENABLE) NVIC_SystemReset();
				}
				CFG_Save_Task();
				power_manage();
				IKY_BLE_Task();
				ble_nus_send_task();
				ResetMcuTask();
				
				if(CY8CMBR3xxx_isOK == false && CheckTimeout(&tTouchReinitTimeout) == SYSTICK_TIMEOUT)
				{
					twi_reinit();
					if (TRUE == CY8CMBR3xxx_Configure(SLAVE_ADDRESS, &CY8CMBR3108_LQXI_configuration[0]))
					{
						NRF_LOG_PRINTF(" OK 11\r\n");
						CY8CMBR3xxx_isOK = true;			
					}
					else
					{
						NRF_LOG_PRINTF(" ERROR 11\r\n");
						CY8CMBR3xxx_isOK = false;						
					}
					InitTimeout(&tTouchReinitTimeout,SYSTICK_TIME_SEC(3));
				}
				if (rbc_mesh_event_get(&evt) == NRF_SUCCESS)
        {   
            rbc_mesh_event_handler(&evt);
            rbc_mesh_event_release(&evt);
        }
				//reset password function
				if( (io_btn_1.bitOld == 0) && (io_btn_2.bitOld == 0) && (io_btn_3.bitOld == 0))
				{
					InitTimeout(&tButtonTouchTimeout,SYSTICK_TIME_SEC(10));
				}
				if(CheckTimeout(&tButtonTouchTimeout) == SYSTICK_TIMEOUT)
				{
					if(io_buzzer.enable != IO_TOGGLE_ENABLE)IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,5);
					memcpy(sysCfg.Password,DEFAULT_PASSWORD,DEVICE_PASSWORD_LENGTH);
					system_config_change = true;
					InitTimeout(&tButtonTouchTimeout,SYSTICK_TIME_SEC(10));					
				}
    }
}

void twi_init (void)
{
		#if 1	
		I2C_init();
		#else
    ret_code_t err_code;
    err_code = nrf_drv_twi_init(&m_twi_CY8CMBRxxxx, &twi_config, NULL /*Host_TWI_Handler*/, NULL);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_twi_enable(&m_twi_CY8CMBRxxxx);
		#endif
}

void twi_reinit (void)
{
		#if 1
	
		#else
    ret_code_t err_code;
		nrf_drv_twi_uninit(&m_twi_CY8CMBRxxxx);
    err_code = nrf_drv_twi_init(&m_twi_CY8CMBRxxxx, &twi_config, NULL /*Host_TWI_Handler*/, NULL);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_twi_enable(&m_twi_CY8CMBRxxxx);
	
		#endif
}

void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
		ready_flag = true;
}

//void CY8CMBR3xxx_Event_Handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low)
//{
//	bHostInterruptFlag = 1;
//}

/**
 * @brief Handler for timer events.
 */
void timer_touch_event_handler(void* p_context)
{	
	if(CY8CMBR3xxx_isOK)
	{
		if (TRUE == CY8CMBR3xxx_ReadSensorStatus(SLAVE_ADDRESS, &sensorStatus))
		{
			IOFilter_Update(&io_btn_1,sensorStatus.buttonStatus & TOUCH_CHANNEL_1_MASK);
			IOFilter_Update(&io_btn_2,sensorStatus.buttonStatus & TOUCH_CHANNEL_2_MASK);
			IOFilter_Update(&io_btn_3,sensorStatus.buttonStatus & TOUCH_CHANNEL_3_MASK);
			IOFilter_Update(&io_proximity_1,sensorStatus.proxStatus & PROXIMITY_CHANNEL_1_MASK);			
		}
		else
		{
			CY8CMBR3xxx_isOK = false;
		}
	}
	
	if(io_proximity_1.bitOld && CheckTimeout(&tStopCheckProximityTimeout) == SYSTICK_TIMEOUT)
	{
			IO_ToggleSetStatus(&io_led_red_1,1050,50,IO_TOGGLE_ENABLE,1);
	}
	
	if(IO_ToggleProcess(&io_led_red_1,TIMER_INTERVAL) == IO_STATUS_ON)
	{
		led_proximity_set_state(1);//nrf_gpio_pin_set(LED_BLUE_1);
	}
	else
	{
		led_proximity_set_state(0);//nrf_gpio_pin_clear(LED_BLUE_1);
	}
	

	if(IO_ToggleProcess(&io_buzzer,TIMER_INTERVAL) == IO_STATUS_ON)
	{
		if(buzzer_status == 0)app_pwm_channel_duty_set(&PWM1, 0, 50);
		buzzer_status = 1;
	}
	else
	{
		if(buzzer_status == 1)app_pwm_channel_duty_set(&PWM1, 0, 100);
		buzzer_status = 0;
	}
}


/**
 * @brief Handler for timer events.
 */
void out_update_event_handler(uint8_t status)
{ 
	
}

/**
 * @brief Handler for timer events.
 */
void button_1_update_event_handler(uint8_t status)
{  
	uint32_t err_code;	
	if(status)
	{
		IO_ToggleSetStatus(&io_led_red_1,3000,50,IO_TOGGLE_ENABLE,1);

		reportFlag |= OUTPUT_TOUCH_CONTROL_FLAG;
		
		IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,1);
		if(IO_ToggleStatusOnRAM(&ioRelay_1_Status))
		{
			nrf_gpio_pins_set(OUTPUT_1_MASK);
		}
		else
		{
			nrf_gpio_pins_clear(OUTPUT_1_MASK);
		}
		
		err_code = app_timer_stop(m_btn_1_timer_id);
		APP_ERROR_CHECK(err_code);
		err_code = app_timer_start(m_btn_1_timer_id, BUTTON_CLICK_INTERVAL, NULL);
		APP_ERROR_CHECK(err_code);
	}
}


/**
 * @brief Handler for timer events.
 */
void button_2_update_event_handler(uint8_t status)
{ 
	uint32_t err_code;	
	if(status)
	{
		IO_ToggleSetStatus(&io_led_red_1,3000,50,IO_TOGGLE_ENABLE,1);
		
		reportFlag |= OUTPUT_TOUCH_CONTROL_FLAG;
		
		IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,1);
		if(IO_ToggleStatusOnRAM(&ioRelay_2_Status))
		{
			nrf_gpio_pins_set(OUTPUT_2_MASK);
		}
		else
		{
			nrf_gpio_pins_clear(OUTPUT_2_MASK);
		}		
		
		err_code = app_timer_stop(m_btn_2_timer_id);
		APP_ERROR_CHECK(err_code);
		err_code = app_timer_start(m_btn_2_timer_id, BUTTON_CLICK_INTERVAL, NULL);
		APP_ERROR_CHECK(err_code);
	}
}


/**
 * @brief Handler for timer events.
 */
void button_3_update_event_handler(uint8_t status)
{  
	if(status)
	{
		IO_ToggleSetStatus(&io_led_red_1,3000,50,IO_TOGGLE_ENABLE,1);
		
		reportFlag |= OUTPUT_TOUCH_CONTROL_FLAG;
		
		IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,1);
		if(IO_ToggleStatusOnRAM(&ioRelay_3_Status))
		{
			nrf_gpio_pins_set(OUTPUT_3_MASK);
		}
		else
		{
			nrf_gpio_pins_clear(OUTPUT_3_MASK);
		}
	}
}

/**
 * @brief Handler for timer events.
 */
void proximity_1_update_event_handler(uint8_t status)
{  
//	if(status && CheckTimeout(&tStopCheckProximityTimeout) == SYSTICK_TIMEOUT)
//	{
//			IO_ToggleSetStatus(&io_buzzer,200,100,IO_TOGGLE_ENABLE,1);
//	}
}

void io_control_init(void)
{
	APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
	
	nrf_gpio_cfg_output(RELAY_1_PIN);
	nrf_gpio_cfg_output(RELAY_2_PIN);
	nrf_gpio_cfg_output(RELAY_3_PIN);
	nrf_gpio_cfg_output(RELAY_4_PIN);
	
	nrf_gpio_cfg_output(LED_GREEN_1);
	nrf_gpio_cfg_output(LED_GREEN_2);
	nrf_gpio_cfg_output(LED_GREEN_3);
	nrf_gpio_cfg_output(LED_GREEN_4);
	
	if(IO_ReadStatusOnRAM(&ioRelay_1_Status))
	{
		nrf_gpio_pins_set(OUTPUT_1_MASK);
	}
	else nrf_gpio_pins_clear(OUTPUT_1_MASK);
	
	if(IO_ReadStatusOnRAM(&ioRelay_2_Status))
	{
		nrf_gpio_pins_set(OUTPUT_2_MASK);
	}
	else nrf_gpio_pins_clear(OUTPUT_2_MASK);
	
	if(IO_ReadStatusOnRAM(&ioRelay_3_Status))
	{
		nrf_gpio_pins_set(OUTPUT_3_MASK);
	}
	else nrf_gpio_pins_clear(OUTPUT_3_MASK);
	
	if(IO_ReadStatusOnRAM(&ioRelay_4_Status))
	{
		nrf_gpio_pins_set(OUTPUT_4_MASK);
	}
	else nrf_gpio_pins_clear(OUTPUT_4_MASK);
}

void boards_init(void)
{
	uint32_t err_code;
	
	nrf_gpio_cfg_output(LED_BLUE_1);
	
	/* 1-channel PWM, 2KHz, output on DK LED pins. */
	app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(500L,BUZZER_PIN,LED_BLUE_1);
	/* Switch the polarity of the second channel. */
	pwm1_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_HIGH;
    
	/* Initialize and enable PWM. */
	err_code = app_pwm_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
	APP_ERROR_CHECK(err_code);
	app_pwm_enable(&PWM1);
}

void timer_btn_1_event_handler(void* p_context)
{
	if(io_btn_1.updateHighCnt == 10)	
	{
		IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,5);
		bootModeFlag = BOOT_MODE_ACTIVE;		
	}
	io_btn_1.updateHighCnt = 0;
}

void timer_btn_2_event_handler(void* p_context)
{
	if(io_btn_2.updateHighCnt == 10)	
	{
		IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,5);
		bootModeFlag = BOOT_MODE_ACTIVE;
	}
	io_btn_2.updateHighCnt = 0;
}

void timer_systick_event_handler(void* p_context)
{
	SysTick___Task();
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
void timers_init(void)
{
    uint32_t err_code;

    // Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    // Create timers.
    err_code = app_timer_create(&m_touch_timer_id,APP_TIMER_MODE_REPEATED, timer_touch_event_handler);
    APP_ERROR_CHECK(err_code);
		err_code = app_timer_create(&m_btn_1_timer_id,APP_TIMER_MODE_SINGLE_SHOT, timer_btn_1_event_handler);
		APP_ERROR_CHECK(err_code);
		err_code = app_timer_create(&m_btn_2_timer_id,APP_TIMER_MODE_SINGLE_SHOT, timer_btn_2_event_handler);
		APP_ERROR_CHECK(err_code);
		err_code = app_timer_create(&m_systick_timer_id,APP_TIMER_MODE_REPEATED, timer_systick_event_handler);
		APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting application timers.
 */
void application_timers_start(void)
{
    uint32_t err_code;

    // Start application timers.
    err_code = app_timer_start(m_touch_timer_id, TOUCH_SCAN_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);	
		err_code = app_timer_start(m_systick_timer_id, SYSTICK_TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


void ble_nus_send_task(void)
{
	uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
	uint8_t index = 0;
	static Timeout_Type tNusSendTimeout;
	uint8_t c;
	uint16_t i;
	uint32_t err_code;
	
	if(m_nus_xfer_done && CheckTimeout(&tNusSendTimeout) == SYSTICK_TIMEOUT) //send timeout
	{
		m_nus_xfer_done = false;
	}
	
	if(m_nus_xfer_done) return; //sending
		
	for(i=0;i<BLE_NUS_MAX_DATA_LEN;i++)
	{
		if(RINGBUF_Get(&IKYBLE_TxRingBuf,&c) == 0)
		{
			data_array[index] = c;
			index++;
		}
		else
		{
			 break; //data empty
		}
	}
	
	if (index)
	{
		NRF_LOG_PRINTF("ble nus send %d bytes ...",index);
		err_code = ble_nus_string_send(&m_nus, data_array,index);
		if(err_code == NRF_SUCCESS)
		{
			m_nus_xfer_done = true;
			NRF_LOG_PRINTF("success\r\n");
		}
		else
		{
			m_nus_xfer_done = false;
			NRF_LOG_PRINTF("fail %x\r\n",err_code);
		}
		
		if(m_nus_xfer_done) InitTimeout(&tNusSendTimeout,500);		
		index = 0;
	}
}

void led_proximity_set_state(uint8_t __led)
{
	uint8_t relay_status  = 0;
	LED_PROXIMITY_STATE newState = LED_PROXIMITY_OFF;
	
	if(IO_GetStatusOnRAM(ioRelay_1_Status)) relay_status |=  MESH_COMM_OUTPUT_1_MSK;
	if(IO_GetStatusOnRAM(ioRelay_2_Status)) relay_status |=  MESH_COMM_OUTPUT_2_MSK;
	if(IO_GetStatusOnRAM(ioRelay_3_Status)) relay_status |=  MESH_COMM_OUTPUT_3_MSK;
	if(IO_GetStatusOnRAM(ioRelay_4_Status)) relay_status |=  MESH_COMM_OUTPUT_4_MSK;
	
	if(__led)
	{
			newState = LED_PROXIMITY_ON;
	}
	else
	{
		if(relay_status) //Chỉ Tắt led proximity khi có ít nhất 1 relay bật
		{
				newState = LED_PROXIMITY_OFF;
		}	
		else
		{
				newState = LED_PROXIMITY_DIM;
		}
	}
		
	if(newState != ledProximityState)
	{
			ledProximityState = newState;
			if(ledProximityState == LED_PROXIMITY_ON) app_pwm_channel_duty_set(&PWM1, 1, 100);
			else if(ledProximityState == LED_PROXIMITY_DIM) app_pwm_channel_duty_set(&PWM1, 1, 5);
			else app_pwm_channel_duty_set(&PWM1, 1, 0);
	}
}
/** 
 * @}
 */
