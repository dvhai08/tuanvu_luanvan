/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : d3des.h
* Author             : MCD Application Team
* Version            : V1.0.0
* Date               : 10/06/2008
* Description        : d3des header file
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
//#include "stm32f10x.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __D3DES_H
#define __D3DES_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* DES constants */
/* Number of bytes to store a DES block */
#define DES_BLOCK_SIZE  8
/* Number of bytes to store a DES key */
#define DES_KEY_SIZE    8

/* DES constants */
/* Number of bytes to store a Triple DES block */
#define TDES_BLOCK_SIZE 8
/* Number of bytes to store a Triple DES key */
#define TDES_KEY_SIZE   8

/* Exported macro ------------------------------------------------------------*/
/* DES decryption is as DES encryption. */
#define DES_decrypt DES_encrypt
/* Triple DES decryption is as Triple DES encryption. */
#define TDES_decrypt TDES_encrypt 

typedef  uint8_t u8;
typedef  uint16_t u16;
typedef  uint32_t u32;
/* Exported functions ------------------------------------------------------- */
void DES_keyschedule_enc(u8 *key);
void DES_keyschedule_dec(u8 *key); 
void DES_encrypt(u8 *inblock, u8 *outblock);
void TDES_keyschedule_enc(u8 *key);
void TDES_keyschedule_dec(u8 *key);
void TDES_encrypt(u8 *inblock, u8 *outblock);

#endif /* __D3DES_H */

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
