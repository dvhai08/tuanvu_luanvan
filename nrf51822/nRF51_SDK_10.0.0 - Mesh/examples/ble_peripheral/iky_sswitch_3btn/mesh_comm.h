

#ifndef __MESH_COMM_H
#define __MESH_COMM_H

#include <stdbool.h>
#include <stdint.h>
#include "nrf_error.h"
#include "app_util.h"


#define     MESH_COMM_ADDR_SIZE                	6

#define     MESH_COMM_OPCODE_CONTROL	          10
#define     MESH_COMM_OPCODE_RESPONSE           11
// Mesh handle
#define 		MESH_CONTROL_HANDLE									0x1010
#define 		MESH_RESPONSE_HANDLE								0x0001
// Device mask control
#define     MESH_COMM_OUTPUT_1_MSK          		0x01
#define     MESH_COMM_OUTPUT_2_MSK           		0x02
#define     MESH_COMM_OUTPUT_3_MSK              0x04
#define     MESH_COMM_OUTPUT_4_MSK              0x08


/**@brief Mesh comm payload.
*/
typedef struct __attribute__((packed)){

    uint8_t base_addr[6];
    uint8_t opcode;
    uint8_t index;
		uint8_t value;
} mesh_comm_payload_t;

extern mesh_comm_payload_t 			mesh_comm_rx_payload;
extern mesh_comm_payload_t 			mesh_comm_tx_payload;

void update_mesh_comm_address(void);
uint32_t mesh_comm_address_is_valid(uint8_t *addr);
uint32_t mesh_comm_update_tx_payload(uint8_t value);
uint32_t mesh_comm_parser_payload(mesh_comm_payload_t * p_payload);

/** @} */
#endif //__MESH_COMM_H
