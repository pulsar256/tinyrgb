/*
 * TinyRGB.c
 *
 * Created: 11.09.2013 20:10:35
 *  Author: Paul Rogalinski, paul@paul.vc
 *
 * Pins: 
 *  2: RX  -> BT TX
 *  3: TX  -> BT RX
 * 11: PD6 -> Status LED
 * 14: PB2 / OC0A -> Blue
 * 15: PB3 / OC1A -> Green
 * 16: PB4 / OC1B -> RED
 
 todo: hack: 4th white channel to be controlled by "set value" commands but not via hsv / effect model.
 todo: protocol
 todo: save last state in eeprom, reload on boot.  
 \o/
 */ 

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
#define LOCAL_ECHO				1;

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#include "random.h"
#include "pwm.h"
#include "usart.h"
#include "colors.h"

int parseNextInt(char** buffer);


const char* welcome;

// RgbColor is unsigned (char), so we are use ints here.
int     offsetRed = 0, offsetGreen = 0, offsetBlue = 0; // color offsets, can be negative. rgb output will always be clipped to 0..255
HsvColor	hsv; // current hsv values (in MODE_FADE_HSV)
RgbColor 	rgb; // current rgb values (in MODE_FADE_RANDOM_RGB)
RgbColor 	tRgb; // target rgb values (for MODE_FADE_RANDOM_RGB)
RgbColor	mRgb; // maximum / clipping values

uint8_t mode = MODE_FADE_RANDOM_RGB;
uint8_t wait = FADE_WAIT;
uint8_t currentWait = FADE_WAIT;

void serialBufferHandler(void)
{
	cli();
	setRgb(0,0,0);
	_delay_ms(10);
	setRgb(0,255,0);	
	_delay_ms(100);
	setRgb(0,0,0);
	_delay_ms(10);
	sei();
	
	
	char *bufferCursor;
	
		
	// "_______________\0"
	// "SRGB:RRRGGGBBB\0" -> set r / g / b values
	// "SW:WWW" set white level
	// "SO:RRRGGGBBB" set offset r / g / b
	// "SM:RRRGGGBBB" set maximum r / g / b
		
	// "SHSV:HHHSSSVVV\0"
	// "SMD:MMM"
	// MMM = 00: RGB Values
	// MMM = 01: HSV Values
	// "G\0" -> get current rgb values and mode
	
	bufferCursor = strstr( commandBuffer, "SRGB:" );
	if (bufferCursor != NULL)
	{      
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 5;
		rgb.r = parseNextInt(&bufferCursor);
		rgb.g = parseNextInt(&bufferCursor);
		rgb.b = parseNextInt(&bufferCursor);	
	}
	
	bufferCursor = strstr( commandBuffer, "SM:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		tRgb.r = parseNextInt(&bufferCursor);
		tRgb.g = parseNextInt(&bufferCursor);
		tRgb.b = parseNextInt(&bufferCursor);
	}
	
	bufferCursor = strstr( commandBuffer, "SO:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		offsetRed    = parseNextInt(&bufferCursor);
		offsetGreen  = parseNextInt(&bufferCursor);
		offsetBlue   = parseNextInt(&bufferCursor);
	}
	
	bufferCursor = strstr( commandBuffer, "SW:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		setWhite(parseNextInt(&bufferCursor));
	}
}

int parseNextInt(char** buffer)
{
	char val[4];	
	strncpy(val,*buffer,3);
	val[3]='\0';
	*buffer = *buffer + 3;
	return rgb.b = atoi(val);	
}

int main(void)
{
	SLED_DDR = (1 << PIND6);
	
	welcome = PSTR("\f\aTiny RGB 1.0 by Paul Rogalinski\r\n\0");

	initSerial();
	initPwm();
	
	writePgmStringToSerial(welcome);
	
	setRgb(255,0,0);
	_delay_ms(1000);
	
	setRgb(0,255,0);
	_delay_ms(1000);

	setRgb(0,0,255);
	_delay_ms(1000);
	
	setRgb(0,0,0);
	mode = MODE_FADE_HSV;
	hsv.s = 254;
	hsv.v = 255;
	mRgb.r = 255;
	mRgb.g = 255;
	mRgb.b = 255;
	
	// our "main" loop is scheduled by timer1's overflow IRQ
	// @20MHz system clock and a /64 timer pre-scaler the ISR will run at 610.3515625 Hz / every 1.639ms 
	TIMSK |= (1 << TOIE1);
	sei();
	
	setCommandBufferCallback(serialBufferHandler);
      
	for(;;) { }
}


ISR(TIMER1_OVF_vect)
{
	if (currentWait-- == 0)
	{
		currentWait = wait;
		if (mode == MODE_FADE_RANDOM_RGB)
		{
			if (tRgb.r == rgb.r) tRgb.r = getNextRandom(mRgb.r, offsetRed, rgb.r);
			if (tRgb.g == rgb.g) tRgb.g = getNextRandom(mRgb.g, offsetGreen, rgb.g);
			if (tRgb.b == rgb.b) tRgb.b = getNextRandom(mRgb.b, offsetBlue, rgb.b);
			rgb.r += (rgb.r < tRgb.r) ? 1 : -1;
			rgb.g += (rgb.g < tRgb.g) ? 1 : -1;
			rgb.b += (rgb.b < tRgb.b) ? 1 : -1;
		}
		
		else if (mode == MODE_FADE_HSV)
		{
			hsv.h++;					
			rgb = hsvToRgb(hsv);
		}
		
		setRgb(rgb.r,rgb.g,rgb.b);
		SLED_PORT ^= (1<<SLED_PIN);
	}	

}