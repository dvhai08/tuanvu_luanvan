#ifndef HW_CONFIG_H__
#define HW_CONFIG_H__

#include <stdint.h>

#ifdef HW_1_0_0

#define LED_ON				1
#define LED_OFF				0

#define APP_GPIOTE_MAX_USERS				2
#define SLAVE_ADDRESS								0x37

#define LED_RED_1				12
#define LED_RED_2				14
#define LED_RED_3				0
#define LED_RED_4				29

#define LED_GREEN_1			11
#define LED_GREEN_2			13
#define LED_GREEN_3			1
#define LED_GREEN_4			28

#define	RELAY_1_PIN			6
#define	RELAY_2_PIN			7
#define	RELAY_3_PIN			8
#define	RELAY_4_PIN			9

#define	OUTPUT_1_MASK		(1<<RELAY_1_PIN) | (1<<LED_GREEN_1)
#define	OUTPUT_2_MASK		(1<<RELAY_2_PIN) | (1<<LED_GREEN_2)
#define	OUTPUT_3_MASK		(1<<RELAY_3_PIN) | (1<<LED_GREEN_3)
#define	OUTPUT_4_MASK		(1<<RELAY_4_PIN) | (1<<LED_GREEN_4)

#define	OUTPUT_1_STATE	((NRF_GPIO->OUT >> RELAY_1_PIN) & 1UL)
#define	OUTPUT_2_STATE	((NRF_GPIO->OUT >> RELAY_2_PIN) & 1UL)
#define	OUTPUT_3_STATE	((NRF_GPIO->OUT >> RELAY_3_PIN) & 1UL)
#define	OUTPUT_4_STATE	((NRF_GPIO->OUT >> RELAY_4_PIN) & 1UL)


#define CY8CMBR3xxx_INT_PIN	2
#define CY8CMBR3xxx_SCL_PIN	3
#define CY8CMBR3xxx_SDA_PIN	4

#else

#define LED_ON				1
#define LED_OFF				0

#define APP_GPIOTE_MAX_USERS				2
#define SLAVE_ADDRESS								0x37

#define LED_GREEN_1				14
#define LED_GREEN_2				24
#define LED_GREEN_3				23
#define LED_GREEN_4				12

#define LED_BLUE_1				22

#define	BUZZER_PIN				28

#define	RELAY_1_PIN				7
#define	RELAY_2_PIN				3
#define	RELAY_3_PIN				8
#define	RELAY_4_PIN				9

#define	OUTPUT_1_MASK			(1<<RELAY_1_PIN) | (1<<LED_GREEN_1)
#define	OUTPUT_2_MASK			(1<<RELAY_2_PIN) | (1<<LED_GREEN_2)
#define	OUTPUT_3_MASK			(1<<RELAY_3_PIN) | (1<<LED_GREEN_3)
#define	OUTPUT_4_MASK			(1<<RELAY_4_PIN) | (1<<LED_GREEN_4)

#define	OUTPUT_1_STATE		((NRF_GPIO->OUT >> RELAY_1_PIN) & 1UL)
#define	OUTPUT_2_STATE		((NRF_GPIO->OUT >> RELAY_2_PIN) & 1UL)
#define	OUTPUT_3_STATE		((NRF_GPIO->OUT >> RELAY_3_PIN) & 1UL)
#define	OUTPUT_4_STATE		((NRF_GPIO->OUT >> RELAY_4_PIN) & 1UL)


#define CY8CMBR3xxx_INT_PIN	4
#define CY8CMBR3xxx_SCL_PIN	5
#define CY8CMBR3xxx_SDA_PIN	6

#ifdef HW3TOUCH
#define	TOUCH_CHANNEL_1_MASK		0x04
#define	TOUCH_CHANNEL_2_MASK		0x08
#define	TOUCH_CHANNEL_3_MASK		0x02
#else
#define	TOUCH_CHANNEL_1_MASK		0x08
#define	TOUCH_CHANNEL_2_MASK		0x02
#define	TOUCH_CHANNEL_3_MASK		0x04
#define	TOUCH_CHANNEL_4_MASK		0x80
#endif

#define	PROXIMITY_CHANNEL_1_MASK		0x01

#endif 

#endif // HW_CONFIG_H__

