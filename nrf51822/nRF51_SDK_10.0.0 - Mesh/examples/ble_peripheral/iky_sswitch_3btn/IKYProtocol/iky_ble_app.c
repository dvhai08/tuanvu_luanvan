
#include "iky_ble_app.h"
#include "system_config.h"
#include "iky_encrypt.h"
#include "lib\sys_tick.h"
#include "nrf_log.h"
#include "mesh_comm.h"
#include "io_control.h"

////CA 88 05 BA 1B 06 5C 01 38 -- PASS: 8888
////ca 5 c 9e 2b 06 6b b0 dc 3a 21 64 de 49 73 1f
extern uint32_t ioRelay_1_Status;
extern uint32_t ioRelay_2_Status;
extern uint32_t ioRelay_3_Status;
extern uint32_t ioRelay_4_Status;

uint8_t IKY_BLE_PutReport(IKY_BLE_PACKET_TYPE *TxReport, uint8_t opcode,uint8_t * p_data,uint8_t len);

IKY_BLE_PARSER_PACKET_TYPE IKYBLE__ParserPacket;

IKY_BLE_PACKET_TYPE IKYBLE__ProtoSend;
IKY_BLE_PACKET_TYPE IKYBLE__ProtoRecv;
IKY_BLE_PACKET_TYPE IKYBLE__ProtoReport;

uint8_t IKYBLE__ProtoSend_Buff[32];
uint8_t IKYBLE__ProtoRecv_Buff[32];
uint8_t IKYBLE__ProtoReport_Buff[32];

RINGBUF IKYBLE_TxRingBuf;
uint8_t IKYBLE__TxBuf[64];

uint32_t reportFlag = 0;
RESET_PIN_STATE_TYPE rsPINState = RESET_PIN_IDLE;

void IKY_BLE_TaskInit(void)
{
	memset(&IKYBLE__ProtoSend,0,sizeof(IKY_BLE_PACKET_TYPE));
	memset(&IKYBLE__ProtoRecv,0,sizeof(IKY_BLE_PACKET_TYPE));
	memset(&IKYBLE__ProtoReport,0,sizeof(IKY_BLE_PACKET_TYPE));
	
	RINGBUF_Init(&IKYBLE_TxRingBuf,IKYBLE__TxBuf,sizeof(IKYBLE__TxBuf));
	
	IKYBLE__ParserPacket.state = IKY_BLE_CMD_WAITING_SATRT_CODE;
	IKYBLE__ParserPacket.lenMax = (uint8_t)sizeof(IKYBLE__ProtoRecv_Buff);
	
	IKYBLE__ProtoSend.dataPt = IKYBLE__ProtoSend_Buff;
	IKYBLE__ProtoRecv.dataPt = IKYBLE__ProtoRecv_Buff;
	IKYBLE__ProtoReport.dataPt = IKYBLE__ProtoReport_Buff;
}

void IKY_BLE_Rx_Callback(uint8_t c)
{
			if(IKY_BLE_ParserPacket(&IKYBLE__ParserPacket,&IKYBLE__ProtoRecv,c) == 0)
			{
				IKY_BLE_ProcessData(&IKYBLE__ProtoRecv,&IKYBLE__ProtoSend,IKYBLE__ParserPacket.lenMax);				
			}	
}

void IKY_BLE_Task(void)
{
	uint8_t value;
	uint16_t i;
	uint8_t tmpbuff[16];
	
	IKY_BLE_PutData(&IKYBLE_TxRingBuf,&IKYBLE__ProtoSend);
	IKY_BLE_PutData(&IKYBLE_TxRingBuf,&IKYBLE__ProtoReport);
	
	if(reportFlag & OUTPUT_REPORT_FLAG)
	{
		reportFlag &= ~OUTPUT_REPORT_FLAG;
		i = 0;
		tmpbuff[i++] = IO_GetStatusOnRAM(ioRelay_1_Status);
		tmpbuff[i++] = IO_GetStatusOnRAM(ioRelay_2_Status);
		tmpbuff[i++] = IO_GetStatusOnRAM(ioRelay_3_Status);
		tmpbuff[i++] = IO_GetStatusOnRAM(ioRelay_4_Status);
		IKY_BLE_PutReport(&IKYBLE__ProtoReport, PACKET_OPCODE_OUTPUT_STATUS, tmpbuff, i);			
	}
	
	if(reportFlag & OUTPUT_TOUCH_CONTROL_FLAG)
	{
		reportFlag &= ~ OUTPUT_TOUCH_CONTROL_FLAG;
		
		value  = 0;
		if(IO_GetStatusOnRAM(ioRelay_1_Status)) value |=  MESH_COMM_OUTPUT_1_MSK;
		if(IO_GetStatusOnRAM(ioRelay_2_Status)) value |=  MESH_COMM_OUTPUT_2_MSK;
		if(IO_GetStatusOnRAM(ioRelay_3_Status)) value |=  MESH_COMM_OUTPUT_3_MSK;
		if(IO_GetStatusOnRAM(ioRelay_4_Status)) value |=  MESH_COMM_OUTPUT_4_MSK;
		mesh_comm_update_tx_payload(value);		
	}
	
	if(rsPINState == RESET_PIN_STEP2)
	{
		memset(sysCfg.Password,0,sizeof(sysCfg.Password));
		memcpy((char *)sysCfg.Password,DEFAULT_PASSWORD,sizeof(sysCfg.Password));
		system_config_change = true;
		rsPINState = RESET_PIN_IDLE;
	}
}

uint8_t IKY_BLE_ParserPacket(IKY_BLE_PARSER_PACKET_TYPE *parserPacket,IKY_BLE_PACKET_TYPE *IKYBLEProtoRecv,uint8_t c)
{
	switch(parserPacket->state)
	{
		case IKY_BLE_CMD_WAITING_SATRT_CODE:
			if(c == 0xCA)
			{
				parserPacket->state = IKY_BLE_CMD_GET_OPCODE;
				parserPacket->len = 0;
				parserPacket->crc = 0;
				parserPacket->cnt = 0;
			}			
			break;
			
		case IKY_BLE_CMD_GET_OPCODE:
			parserPacket->opcode = c;
			parserPacket->state = IKY_BLE_CMD_GET_LENGTH;
			break;
		
		case IKY_BLE_CMD_GET_LENGTH:		
			parserPacket->len = c;
			parserPacket->state = IKY_BLE_CMD_GET_DATA;
			break;
		
		case IKY_BLE_CMD_GET_DATA:
			if((parserPacket->cnt >= parserPacket->len) || (parserPacket->len > parserPacket->lenMax))
			{
				parserPacket->state = IKY_BLE_CMD_WAITING_SATRT_CODE;
			}
			else
			{
				parserPacket->crc += c;
				IKYBLEProtoRecv->dataPt[parserPacket->cnt]= c;
				parserPacket->cnt++;
				if(parserPacket->cnt == parserPacket->len)
				{
						parserPacket->state = IKY_BLE_CMD_CRC_CHECK;
				}
			}
			break;
			
		case IKY_BLE_CMD_CRC_CHECK:
			parserPacket->state= IKY_BLE_CMD_WAITING_SATRT_CODE;
			if(parserPacket->crc  == c)
			{	
					IKYBLEProtoRecv->length = parserPacket->len;
					IKYBLEProtoRecv->opcode = parserPacket->opcode;
					return 0;
			}
			break;
			
		default:
			parserPacket->state = IKY_BLE_CMD_WAITING_SATRT_CODE;
			break;
	}
	return 0xff;
}

uint8_t IKY_BLE_ProcessData(IKY_BLE_PACKET_TYPE *IKYBLEProtoRecv,IKY_BLE_PACKET_TYPE *IKYBLEProtoSend,uint32_t maxPacketSize)
{
	uint8_t ResponseCode = RESPONSE_CODE_APPROVED;
	uint8_t ResponseFlag = 1;
	uint32_t u32Temp = 0;
	uint32_t u32PINRecv = 0xF6B41C65;
	uint8_t PINisOK = 0;
	uint8_t u8Tmp1[16];
	uint8_t u8Tmp2[16];

	//verify PIN
	if(IKYBLEProtoRecv->length >= DEVICE_PASSWORD_LENGTH)
	{
		memcpy(u8Tmp1,sysCfg.Password,DEVICE_PASSWORD_LENGTH);
		memcpy(&u8Tmp1[DEVICE_PASSWORD_LENGTH],&IKYBLEProtoRecv->dataPt[ENCRYPT_BLOCK_PIN_LENGTH],IKYBLEProtoRecv->length - ENCRYPT_BLOCK_PIN_LENGTH);
		u32Temp = IKY_CalcPINBlock(u8Tmp1,DEVICE_PASSWORD_LENGTH + IKYBLEProtoRecv->length - ENCRYPT_BLOCK_PIN_LENGTH);
		u32PINRecv = 	(uint32_t)IKYBLEProtoRecv->dataPt[0]<<24;
		u32PINRecv |= (uint32_t)IKYBLEProtoRecv->dataPt[1]<<16;
		u32PINRecv |= (uint32_t)IKYBLEProtoRecv->dataPt[2]<<8;
		u32PINRecv |= (uint32_t)IKYBLEProtoRecv->dataPt[3];
		if(u32Temp == u32PINRecv) 
		{
			PINisOK = 1;
			NRF_LOG_PRINTF("\r\nPIN is OK\r\n");
			memcpy(u8Tmp2,sysCfg.Password,DEVICE_PASSWORD_LENGTH);
		}
	}
	else 
	{
		return 0;
	}

	NRF_LOG_PRINTF("\r\nOPCODE %x\r\n",IKYBLEProtoRecv->opcode);
	//
	switch(IKYBLEProtoRecv->opcode)
	{
		case PACKET_OPCODE_LOGIN:
			if(!PINisOK)
			{
				ResponseCode = RESPONSE_CODE_INCORRECT_PIN;
				break;
			}
			break;
			
		case PACKET_OPCODE_OUTPUT_STATUS:
			if(!PINisOK){
				ResponseCode = RESPONSE_CODE_INCORRECT_PIN;
			}
			else
			{
				ResponseFlag = 0;
				reportFlag |= OUTPUT_REPORT_FLAG;
			}
			break;
		
		case PACKET_OPCODE_CHANGE_NAME:
			if(!PINisOK){
				ResponseCode = RESPONSE_CODE_INCORRECT_PIN;
			}
			else {			
				if(IKYBLEProtoRecv->length < sizeof(sysCfg.DeviceName)) {
					if(io_buzzer.enable != IO_TOGGLE_ENABLE)
					{
						IO_ToggleSetStatus(&io_buzzer,100,100,IO_TOGGLE_ENABLE,1);
					}
					memset(sysCfg.DeviceName,0x00,sizeof(sysCfg.DeviceName));
					memcpy(sysCfg.DeviceName,&IKYBLEProtoRecv->dataPt[4],IKYBLEProtoRecv->length - 4);					
					system_config_change = true;
					ResetMcuSet(MCU_RESET_AFTER_1_SEC);
				}
				else{
					ResponseCode = RESPONSE_CODE_SYSTEM_MALFUNCTION;
				}
			}
			break;
			
		case PACKET_OPCODE_CHANGE_PIN:			
			if(!PINisOK){
				ResponseCode = RESPONSE_CODE_INCORRECT_PIN;
				break;
			}
			if(IKYBLEProtoRecv->length != 12){
				ResponseCode = RESPONSE_CODE_SYSTEM_MALFUNCTION;
			}
			else {
				if(io_buzzer.enable != IO_TOGGLE_ENABLE)
				{
					IO_ToggleSetStatus(&io_buzzer,100,100,IO_TOGGLE_ENABLE,1);
				}
				memset(u8Tmp1,0,sizeof(u8Tmp1));
				IKY_Decrypt(u8Tmp1,u8Tmp2,&IKYBLEProtoRecv->dataPt[ENCRYPT_BLOCK_PIN_LENGTH]);
				memset(sysCfg.Password,0,sizeof(sysCfg.Password));
				memcpy(sysCfg.Password,u8Tmp1,DEVICE_PASSWORD_LENGTH);
				system_config_change = true;
			}
			break;
			
		case PACKET_OPCODE_CONTROL_OUTPUT:			
			if(!PINisOK){
				ResponseCode = RESPONSE_CODE_INCORRECT_PIN;
				break;
			}else{
				ResponseFlag = 0;
				IO_ToggleSetStatus(&io_buzzer,50,100,IO_TOGGLE_ENABLE,1);
				switch(IKYBLEProtoRecv->dataPt[ENCRYPT_BLOCK_PIN_LENGTH])
				{					
					case 1:
						if(IO_UpdateStatusOnRAM(&ioRelay_1_Status, IKYBLEProtoRecv->dataPt[ENCRYPT_BLOCK_PIN_LENGTH + 1]))
						{
							nrf_gpio_pins_set(OUTPUT_1_MASK);
						}
						else nrf_gpio_pins_clear(OUTPUT_1_MASK);
						break;
					case 2:						
						if(IO_UpdateStatusOnRAM(&ioRelay_2_Status, IKYBLEProtoRecv->dataPt[ENCRYPT_BLOCK_PIN_LENGTH + 1]))
						{
							nrf_gpio_pins_set(OUTPUT_2_MASK);
						}
						else nrf_gpio_pins_clear(OUTPUT_2_MASK);
						break;
					
					case 3:
						if(IO_UpdateStatusOnRAM(&ioRelay_3_Status, IKYBLEProtoRecv->dataPt[ENCRYPT_BLOCK_PIN_LENGTH + 1]))
						{
							nrf_gpio_pins_set(OUTPUT_3_MASK);
						}
						else nrf_gpio_pins_clear(OUTPUT_3_MASK);
						break;
					
					case 4:
						if(IO_UpdateStatusOnRAM(&ioRelay_4_Status, IKYBLEProtoRecv->dataPt[ENCRYPT_BLOCK_PIN_LENGTH + 1]))
						{
							nrf_gpio_pins_set(OUTPUT_4_MASK);
						}
						else nrf_gpio_pins_clear(OUTPUT_4_MASK);
						break;
						
					default:
						break;
				}
			}
			break;
			
		case PACKET_OPCODE_FWVER:						
			ResponseFlag = 0;
			//response
			IKYBLEProtoSend->start = 0xCA;
			IKYBLEProtoSend->opcode = PACKET_OPCODE_FWVER;
			IKYBLEProtoSend->length = strlen(FIRMWARE_VERSION);
			memcpy(IKYBLEProtoSend->dataPt,FIRMWARE_VERSION,IKYBLEProtoSend->length);			
			IKYBLEProtoSend->crc = IKY_CalcCheckSum(IKYBLEProtoSend->dataPt,IKYBLEProtoSend->length);	
			break;
		
		case PACKET_OPCODE_TOUCH_TYPE:
			break;
		
		default:
			ResponseFlag = 0;
			break;
	}
	if(ResponseFlag)
	{
		IKYBLEProtoSend->start = 0xCA;
		IKYBLEProtoSend->opcode = IKYBLEProtoRecv->opcode;
		IKYBLEProtoSend->length = 1;
		IKYBLEProtoSend->dataPt[0] = ResponseCode;
		IKYBLEProtoSend->crc = IKY_CalcCheckSum(IKYBLEProtoSend->dataPt,IKYBLEProtoSend->length);
	}
	IKYBLEProtoRecv->opcode = 0;
	return 0;
}

//added by haidv#date:20/02/2016	
uint8_t IKY_BLE_PutReport(IKY_BLE_PACKET_TYPE *TxReport,uint8_t opcode,uint8_t * p_data,uint8_t len)
{
	uint32_t u32Temp;	
	uint8_t* u8pt1 = TxReport->dataPt;
	uint8_t* u8pt2;
	//
	memcpy(u8pt1,sysCfg.Password,DEVICE_PASSWORD_LENGTH);
	memcpy(&u8pt1[DEVICE_PASSWORD_LENGTH],p_data,len);
	u32Temp = IKY_CalcPINBlock(u8pt1,DEVICE_PASSWORD_LENGTH + len);
	u8pt2 =  (uint8_t*)&u32Temp;
	u8pt1[0] = u8pt2[0];
	u8pt1[1] = u8pt2[1];
	u8pt1[2] = u8pt2[2];
	u8pt1[3] = u8pt2[3];			
	//
	TxReport->start = 0xCA;
	TxReport->opcode = opcode;
	TxReport->length = ENCRYPT_BLOCK_PIN_LENGTH + len;	
	TxReport->crc = IKY_CalcCheckSum(TxReport->dataPt,TxReport->length);
	return 0;
}


uint8_t IKY_BLE_PutData(RINGBUF *TxRingbuf,IKY_BLE_PACKET_TYPE * TxData)
{
	uint8_t i = 0;
	uint8_t length = 0;
	if(TxData->length)
	{
		RINGBUF_Put(TxRingbuf,TxData->start);
		RINGBUF_Put(TxRingbuf,TxData->length);
		RINGBUF_Put(TxRingbuf,TxData->opcode);		
		for(i = 0;i < TxData->length;i++)
		{
			RINGBUF_Put(TxRingbuf,TxData->dataPt[i]);			
		}
		RINGBUF_Put(TxRingbuf,TxData->crc);		
		TxData->length = 0;	
		return length;	
	}
	return 0;
}




