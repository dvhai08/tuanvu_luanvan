#ifndef MAIN_H__
#define MAIN_H__

#include <stdint.h>


void twi_init (void);
void twi_reinit (void);
void io_control_init(void);
void boards_init(void);
void timers_init(void);
void application_timers_start(void);
void button_1_update_event_handler(uint8_t status);
void button_2_update_event_handler(uint8_t status);
void button_3_update_event_handler(uint8_t status);
void proximity_1_update_event_handler(uint8_t status);
void out_update_event_handler(uint8_t status);
void ble_nus_send_task(void);
void led_proximity_set_state(uint8_t __led);
#endif // MAIN_H__

