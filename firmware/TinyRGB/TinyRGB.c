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
#define MODE_FADE_RANDOM_RGB    1
#define MODE_FIXED				2
#define MODE_FADE_HSV			3
#define FADE_WAIT				100
#define LOCAL_ECHO				1;

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "random.h"
#include "pwm.h"
#include "usart.h"
#include "colors.h"

const char* welcome;

int     offsetRed = 0, offsetGreen = 0, offsetBlue = 0; // color offsets, can be negative. rgb output will allways be clipped to 0..255
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
}

int main(void)
{
	SLED_DDR = (1 << PIND6);
	
	welcome = PSTR("\f\aTiny RGB 1.0 by Paul Rogalinski, paul@paul.vc\r\n\r\nKnown commands: \r\nshow - shows current rgb values\r\nset - sets a fixed rgb value\r\nfade - enters the color cycling mode\r\nbounds - sets bounds for max and offset rgb values for the fade mode\r\nspeed - speed in the fade mode, lower values are faster.\r\n\r\nSend any dummy byte (keypress) over serial to enter the commandline mode.\r\n\r\nREADY.\r\n");

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