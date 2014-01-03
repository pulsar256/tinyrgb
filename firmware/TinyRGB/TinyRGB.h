/*
* TinyRGB.h
*
* Created: 03.01.2014 02:34:44
* Author: Paul Rogalinski, paul@paul.vc
*/


#ifndef TINYRGB_H_
#define TINYRGB_H_

#ifndef F_CPU
#define F_CPU 20000000UL     /* Set your clock speed here, mine is a 20 Mhz quartz*/
#endif

#define UART_BAUD_RATE 9600UL
// @see 14.3.1 Internal Clock Generation – The Baud Rate Generator
#define UART_BAUD_CALC(UART_BAUD_RATE,F_CPU) ((F_CPU)/((UART_BAUD_RATE)*16l)-1)

#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

#define SLED_PORT				PORTD
#define SLED_DDR				DDRD
#define SLED_PIN				PIND6
#define REG_RED					OCR1B
#define REG_GRN					OCR1A
#define REG_BLU					OCR0A
#define REG_WHI					OCR0B
#define MODE_FADE_RANDOM_RGB    1
#define MODE_FIXED				2
#define MODE_FADE_HSV			3
#define FADE_WAIT				100
#define LOCAL_ECHO				1
//#define ENABLE_BLINK_CONFIRM
#define ENABLE_WHITECHANNEL
#define	EEPStart 0x10


int parseNextInt(char** buffer);
void updateEEProm();
void restoreFromEEProm();
void serialBufferHandler(char* commandBuffer);

#endif /* TINYRGB_H_ */