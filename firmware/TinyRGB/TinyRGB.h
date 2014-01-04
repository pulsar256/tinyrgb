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


#define SLED_PORT               PORTD
#define SLED_DDR                DDRD
#define SLED_PIN                PIND6
#define REG_RED                 OCR1B
#define REG_GRN                 OCR1A
#define REG_BLU                 OCR0A
#define REG_WHI                 OCR0B
#define MODE_FADE_RANDOM_RGB    1
#define MODE_FIXED              2
#define MODE_FADE_HSV           3
#define LOCAL_ECHO              1
//#define ENABLE_BLINK_CONFIRM
#define ENABLE_WHITECHANNEL
#define	EEPStart                0x10


int parseNextInt(char** buffer);
void updateEEProm();
void restoreFromEEProm();
void serialBufferHandler(char* commandBuffer);

#endif /* TINYRGB_H_ */