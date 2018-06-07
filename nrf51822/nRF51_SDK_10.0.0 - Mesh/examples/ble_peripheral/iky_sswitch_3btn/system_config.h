
#ifndef __SYSTEM_CONFIG_H__
#define __SYSTEM_CONFIG_H__
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "nrf51.h"
#include "pstorage.h"
#include <stdio.h>
#include "boards.h"
#include "ble_gap.h"

#define RELAY_ON									0xa5a5a5a5
#define RELAY_OFF 								0x5a5a5a5a

#define SYSTEM_CONFIG_ADDRESS

#define DEVICE_PASSWORD_LENGTH		4
#define DEVICE_NAME_LENGTH				20


#define MCU_RESET_NONE						0
#define MCU_RESET_IMMEDIATELY 		1
#define MCU_RESET_AFTER_10_SEC 		2
#define MCU_RESET_AFTER_30_SEC 		3
#define MCU_RESET_IS_WAITING 			4
#define MCU_RESET_AFTER_5_SEC			5
#define MCU_RESET_AFTER_2_SEC			6
#define MCU_RESET_AFTER_1_SEC			7


#define FIRMWARE_VERSION						"1.0.0.7"
#define DEFAULT_DEVICE_NAME					"itot"
#define DEFAULT_RADIO_ACCESS_ADDR 	0xA541A68F
#define DEFAULT_RADIO_CHANNEL 			38
#define DEFAULT_RSSI								80
#define DEFAULT_PASSWORD						"1234"
//
#define FW_RELEASE_DATE						__DATE__ " " __TIME__


typedef struct __attribute__((packed)){
	uint32_t size;
	uint8_t Password[DEVICE_PASSWORD_LENGTH];
	uint8_t DeviceName[DEVICE_NAME_LENGTH];
	uint32_t crc;
}system_config_t;

extern system_config_t sysCfg;
extern bool system_config_change;

void CFG_Load(void);
void CFG_Reload(void);
void CFG_Save_Task(void);

void ResetMcuSet(uint8_t resetType);
void ResetMcuTask(void);
#endif
