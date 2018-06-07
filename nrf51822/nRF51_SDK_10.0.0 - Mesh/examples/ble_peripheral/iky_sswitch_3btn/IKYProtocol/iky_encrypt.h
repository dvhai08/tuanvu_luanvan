/* 
 * File:   iky_encrypt.h
 * Author: HaiDV
 *
 * Created on July 30, 2014, 2:41 PM
 */

#ifndef IKY_ENCRYPT_H
#define	IKY_ENCRYPT_H
#include <stdint.h>
#include <string.h>
#ifdef	__cplusplus
extern "C" {
#endif
void HashMD5(uint8_t *dst,uint8_t *src,uint16_t srcLen);
void IKY_CombineKey(uint8_t* Dst,uint8_t* MasterKey,uint8_t* SSkey,uint8_t* PAN);
void IKY_Encrypt(uint8_t* Dst, uint8_t* MaterKey, uint8_t* SSKey, uint8_t* PAN, uint8_t* planMsg);
void IKY_Decrypt(uint8_t* Dst, uint8_t* MaterKey, uint8_t* planMsg);
uint8_t IKY_CalcCheckSum(uint8_t *buff, uint16_t length);
uint32_t IKY_CalcPINBlock(uint8_t *buff, uint16_t length);
#ifdef	__cplusplus
}
#endif

#endif	/* IKY_ENCRYPT_H */

