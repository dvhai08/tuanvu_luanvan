
/*******************************************************************************
* Included headers
*******************************************************************************/
#include "CY8CMBR3xxx_HostFunctions.h"
#include "nrf_delay.h"
#include "app_twi.h"
#include "nrf_log.h"
#include "i2c_driver.h"
/*******************************************************************************
* API Constants
*******************************************************************************/
#define CY8CMBR3xxx_READ                    (1)
#define CY8CMBR3xxx_WRITE                   (0)
#define CY8CMBR3xxx_ACK                     (0)
#define CY8CMBR3xxx_NACK                    (1)
#define CY8CMBR3xxx_READ_BYTE_ERROR         (0x80000000)

/* The following macro defines the maximum number of times the low-level read
 * and write functions try to communicate with the CY8CMBR3xxx device, as
 * long as the I2C communication is unsuccessful.
 */
#define CY8CMBR3xxx_RETRY_TIMES             (10)  
#define CY8CMBR3xxx_TIMEOUT             		(100000)
#define CY8CMBR3xxx_ADDRESS             		(0x37)

//bool bHostTXFlag = false;
//bool bHostRXFlag = false;
//nrf_drv_twi_t m_twi_CY8CMBRxxxx = NRF_DRV_TWI_INSTANCE(0);
//extern app_twi_t m_app_twi;

//uint8_t* hostTXBuffer = NULL;
//uint8_t hostTXLength;
//uint8_t* hostRXBuffer = NULL;
//uint8_t hostRXLength;

uint8_t hostRetryCount = CY8CMBR3xxx_RETRY_TIMES;
/*******************************************************************************
*   Function Code
*******************************************************************************/
/**
 * @brief TWI events handler.
 */
void Host_TWI_Handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{      
//    switch(p_event->type)
//    {
//        case NRF_DRV_TWI_RX_DONE:	
//						bHostRXFlag = true;					
//						hostRXBuffer = NULL;						
//						hostRetryCount = CY8CMBR3xxx_RETRY_TIMES;
//            break;
//        case NRF_DRV_TWI_TX_DONE:
//            bHostTXFlag = true;
//						hostTXBuffer = NULL;
//						hostRetryCount = CY8CMBR3xxx_RETRY_TIMES;
//            break;
//        default:
//						switch(p_event->error_src)
//						{
//							case NRF_TWI_ERROR_ADDRESS_NACK:
//							case NRF_TWI_ERROR_DATA_NACK:
//								NRF_LOG_PRINTF("TWI ERROR\r\n");
//								if(hostRetryCount) hostRetryCount--;
//								else break;
//								if(hostTXBuffer)
//								{
//									nrf_drv_twi_tx(&m_twi_CY8CMBRxxxx, CY8CMBR3xxx_ADDRESS, hostTXBuffer, hostTXLength, true);
//								}
//								else if(hostRXBuffer)
//								{
//									nrf_drv_twi_rx(&m_twi_CY8CMBRxxxx, CY8CMBR3xxx_ADDRESS, hostRXBuffer, hostRXLength, false);
//								}
//								break;
//							default: break;
//						}
//            break;        
//    }   
}
/*******************************************************************************
* Function Name: Host_LowLevelWrite
********************************************************************************
*
* Summary:
*  This API writes to the register map of the CY8CMBR3xxx device using the I2C 
*  communication protocol. The implementation is host processor dependent and 
*  you may need to update the API code to suit your host.
*
* Parameters:
*  uint8 slaveAddress:
*   The I2C address of the CY8CMBR3xxx device. Valid range: 8 - 119
*
*  uint8 *writeBuffer:
*   The buffer from which data is written to the device. 
*
*   The first element should always contain the location of the register 
*   of the device to write to. This value can be within 0 – 251.
*
*   Each successive element should contain the data to be written to that 
*   register and the successive registers. These elements can have a value 
*   between 0 – 255. The number of data bytes can be between 0 and 128.
*
*  uint8 numberOfBytes:
*   Number of bytes to be written, equal to the number of elements in the 
*   buffer (i.e. number of data bytes + 1)
*
* Pre:
*  The I2C interface should be enabled and working before this Low Level 
*  API can be called. Also make sure that the Global Interrupts are also
*  enabled (if required)
*
* Post:
*  N/A
*
* Return:
*  status
*    Value                Description
*    TRUE                 Write process was successful
*    FALSE                Write process was not successful
*
*******************************************************************************/
bool Host_LowLevelWrite(uint8 slaveAddress, uint8 *writeBuffer, uint8 numberOfBytes)
{
    bool status = FALSE;                                       /* Default return is FALSE if anything goes wrong */    
    ret_code_t err_code;
//		uint32_t timeout = CY8CMBR3xxx_TIMEOUT;
	
		#if 1
		err_code = I2C___BlockWrite(slaveAddress, writeBuffer, numberOfBytes);
	
    
    /* Check whether the transaction was successful or it timed-out */
    status = (err_code == I2C_error_None)? TRUE: FALSE;
			
		#else
	
		hostTXBuffer = writeBuffer;
		hostTXLength = numberOfBytes;
		err_code = nrf_drv_twi_tx(&m_twi_CY8CMBRxxxx, slaveAddress, writeBuffer, numberOfBytes, false);
		
		while((bHostTXFlag == false) && (--timeout))
		{
			nrf_delay_us(1);
		}
						
		status = (bHostTXFlag)? TRUE: FALSE;
		bHostTXFlag = false;
		
		#endif
    /* Return the status */
		if(status == FALSE)NRF_LOG_PRINTF("TX TWI ERROR %d\r\n", err_code);
    return status;
}

/*******************************************************************************
* Function Name: Host_LowLevelRead
********************************************************************************
*
* Summary:
*  This API reads from the register map of the CY8CMBR3xxx device using the 
*  I2C communication protocol. The implementation is host processor dependent 
*  and you may need to update the API code to suit your host.
*
* Parameters:
*  uint8 slaveAddress:
*   The I2C address of the CY8CMBR3xxx device. Valid range: 8 - 119
*
*  uint8 *readBuffer:
*   The buffer to be updated with the data read from the device.
*
*   The register location to read from should be set prior to calling 
*   this API. Each successive element to contain the data read from that 
*   register and the successive registers. These elements can have a value 
*   between 0 – 255.
*
*  uint8 numberOfBytes:
*   Number of data bytes to be read, equal to the number of elements in 
*   the buffer. Valid range: 1 – 252
*
* Pre:
*  The I2C interface should be enabled and working before this Low Level 
*  API can be called. Also make sure that the Global Interrupts are also
*  enabled (if required)
*
* Post:
*  N/A
*
* Return:
*  status
*    Value                Description
*    TRUE                 Read process was successful
*    FALSE                Read process was not successful
*
*******************************************************************************/
bool Host_LowLevelRead(uint8 slaveAddress, uint8 *readBuffer, uint8 numberOfBytes)
{
    bool status = FALSE;                                       /* Default return is FALSE if anything goes wrong */
    ret_code_t err_code;
//		uint32_t timeout = CY8CMBR3xxx_TIMEOUT;
		
		#if 1
		err_code = I2C___BlockRead(slaveAddress, readBuffer, numberOfBytes);
    /* Check whether the transaction was successful or it timed-out */
    status = (err_code == I2C_error_None)? TRUE: FALSE;
    		
		#else
		
		hostRXBuffer = readBuffer;
		hostRXLength = numberOfBytes;		
		err_code = nrf_drv_twi_rx(&m_twi_CY8CMBRxxxx, slaveAddress, readBuffer, numberOfBytes, false);
	
		while((bHostRXFlag == false) && (--timeout))
		{
			nrf_delay_us(1);
		}
				
		status = (bHostRXFlag)? TRUE: FALSE;
		bHostRXFlag = false;
		
		#endif
    /* Return the status */
		if(status == FALSE)NRF_LOG_PRINTF("RX TWI ERROR %d\r\n",err_code);
    return status;
}

/*******************************************************************************
* Function Name: Host_LowLevelDelay
********************************************************************************
*
* Summary:
*  This API implements a time-delay function to be used by the High-level APIs. 
*  The delay period is in milliseconds. This delay is achieved by a 
*  code-execution block for the required amount of time.
*
*  The implementation is host processor dependent and you need to update the 
*  API code to suit your host.
*
* Parameters:
*  uint16 milliseconds:
*   The amount of time in milliseconds for which a wait is required. 
*   Valid range: 0 – 65535
*
* Return:
*  None
*
*******************************************************************************/
void Host_LowLevelDelay(uint16 milliseconds)
{
    // Call the host-specific delay implementation
    // Replace this with the correct host delay routine for introducing delays in milliseconds
    nrf_delay_ms((uint32) milliseconds);
}

/****************************End of File***************************************/
