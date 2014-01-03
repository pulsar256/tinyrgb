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
 *  9: PD5 / OC0B -> White (unsupported by the current hardware design)
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

const char* response_ok;
const char* response_err;

HsvColor	hsv; // current hsv values (in MODE_FADE_HSV)
RgbColor    oRgb; // offset values for rgb, cast to int8_t before using
RgbColor 	rgb; // current rgb values (in MODE_FADE_RANDOM_RGB)
RgbColor 	tRgb; // target rgb values (for MODE_FADE_RANDOM_RGB)
RgbColor	mRgb; // maximum / clipping values

uint8_t mode = MODE_FADE_RANDOM_RGB;
uint8_t wait = FADE_WAIT;
uint8_t currentWait = FADE_WAIT;

void dumpRgbColorToSerial(RgbColor* color)
{
	char buffer[4];
	itoa(color->r,buffer,10);
	writeStringToSerial(buffer);
	writeNewLine();
	itoa(color->g,buffer,10);
	writeStringToSerial(buffer);
	writeNewLine();
	itoa(color->b,buffer,10);
	writeStringToSerial(buffer);
	writeNewLine();
}



/*
 * Parses the command buffer and changes the current state accordingly.
 *
 * Command Syntax:
 * "SRGB:RRRGGGBBB"      set r / g / b values, implicit change to mode 002 - fixed RGB.
 * "SW:WWW"              set white level (unsupported by the current hardware)
 * "SO:±RR±GG±BB"        set offset r / g / b for RGB Fade Mode (+/-99)
 * "SM:RRRGGGBBB"        set maximum r / g / b for RGB Fade Mode
 * "SSV:SSSVVV"          set SV for HSV Fade mode
 * "SMD:MMM"             set mode (001 - random rgb fade, 002 - fixed RGB, 003 - HSV Fade (buggy))
 * "SD:DDD"              set delay for fade modes.
 * "status"              get current rgb values and mode
 * "help"                explains the protocol.
 */
void serialBufferHandler(char* commandBuffer)
{
	cli();
	char *bufferCursor;
	bool processed = false;
	
	if (!processed) bufferCursor = strstr( commandBuffer, "SRGB:" );
	
	if (bufferCursor != NULL)
	{      
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 5;
		rgb.r = parseNextInt(&bufferCursor);
		rgb.g = parseNextInt(&bufferCursor);
		rgb.b = parseNextInt(&bufferCursor);
		processed = true;
		bufferCursor = NULL;
	}
	
	// Set max RGB
	if (!processed) bufferCursor  = strstr( commandBuffer, "SM:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		tRgb.r = parseNextInt(&bufferCursor);
		tRgb.g = parseNextInt(&bufferCursor);
		tRgb.b = parseNextInt(&bufferCursor);
		processed = true;
		bufferCursor = NULL;
	}
	
	// Set RGB offset
	if (!processed) bufferCursor  = strstr( commandBuffer, "SO:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		oRgb.r  = parseNextInt(&bufferCursor);
		oRgb.g  = parseNextInt(&bufferCursor);
		oRgb.b  = parseNextInt(&bufferCursor);
		processed = true;
		bufferCursor = NULL;
	}
	
	// Set white level (unsupported by current hardware design)
	if (!processed) bufferCursor  = strstr( commandBuffer, "SW:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor = bufferCursor + 3;
		setWhite(parseNextInt(&bufferCursor));
		processed = true;
		bufferCursor = NULL;
	}
	
	// set delay / wait cycles between color changes.
	if (!processed) bufferCursor  = strstr( commandBuffer, "SD:" );
	if (bufferCursor != NULL)
	{
		bufferCursor = bufferCursor + 3;
		wait = parseNextInt(&bufferCursor);
		processed = true;
		bufferCursor = NULL;
	}
	
	// Set S and V values of the HSV Register.
	if (!processed) bufferCursor  = strstr( commandBuffer, "SSV:" );
	if (bufferCursor != NULL)
	{
		bufferCursor = bufferCursor + 4;
		hsv.s = parseNextInt(&bufferCursor);
		hsv.v = parseNextInt(&bufferCursor);
		processed = true;
		bufferCursor = NULL;
	}
	
	// Sets the delay
	if (!processed) bufferCursor  = strstr( commandBuffer, "SMD:" );
	if (bufferCursor != NULL)
	{
		bufferCursor = bufferCursor + 4;
		mode = parseNextInt(&bufferCursor);
		processed = true;
		bufferCursor = NULL;
	}
	
	// Dumps all status registers
	if (!processed) bufferCursor = strstr( commandBuffer, "status" );
	if (bufferCursor != NULL)
	{
		char buffer[4];
		
		writePgmStringToSerial(PSTR("#Mode: \r\n"));
		itoa(mode,buffer,10);
		writeStringToSerial(buffer);
		writeNewLine();
		
		writePgmStringToSerial(PSTR("#Delay: \r\n"));
		itoa(wait,buffer,10);
		writeStringToSerial(buffer);
		writeNewLine();
		
		writePgmStringToSerial(PSTR("#Current RGB Values:\r\n"));
		dumpRgbColorToSerial(&rgb);		
		writePgmStringToSerial(PSTR("#Current HSV Values:\r\n"));
		dumpRgbColorToSerial((struct RgbColor *)&hsv); // hsv has same memory layout as rgb
		writePgmStringToSerial(PSTR("#Current RGB Offset Values:\r\n"));
		dumpRgbColorToSerial(&oRgb);		
		writePgmStringToSerial(PSTR("#Current RGB Clipping Values:\r\n"));
		dumpRgbColorToSerial(&mRgb);
		processed = true;
		bufferCursor = NULL;
	}
	
	// Dumps all status registers
	if (!processed) bufferCursor = strstr( commandBuffer, "help" );
	if (bufferCursor != NULL)
	{
		writePgmStringToSerial(PSTR("\r\nCheck github.com/pulsar256/tinyrgb for protocol specification. No more space left on chip\r\n"));
		processed = true;
		bufferCursor = NULL;
	}
	
	if (!processed)
	{
		writePgmStringToSerial(response_err);
	}
	else
	{
		writePgmStringToSerial(response_ok);
		updateEEProm();
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


void writeEEPromValue(int offset, uint8_t value)
{
	uint8_t* addr = (uint8_t*)(EEPStart + offset);
	eeprom_update_byte(addr,value);
}

uint8_t readEEPromValue(int offset)
{
	uint8_t* addr = (uint8_t*)(EEPStart + offset);
	return eeprom_read_byte(addr);
}


void updateEEProm()
{
	int c = 0;
	writeEEPromValue(c++, mode);
	writeEEPromValue(c++, rgb.r);	
	writeEEPromValue(c++, rgb.g);	
	writeEEPromValue(c++, rgb.b);	
	writeEEPromValue(c++, hsv.h);
	writeEEPromValue(c++, hsv.s);
	writeEEPromValue(c++, hsv.v);
	writeEEPromValue(c++, oRgb.r);
	writeEEPromValue(c++, oRgb.g);
	writeEEPromValue(c++, oRgb.b);
	writeEEPromValue(c++, wait);
}

void restoreFromEEProm()
{
	int c = 0;
	mode = readEEPromValue(c++);
	if (mode == 0) return; // state not saved yet.
	rgb.r = readEEPromValue(c++);	
	rgb.g = readEEPromValue(c++);
	rgb.b = readEEPromValue(c++);
	hsv.h = readEEPromValue(c++);
	hsv.s = readEEPromValue(c++);
	hsv.v = readEEPromValue(c++);
	oRgb.r = readEEPromValue(c++);
	oRgb.g = readEEPromValue(c++);
	oRgb.b = readEEPromValue(c++);
	wait = readEEPromValue(c++);
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
	
	response_ok = PSTR("\r\nOK\r\n");
	response_err = PSTR("\r\nERR\r\n");

	initSerial();
	initPwm();
	
	writePgmStringToSerial(PSTR("\f\aTiny RGB 0.1, https://github.com/pulsar256/tinyrgb \r\nCompiled: " __DATE__ "\r\n"));
	
	setRgb(255,0,0);
	restoreFromEEProm();
	_delay_ms(1000);
	
	setRgb(0,255,0);
	_delay_ms(1000);

	setRgb(0,0,255);
	_delay_ms(1000);
	
	setRgb(0,0,0);
	if (mode == 0)
	{
		mode = MODE_FADE_HSV;
		hsv.s = 254;
		hsv.v = 255;
		mRgb.r = 255;
		mRgb.g = 255;
		mRgb.b = 255;	
	}
	
	
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