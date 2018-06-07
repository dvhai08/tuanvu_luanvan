
#ifndef _I2C_DRIVER_H
#define _I2C_DRIVER_H

#include <stdint.h>
#include "hw_config.h"

#define SDA_PIN		CY8CMBR3xxx_SDA_PIN
#define SCL_PIN		CY8CMBR3xxx_SCL_PIN
    
//================================================================
// DO NOT EDIT BELOW THIS LINE         DO NOT EDIT BELOW THIS LINE
//================================================================
#define I2C_error_None                  0
#define I2C_error_NACK                  1
#define I2C_error_BusCollision          2
#define I2C_error_NoResponse            3
#define I2C_error_WriteCollision        4
#define I2C_error_ReadTimeout           5
#define I2C_error_MaxLengthExceeded     6  
    
//-------------------------------
// Global Function Prototypes
//-------------------------------
extern void    I2C_init     (void);
extern uint8_t I2C_read     (uint8_t* buffer,  uint8_t length);
extern uint8_t I2C_write    (uint8_t  out);
extern void    I2C_nack     (void);
extern void    I2C_ack      (void);
extern void    I2C_start    (void);
extern void    I2C_restart  (void);
extern void    I2C_stop     (void);
extern void    I2C_idle     (void);
		
extern uint8_t I2C_BlockRead(uint8_t  dataAddress, uint8_t* data, uint8_t  length);
extern uint8_t I2C_BlockWrite(uint8_t  dataAddress,uint8_t* data, uint8_t  length);
		
extern uint8_t I2C___BlockRead(uint8_t  dataAddress, uint8_t* data, uint8_t  length);
extern uint8_t I2C___BlockWrite(uint8_t  dataAddress,uint8_t* data, uint8_t  length);
    
#endif
