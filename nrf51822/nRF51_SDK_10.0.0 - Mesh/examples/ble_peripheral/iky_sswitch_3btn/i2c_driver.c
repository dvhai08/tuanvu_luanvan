
#include "i2c_driver.h"
#include "nrf_gpio.h"

#define CY8CMBR3xxx_MAX_BUFFER_LENGTH		(8)
#define CY8CMBR3xxx_ADDRESS							(0x37)
#define I2C_RETRY_TIMES									(10)
//-------------------------------
// Function Prototypes
//-------------------------------
void    I2C_init        (void);
uint8_t I2C_read        (uint8_t* buffer,  uint8_t length);
uint8_t I2C_write       (uint8_t  out);
void    I2C_nack        (void);
void    I2C_ack         (void);
void    I2C_start       (void);
void    I2C_restart     (void);
void    I2C_stop        (void);
void    I2C_idle        (void);


#define SDA_InitInput()			nrf_gpio_cfg_input(SDA_PIN,NRF_GPIO_PIN_PULLUP)
#define SDA_ReadInput()			nrf_gpio_pin_read(SDA_PIN)

#define SDA_InitOutput()		nrf_gpio_cfg_output(SDA_PIN)
#define SDA_Low()						nrf_gpio_pin_clear(SDA_PIN)	
#define SDA_High()					nrf_gpio_pin_set(SDA_PIN)

#define SCL_InitOutput()		nrf_gpio_cfg_output(SCL_PIN)
#define SCL_Low()						nrf_gpio_pin_clear(SCL_PIN)	
#define SCL_High()					nrf_gpio_pin_set(SCL_PIN)

//-------------------------------
// Global Function Prototypes
//-------------------------------
uint8_t I2C_BlockRead (uint8_t, uint8_t*, uint8_t);
uint8_t I2C_BlockWrite(uint8_t, uint8_t*, uint8_t);


void I2CDelay(unsigned int count)
{
	while (count--);
}
/*******************************************************************************
  Summary:
    Reads 'length' bytes at the data 'dataAddress' from the CY8CMBR3xxx at I2C
    address 'deviceAddress' and stores the values in the 'data' array.
      
*******************************************************************************/
uint8_t I2C_BlockRead(uint8_t  dataAddress, uint8_t* data, uint8_t  length)
{
    uint8_t error;
		uint8_t retryCount = I2C_RETRY_TIMES;
    //-----------------------------------------
    // START and Device Address (+ WRITE)
    //-----------------------------------------
		retryCount = I2C_RETRY_TIMES;
		for(;;)
		{
			I2C_start();
			error = I2C_write((uint8_t)(CY8CMBR3xxx_ADDRESS << 1));    
			if((!error) || ((0 == (--retryCount)))) break;
		}	
		I2C_write(dataAddress);		
		I2C_stop();
    //-----------------------------------------
    // RESTART and Device Address (+ READ)
    //-----------------------------------------
    I2C_start();
    retryCount = I2C_RETRY_TIMES;
		for(;;)
		{
			I2C_start();
			error = I2C_write((uint8_t)(CY8CMBR3xxx_ADDRESS << 1) + 1);
			if((!error) || ((0 == (--retryCount)))) break;
		}    		
		//-----------------------------------------
    // Read Data and STOP
    //-----------------------------------------			
    I2C_read(data, length);
    I2C_nack();
    I2C_stop();
       
    return I2C_error_None;
}

uint8_t I2C___BlockRead(uint8_t  dataAddress, uint8_t* data, uint8_t  length)
{
    uint8_t error;
		uint8_t retryCount = I2C_RETRY_TIMES;    
    //-----------------------------------------
    // START and Device Address (+ READ)
    //-----------------------------------------
    I2C_start();
    retryCount = I2C_RETRY_TIMES;
		for(;;)
		{
			I2C_start();
			error = I2C_write((uint8_t)(CY8CMBR3xxx_ADDRESS << 1) + 1);
			if((!error) || ((0 == (--retryCount)))) break;
		}    		
		if(error) return I2C_error_NACK;
		//-----------------------------------------
    // Read Data and STOP
    //-----------------------------------------			
    I2C_read(data, length);
    I2C_nack();
    I2C_stop();
       
    return I2C_error_None;
}
/*******************************************************************************
  Summary:
    Writes 'length' bytes from the 'data' array to the CY8CMBR3xxx data location 
    'dataAddress' at I2C address 'deviceAddress'.

*******************************************************************************/
uint8_t I2C_BlockWrite(uint8_t  dataAddress,uint8_t* data, uint8_t  length)
{
    uint8_t error;
    uint8_t retryCount = I2C_RETRY_TIMES;
    if(CY8CMBR3xxx_MAX_BUFFER_LENGTH < length)
		{
			return I2C_error_MaxLengthExceeded;
		}
    //-----------------------------------------
    // START and Device Address
    //-----------------------------------------
		for(;;)
		{
			I2C_start();

			error = I2C_write((uint8_t)(CY8CMBR3xxx_ADDRESS << 1));
			if ((!error) || ((0 == (--retryCount)))) break;
    }
    //-----------------------------------------
    // Data Address
    //-----------------------------------------
    error = I2C_write(dataAddress);

    //-----------------------------------------
    // Write Data
    //-----------------------------------------
    for (uint8_t i = 0; i < length; i++)
    {
        error = I2C_write(data[i]);
    }    
    I2C_stop();
    return I2C_error_None;
}

uint8_t I2C___BlockWrite(uint8_t  slaveAddress,uint8_t* data, uint8_t  length)
{
    uint8_t error;
		uint8_t i;
    uint8_t retryCount = I2C_RETRY_TIMES;
    //-----------------------------------------
    // START and Device Address
    //-----------------------------------------
		for(;;)
		{
			I2C_start();   

			error = I2C_write((uint8_t)(slaveAddress << 1));
			if ((!error) || ((0 == (--retryCount)))) break;
    }
		if(error) return I2C_error_NACK;
    //-----------------------------------------
    // Write Data
    //-----------------------------------------
    for (i = 0; i < length; i++)
    {
        error = I2C_write(data[i]);
    }    
    I2C_stop();
    return I2C_error_None;
}
/*******************************************************************************
  Function:
    void I2C_init(void)

  Summary:
    Initializes the I2C module for communications with the MTCH112.
    
  Description:
    Initialization of SCL and SDA pins are performed using the #define labels
    located in the mtch112_driver.h header file. The I2C module is configured
    for master-mode I2C.
    No interrupts are enabled.
    
  Precondition:
    mtch112_driver.h must be configured to use the correct labels for the 
    specific SSP module and PIC microcontroller.
*******************************************************************************/
void I2C_init(void)
{
	SCL_InitOutput();
}

/*******************************************************************************
  Function:
    uint8_t I2C_write(uint8_t value)

  Summary:
    Sends the 'value' to the slave device.
    
  Description:
    Writes the provided 'value' to the SSPBUF. Checks for write collisions and
    immediately STOPs if it occurs. Waits for the buffer to complete the send.

  Precondition:
    mtch112_driver.h must be configured to use the correct labels for the 
    specific SSP module and PIC microcontroller.
    
  Returns:
    I2C_error_None - if no error occurred during the write operation. ACK.
    I2C_error_NACK - if the slave device NACK'd the written byte.
    I2C_error_WriteCollision - if a write collision occurred.
*******************************************************************************/
uint8_t I2C_write(uint8_t value)
{
	uint8_t i;
	uint8_t mask;
	SDA_InitOutput();
	mask = 0x80;                      
	for (i=0; i<8; i++)					/* send one byte of data */
	{
			if (mask & value) 			/* send bit according to data */
	      SDA_High();
			else SDA_Low();
	
			mask = mask >> 1;				/* shift right for the next bit */
			I2CDelay(0);
			SCL_High();							/* clock is high */
			I2CDelay (1);
			SCL_Low();							/* clock is low */
	}
	SDA_InitInput();
	mask = SDA_ReadInput();			/* read acknowledge */
	I2CDelay(3);
	SCL_High();									/* generate 9th clock pulse */
	I2CDelay(1);
	SCL_Low();									/* clock is low */	
	I2CDelay(6);								/* to avoid short pulse transition on SDA line */	
	return (mask);							/* return acknowledge bit */	
}

/******************************************************************************/
/*	Subroutine:	I2CMasterRead			 		      */
/*			                                 		      */
/*	Description:	Read one byte of data from the slave device. Check    */
/*			for WAIT condition before every bit is received.      */	
/*                                                                            */
/*      Input:	     	Acknowledge require:				      */
/*			0 - generate LOW output after a byte is received      */
/*			1 - generate HIGH output after a byte is received     */
/*                                                                            */
/*      Return:  	received one byte of data from slave device	      */
/*		        						      */                       
/*                                                                    	      */
/******************************************************************************/
unsigned char I2CMasterRead(unsigned char ack)
{
	unsigned char i;
	unsigned int mask, rec_data;    
	SDA_InitInput();
	rec_data = 0;	
	mask = 0x80;	
	for (i=0; i<8; i++)
	{
			if (SDA_ReadInput())                    
	    	rec_data |= mask;

	    mask = mask >> 1;   
			SCL_High();       		/* clock is high */
	    I2CDelay(2);      
	    SCL_Low();                    /* clock is low */                                                                       
	} 
	SDA_InitOutput();
	if (ack)               		/* set SDA data first before port direction */	
	   SDA_High();             	/* send acknowledge */
	else SDA_Low();
       
	I2CDelay(3);
 	SCL_High();    	  			/* clock is high */
	I2CDelay(1);
	SCL_Low();							/* clock is low */
	SDA_High();
	I2CDelay(6);						/* to avoid short pulse transition on SDA line */
	return (rec_data);
}
/*******************************************************************************
  Function:
    uint8_t I2C_read(uint8_t* input, uint8_t length)

  Summary:
    Reads 'length' values from the slave and stores them in 'input'.
    
  Description:
    For each byte to be received: the I2C module is set to enable reception of
    a byte, the value is stored in the input buffer, bus collisions are checked,
    and an ACK is sent. A NACK is sent after the last byte has been received.

  Precondition:
    mtch112_driver.h must be configured to use the correct labels for the 
    specific SSP module and PIC microcontroller.
    
  Returns:
    I2C_error_None - if no error occurred during the read operation.
    I2C_error_BusCollision - if a bus collision occurred.
    I2C_error_ReadTimeout - if the system timed out while waiting for the data
*******************************************************************************/
uint8_t I2C_read(uint8_t* input, uint8_t length)
{
		uint8_t i = 0;
    for(;;)
    {
        if (i < (length-1)) input[i] = I2CMasterRead(0);          // ACK each byte until finished receiving
        else        input[i] = I2CMasterRead(1);         // NACK after final byte
				i++;
				if(i > (length-1)) break;
    }
    return I2C_error_None;              // last byte received so don't send ACK
}


/*******************************************************************************
  Function:
    void I2C_nack(void)

  Summary:
    Send a NACK sequence
*******************************************************************************/
void I2C_nack(void)
{
	SDA_InitOutput();
	SDA_Low();
       
	I2CDelay(3);
 	SCL_High();    	  		/* clock is high */
	I2CDelay(1);
	SCL_Low();                	/* clock is low */
	SDA_High();
	I2CDelay(6);			/* to avoid short pulse transition on SDA line */
}

/*******************************************************************************
  Function:
    void I2C_ack(void)

  Summary:
    Send an ACK sequence
*******************************************************************************/
void I2C_ack(void)
{
	SDA_InitOutput();	
	SDA_High();             	/* send acknowledge */
       
	I2CDelay(3);
 	SCL_High();    	  		/* clock is high */
	I2CDelay(1);
	SCL_Low();                	/* clock is low */
	SDA_High();
	I2CDelay(6);			/* to avoid short pulse transition on SDA line */
}

/*******************************************************************************
  Function:
    void I2C_start(void)

  Summary:
    Send a START condition
*******************************************************************************/
void I2C_start(void)
{
	SDA_InitOutput();
	SDA_High();        /* to make sure the SDA and SCL are both high */
	SCL_High();
	I2CDelay(5);       /* add delay */

	SDA_Low();        /* SDA line go LOW first */
	I2CDelay(10);
	SCL_Low();        /* then followed by SCL line with time delay */
}

/*******************************************************************************
  Function:
    void I2C_restart(void)

  Summary:
    Send a RESTART condition
*******************************************************************************/
void I2C_restart(void)
{
//    I2C_LABEL_RSEN = 1;      // Send RESTART condition
//    while(I2C_LABEL_RSEN);   // Wait for RESTART condition to execute
}

/*******************************************************************************
  Function:
    void I2C_stop(void)

  Summary:
    Send a STOP condition
*******************************************************************************/
void I2C_stop(void)
{
	SDA_InitOutput();
	SDA_Low();
	SCL_High();
	I2CDelay(10);		
	SDA_High();
}

/*******************************************************************************
  Function:
    void I2C_idle(void)

  Summary:
    Block until the I2C hardware module has completed its current task.
*******************************************************************************/
void I2C_idle(void)
{
//    while ((I2C_BUSY_FLAGS) || (I2C_LABEL_RnW));
}
