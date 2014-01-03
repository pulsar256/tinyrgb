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

#include "TinyRGB.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdlib.h>

#include "random.h"
#include "pwm.h"
#include "usart.h"
#include "colors.h"

const char* welcome;

HsvColor	hsv; // current hsv values (in MODE_FADE_HSV)
RgbColor    oRgb; // offset values for rgb, cast to int8_t before using
RgbColor 	rgb; // current rgb values (in MODE_FADE_RANDOM_RGB)
RgbColor 	tRgb; // target rgb values (for MODE_FADE_RANDOM_RGB)
RgbColor	mRgb; // maximum / clipping values

uint8_t mode = MODE_FADE_RANDOM_RGB;
uint8_t wait = FADE_WAIT;
uint8_t currentWait = FADE_WAIT;


/*
 * Parses the command buffer (commandBuffer is directly accessed from usart.h) and
 * changes the current state accordingly.
 * Command Syntax:
 * "SRGB:RRRGGGBBB"      set r / g / b values, implicit change to mode 002 - fixed RGB.
 * "SW:WWW"              set white level (unsupported by the current hardware)
 * "SO:±RR±GG±BB"        set offset r / g / b for RGB Fade Mode (+/-99)
 * "SM:RRRGGGBBB"        set maximum r / g / b for RGB Fade Mode
 * "SSV:SSSVVV"          set SV for HSV Fade mode
 * "SMD:MMM"             set mode (001 - random rgb fade, 002 - fixed RGB, 003 - HSV Fade (buggy))
 * "SD:DDD"              set delay for fade modes.
 * "G:"                  get current rgb values and mode
 */
void serialBufferHandler(void)
{
	cli();
	char *bufferCursor;
	
	// SET RGB
	bufferCursor = strstr( commandBuffer, "SRGB:" );
	if (bufferCursor != NULL)
	{      
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 5;
		rgb.r = parseNextInt(&bufferCursor);
		rgb.g = parseNextInt(&bufferCursor);
		rgb.b = parseNextInt(&bufferCursor);	
	}
	
	// Set max RGB
	bufferCursor = strstr( commandBuffer, "SM:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		tRgb.r = parseNextInt(&bufferCursor);
		tRgb.g = parseNextInt(&bufferCursor);
		tRgb.b = parseNextInt(&bufferCursor);
	}
	
	// Set RGB offset
	bufferCursor = strstr( commandBuffer, "SO:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		oRgb.r  = parseNextInt(&bufferCursor);
		oRgb.g  = parseNextInt(&bufferCursor);
		oRgb.b  = parseNextInt(&bufferCursor);
	}
	
	// Set white level (unsupported by current hardware design)
	bufferCursor = strstr( commandBuffer, "SW:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		setWhite(parseNextInt(&bufferCursor));
	}
	
	// set delay / wait cycles between color changes.
	bufferCursor = strstr( commandBuffer, "SD:" );
	if (bufferCursor != NULL)
	{
		bufferCursor = bufferCursor + 3;
		wait = parseNextInt(&bufferCursor);
	}
	
	// Set S and V values of the HSV Register.
	bufferCursor = strstr( commandBuffer, "SSV:" );
	if (bufferCursor != NULL)
	{
		bufferCursor = bufferCursor + 4;
		hsv.s = parseNextInt(&bufferCursor);
		hsv.v = parseNextInt(&bufferCursor);
	}
	
	// Sets the mode
	bufferCursor = strstr( commandBuffer, "SMD:" );
	if (bufferCursor != NULL)
	{
		bufferCursor = bufferCursor + 4;
		mode = parseNextInt(&bufferCursor);
	}
	
	#ifdef BLINK_CONFIRM
		setRgb(0,0,0);
		_delay_ms(10);
		setRgb(0,255,0);
		_delay_ms(100);
		setRgb(0,0,0);
		_delay_ms(10);
	#endif
	sei();
}

void updateEEProm()
{
	int c = 0;
	EEPWriteByte(EEPStart + c++, mode);
	EEPWriteByte(EEPStart + c++, rgb.r);	
	EEPWriteByte(EEPStart + c++, rgb.g);	
	EEPWriteByte(EEPStart + c++, rgb.b);	
	EEPWriteByte(EEPStart + c++, hsv.h);
	EEPWriteByte(EEPStart + c++, hsv.s);
	EEPWriteByte(EEPStart + c++, hsv.v);
	EEPWriteByte(EEPStart + c++, oRgb.r);
	EEPWriteByte(EEPStart + c++, oRgb.g);
	EEPWriteByte(EEPStart + c++, oRgb.b);
}

void restoreFromEEProm()
{
	int c = 0;
	mode = EEPReadByte(EEPStart + c++);
	if (mode == 0) return;
	rgb.r = EEPReadByte(EEPStart + c++); 	
	rgb.g = EEPReadByte(EEPStart + c++); 
	rgb.b = EEPReadByte(EEPStart + c++); 
	hsv.h = EEPReadByte(EEPStart + c++); 
	hsv.s = EEPReadByte(EEPStart + c++); 
	hsv.v = EEPReadByte(EEPStart + c++); 
	oRgb.r = EEPReadByte(EEPStart + c++);
	oRgb.g = EEPReadByte(EEPStart + c++);
	oRgb.b = EEPReadByte(EEPStart + c++);
}

/*
 * Parses an integer value from a triple of chars. advances the pointer to the string afterwards
 */
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
			if (tRgb.r == rgb.r) tRgb.r = getNextRandom(mRgb.r, (int8_t)oRgb.r, rgb.r);
			if (tRgb.g == rgb.g) tRgb.g = getNextRandom(mRgb.g, (int8_t)oRgb.g, rgb.g);
			if (tRgb.b == rgb.b) tRgb.b = getNextRandom(mRgb.b, (int8_t)oRgb.b, rgb.b);
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