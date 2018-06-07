
#include "mesh_comm.h"
#include "rbc_mesh.h"
#include <string.h>
#include "ble_gap.h"
#include "io_control.h"
#include "nrf_log.h"
#include "app_error.h"

extern ble_gap_addr_t my_addr;
// address parameters
static uint8_t            mesh_comm_addr[MESH_COMM_ADDR_SIZE];
mesh_comm_payload_t 			mesh_comm_tx_payload;
mesh_comm_payload_t 			mesh_comm_rx_payload;

void update_mesh_comm_address(void)
{
    memcpy(mesh_comm_addr, my_addr.addr, MESH_COMM_ADDR_SIZE);
		memcpy(mesh_comm_tx_payload.base_addr, my_addr.addr, MESH_COMM_ADDR_SIZE);
}

uint32_t mesh_comm_address_is_valid(uint8_t *addr)
{    
    if (memcmp(mesh_comm_addr,addr,MESH_COMM_ADDR_SIZE))
    {
				NRF_LOG_PRINTF("NRF_ERROR_INVALID_ADDR\r\n");
        return NRF_ERROR_INVALID_ADDR;
    }
		return NRF_SUCCESS;
}

uint32_t mesh_comm_update_tx_payload(uint8_t value)
{
	uint32_t err_code;
	
	mesh_comm_tx_payload.opcode  = MESH_COMM_OPCODE_RESPONSE;
	mesh_comm_tx_payload.index  = MESH_COMM_OPCODE_RESPONSE;
	mesh_comm_tx_payload.value  = value;
	err_code = rbc_mesh_value_set(MESH_RESPONSE_HANDLE,(uint8_t*)&mesh_comm_tx_payload,sizeof(mesh_comm_payload_t));
	
	return err_code;
}

uint32_t mesh_comm_parser_payload(mesh_comm_payload_t * p_payload)
{
		uint8_t preStatus = 0;
	
		if(mesh_comm_address_is_valid(p_payload->base_addr) != NRF_SUCCESS)
		{
			NRF_LOG_PRINTF("NRF_ERROR_INVALID_ADDR\r\n");
			return NRF_ERROR_INVALID_ADDR;
		}
		NRF_LOG_PRINTF("p_payload->opcode %d \r\n",p_payload->opcode);
    switch(p_payload->opcode)
		{
			case MESH_COMM_OPCODE_CONTROL:
					//
					preStatus  = 0;
					if(IO_GetStatusOnRAM(ioRelay_1_Status)) preStatus |=  MESH_COMM_OUTPUT_1_MSK;
					if(IO_GetStatusOnRAM(ioRelay_2_Status)) preStatus |=  MESH_COMM_OUTPUT_2_MSK;
					if(IO_GetStatusOnRAM(ioRelay_3_Status)) preStatus |=  MESH_COMM_OUTPUT_3_MSK;
					if(IO_GetStatusOnRAM(ioRelay_4_Status)) preStatus |=  MESH_COMM_OUTPUT_4_MSK;
					//MESH_COMM_OUTPUT_1_MSK
					NRF_LOG_PRINTF("p_payload->index %d \r\n",p_payload->index);
					NRF_LOG_PRINTF("p_payload->value %d \r\n",p_payload->value);
					if(p_payload->index & MESH_COMM_OUTPUT_1_MSK)
					{
						if(IO_UpdateStatusOnRAM(&ioRelay_1_Status, p_payload->value & MESH_COMM_OUTPUT_1_MSK))
						{
							nrf_gpio_pins_set(OUTPUT_1_MASK);
						}
						else nrf_gpio_pins_clear(OUTPUT_1_MASK);
					}
					//MESH_COMM_OUTPUT_2_MSK
					if(p_payload->index & MESH_COMM_OUTPUT_2_MSK)
					{
						if(IO_UpdateStatusOnRAM(&ioRelay_2_Status, p_payload->value & MESH_COMM_OUTPUT_2_MSK))
						{
							nrf_gpio_pins_set(OUTPUT_2_MASK);
						}
						else nrf_gpio_pins_clear(OUTPUT_2_MASK);
					}
					//MESH_COMM_OUTPUT_3_MSK
					if(p_payload->index & MESH_COMM_OUTPUT_3_MSK)
					{
						if(IO_UpdateStatusOnRAM(&ioRelay_3_Status, p_payload->value & MESH_COMM_OUTPUT_3_MSK))
						{
							nrf_gpio_pins_set(OUTPUT_3_MASK);
						}
						else nrf_gpio_pins_clear(OUTPUT_3_MASK);
					}
					//MESH_COMM_OUTPUT_4_MSK
					if(p_payload->index & MESH_COMM_OUTPUT_4_MSK)
					{
						if(IO_UpdateStatusOnRAM(&ioRelay_4_Status, p_payload->value & MESH_COMM_OUTPUT_4_MSK))
						{
							nrf_gpio_pins_set(OUTPUT_4_MASK);
						}
						else nrf_gpio_pins_clear(OUTPUT_4_MASK);
					}
					//Response
					mesh_comm_tx_payload.opcode  = MESH_COMM_OPCODE_RESPONSE;
					mesh_comm_tx_payload.index  = MESH_COMM_OPCODE_RESPONSE;
					mesh_comm_tx_payload.value  = 0;
					if(IO_GetStatusOnRAM(ioRelay_1_Status)) mesh_comm_tx_payload.value |=  MESH_COMM_OUTPUT_1_MSK;
					if(IO_GetStatusOnRAM(ioRelay_2_Status)) mesh_comm_tx_payload.value |=  MESH_COMM_OUTPUT_2_MSK;
					if(IO_GetStatusOnRAM(ioRelay_3_Status)) mesh_comm_tx_payload.value |=  MESH_COMM_OUTPUT_3_MSK;
					if(IO_GetStatusOnRAM(ioRelay_4_Status)) mesh_comm_tx_payload.value |=  MESH_COMM_OUTPUT_4_MSK;
					//BipBip
					if(mesh_comm_tx_payload.value != preStatus) //Còi bip khi trạng thái thay đổi
					{
						if(io_buzzer.enable != IO_TOGGLE_ENABLE)IO_ToggleSetStatus(&io_buzzer,100,100,IO_TOGGLE_ENABLE,1);
					}
					break;
			
			case MESH_COMM_OPCODE_RESPONSE:
					break;
			
			default:
				break;
		}
		p_payload->opcode = 0;
    return NRF_SUCCESS;
}
