#ifndef __IKY_BLE_APP_H__
#define __IKY_BLE_APP_H__

#include <stdint.h>
#include "lib\ringbuf.h"
#include "lib\sys_tick.h"

#define PACKET_OPCODE_LOGIN						0x01
#define PACKET_OPCODE_OUTPUT_STATUS		0x02
#define PACKET_OPCODE_CONTROL_OUTPUT	0x03
#define PACKET_OPCODE_CHANGE_PIN			0x04
#define PACKET_OPCODE_CHANGE_NAME			0x05
#define PACKET_OPCODE_FWVER						0x06

#define PACKET_OPCODE_TOUCH_TYPE			0x10

#define ENCRYPT_BLOCK_PIN_LENGTH					4

#define RESPONSE_CODE_APPROVED						0x00
#define RESPONSE_CODE_INCORRECT_PIN				0x55
#define RESPONSE_CODESECURITY_VIOLATION		0x63
#define RESPONSE_CODE_SYSTEM_MALFUNCTION	0x96


#define OUTPUT_REPORT_FLAG								0x01
#define OUTPUT_TOUCH_CONTROL_FLAG					0x02

typedef struct __attribute__((packed)){
	uint8_t start;
	uint8_t opcode;
	uint8_t length;
	uint8_t *dataPt;
	uint8_t crc;
}IKY_BLE_PACKET_TYPE;

typedef enum{
	IKY_BLE_CMD_NEW_STATE,
	IKY_BLE_CMD_GET_LENGTH,
	IKY_BLE_CMD_GET_OPCODE,
	IKY_BLE_CMD_GET_DATA,
	IKY_BLE_CMD_CRC_CHECK,
	IKY_BLE_CMD_WAITING_SATRT_CODE
}IKY_BLE_CMD_STATE_TYPE;

typedef struct
{
	IKY_BLE_CMD_STATE_TYPE state;
	uint8_t len;
	uint8_t lenMax;
	uint8_t cnt;
	uint8_t opcode;
	uint8_t crc;
}IKY_BLE_PARSER_PACKET_TYPE;

typedef enum{
	RESET_PIN_IDLE,
	RESET_PIN_STEP1,
	RESET_PIN_STEP2,
}RESET_PIN_STATE_TYPE;

extern RINGBUF IKYBLE_TxRingBuf;
extern RINGBUF IKYBLE_RxRingBuf;
extern IKY_BLE_PACKET_TYPE IKYBLE__ProtoReport;
extern uint32_t reportFlag;
extern RESET_PIN_STATE_TYPE rsPINState;

void IKY_BLE_Rx_Callback(uint8_t c);
void IKY_BLE_TaskInit(void);
void IKY_BLE_Task(void);
uint8_t IKY_BLE_ParserPacket(IKY_BLE_PARSER_PACKET_TYPE *parserPacket,IKY_BLE_PACKET_TYPE *IKYBLEProtoRecv,uint8_t c);
uint8_t IKY_BLE_ProcessData(IKY_BLE_PACKET_TYPE *IKYBLEProtoRecv,IKY_BLE_PACKET_TYPE *IKYBLEProtoSend,uint32_t maxPacketSize);
uint8_t IKY_BLE_PutData(RINGBUF *TxRingbuf,IKY_BLE_PACKET_TYPE * TxData);


#endif //__IKY_BLE_APP_H__

