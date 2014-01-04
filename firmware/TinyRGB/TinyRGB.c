/*
* TinyRGB.c
*
* Created: 11.09.2013 20:10:35
* Author: Paul Rogalinski, paul@paul.vc
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

HslColor  hsl;   // current hsl values (in MODE_FADE_HSL)
RgbColor  oRgb;  // offset values for rgb, cast to int8_t before using
RgbColor  rgb;   // current rgb values (in MODE_FADE_RANDOM_RGB)
RgbColor  tRgb;  // target rgb values (for MODE_FADE_RANDOM_RGB)
RgbColor  mRgb;  // maximum / clipping values
uint8_t   white; // white level, unsupported by the current hardware

uint8_t mode = MODE_FADE_RANDOM_RGB;
uint8_t wait = 100;
uint8_t currentWait = 100;

bool autosaveEnabled = true;

/*
 * take a guess.
 */
void printHelp()
{
	writePgmStringToSerial(PSTR("\f\aTiny RGB 0.1, github.com/pulsar256/tinyrgb \r\nFull protocol specification available at Github.\r\nCompiled: " __TIMESTAMP__ "\r\n"));
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
	return atoi(val);
}

/*
* dumps the contents of the RgbColor (or casted HslColor) structure to serial.
*/
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
 * "SRGB:RRRGGGBBB"   set r / g / b values, implicit change to mode 002 - fixed RGB/HSL.
 * "SW:WWW"           set white level (unsupported by the current hardware)
 * "SO:±RR±GG±BB"     set offset r / g / b for RGB Fade Mode (+/-99)
 * "SM:RRRGGGBBB"     set maximum r / g / b for RGB Fade Mode
 * "SSV:SSSVVV"       set SV for HSL Fade mode
 * "SMD:MMM"          set mode (001 - random rgb fade, 002 - fixed RGB/HSL, 003 - HSL Fade (buggy))
 * "SD:DDD"           set delay for fade modes.
 * "status"           get current rgb values and mode
 * "help"             explains the protocol.
 */
bool handleCommands(char* commandBuffer)
{
	char *bufferCursor;
	
	// Set RGB values and switch to MODE_FIXED (002)
	bufferCursor = strstr( commandBuffer, "SRGB:" );
	if (bufferCursor != NULL)
	{
		mode = MODE_FIXED;
		bufferCursor += 5;
		rgb.r = parseNextInt(&bufferCursor);
		rgb.g = parseNextInt(&bufferCursor);
		rgb.b = parseNextInt(&bufferCursor);
		return true;
	}
	
	// Set max RGB
	bufferCursor  = strstr( commandBuffer, "SM:" );
	if (bufferCursor != NULL)
	{
		bufferCursor += 3;
		mRgb.r = parseNextInt(&bufferCursor);
		mRgb.g = parseNextInt(&bufferCursor);
		mRgb.b = parseNextInt(&bufferCursor);
		return true;
	}
	
	// Set RGB offset
	bufferCursor  = strstr( commandBuffer, "SO:" );
	if (bufferCursor != NULL)
	{
		bufferCursor += 3;
		oRgb.r  = parseNextInt(&bufferCursor);
		oRgb.g  = parseNextInt(&bufferCursor);
		oRgb.b  = parseNextInt(&bufferCursor);
		return true;
	}
	
	#ifdef ENABLE_WHITECHANNEL
		// Set white level (unsupported by current hardware design)
		bufferCursor  = strstr( commandBuffer, "SW:" );
		if (bufferCursor != NULL)
		{
			bufferCursor += 3;
			white = parseNextInt(&bufferCursor);
			return true;
		}
	#endif
	
	// set delay / wait cycles between color changes.
	bufferCursor  = strstr( commandBuffer, "SD:" );
	if (bufferCursor != NULL)
	{
		bufferCursor += 3;
		wait = parseNextInt(&bufferCursor);
		return true;
	}
	
	// Set HSL register values, values < 0 are ignored, 
	// mode is not changed to fixed to keep the colors
	// cycling while adjusting saturation or luminosity.
	bufferCursor  = strstr( commandBuffer, "SHSL:" );
	if (bufferCursor != NULL)
	{
		bufferCursor += 5;
		int tmp = 0;
		tmp = parseNextInt(&bufferCursor);
		if (tmp > 0) hsl.h = tmp;
		tmp = parseNextInt(&bufferCursor);
		if (tmp > 0) hsl.s = tmp;
		tmp = parseNextInt(&bufferCursor);
		if (tmp > 0) hsl.l = tmp;
		
		// we always use the rgb register to set the output PWM channels.
		// in order to use hsl in a fixed mode, we need to do the conversion
		// once here.
		hslToRgb(&hsl,&rgb);
		
		return true;
	}
	
	// Sets the delay
	bufferCursor  = strstr( commandBuffer, "SMD:" );
	if (bufferCursor != NULL)
	{
		bufferCursor += 4;
		mode = parseNextInt(&bufferCursor);
		return true;
	}
	
	// Dumps all status registers
	bufferCursor = strstr( commandBuffer, "status" );
	if (bufferCursor != NULL)
	{
		char buffer[4];
		
		writeNewLine();
		writePgmStringToSerial(PSTR("#Mode:"));
		writeNewLine();
		itoa(mode,buffer,10);
		writeStringToSerial(buffer);
		writeNewLine();
		
		writeNewLine();
		writePgmStringToSerial(PSTR("#Delay:"));
		writeNewLine();
		itoa(wait,buffer,10);
		writeStringToSerial(buffer);
		writeNewLine();
		
		writeNewLine();
		writePgmStringToSerial(PSTR("#Autosave:"));
		writeNewLine();
		if (autosaveEnabled) writePgmStringToSerial(PSTR("enabled"));
		else writePgmStringToSerial(PSTR("disabled"));
		writeNewLine();
		
		writeNewLine();
		writePgmStringToSerial(PSTR("#Current RGB Values:"));
		writeNewLine();
		dumpRgbColorToSerial(&rgb);
		
		writeNewLine();
		writePgmStringToSerial(PSTR("#Current HSL Values:"));
		writeNewLine();
		dumpRgbColorToSerial((struct RgbColor *)&hsl); // hsl has same memory layout as rgb
		
		writeNewLine();
		writePgmStringToSerial(PSTR("#Current RGB Offset Values:"));
		writeNewLine();
		dumpRgbColorToSerial(&oRgb);
		
		writeNewLine();
		writePgmStringToSerial(PSTR("#Current RGB Clipping Values:"));
		writeNewLine();
		dumpRgbColorToSerial(&mRgb);
		
		return true;
	}
	
	// Dumps all status registers
	bufferCursor = strstr( commandBuffer, "help" );
	if (bufferCursor != NULL)
	{
		printHelp();
		return true;
	}
	
	// Enables (value > 0) / Disables (value == 0) the autosave function.
	bufferCursor  = strstr( commandBuffer, "SAV:" );
	if (bufferCursor != NULL)
	{
		bufferCursor += 4;
		uint8_t value =  parseNextInt(&bufferCursor);
		if (value > 0) autosaveEnabled = true;
		else autosaveEnabled = false;
		return true;	
	}
	
	return false;	
}


/*
 * Callback for the USART handler, executed each time the buffer is full or a 
 * newline has been submitted.
 */
void serialBufferHandler(char* commandBuffer)
{
	cli();
	
	if (!handleCommands(commandBuffer))
	{
		writePgmStringToSerial(PSTR("\r\nERR\r\n"));
	}
	else
	{
		writePgmStringToSerial(PSTR("\r\nOK\r\n"));
		if(autosaveEnabled) updateEEProm();
	}
	
	
	#ifdef ENABLE_BLINK_CONFIRM
	// "roger-beep", very annoying if you do real time updates.
	setRgb(0,0,0);
	_delay_ms(10);
	setRgb(0,255,0);
	_delay_ms(100);
	setRgb(0,0,0);
	_delay_ms(10);
	#endif
	
	sei();
}


/*
 * writes an uint8 value into the eeprom at the offset position. 
 * the passed offset will be off set once again by the value of 
 * EEPStart
 */
void writeEepromByte(int offset, uint8_t value)
{
	uint8_t* addr = (uint8_t*)(EEPStart + offset);
	eeprom_update_byte(addr,value);
}


/*
 * reads an uint8 value from the eeprom at the offset position. 
 * the passed offset will be off set once again by the value of 
 * EEPStart
 */
uint8_t readEepromByte(int offset)
{
	uint8_t* addr = (uint8_t*)(EEPStart + offset);
	return eeprom_read_byte(addr);
}


/*
 * Saves current state into the eeprom
 */
void updateEEProm()
{
	int c = 0;
	writeEepromByte(c++, mode);
	writeEepromByte(c++, rgb.r);
	writeEepromByte(c++, rgb.g);
	writeEepromByte(c++, rgb.b);
	writeEepromByte(c++, hsl.h);
	writeEepromByte(c++, hsl.s);
	writeEepromByte(c++, hsl.l);
	writeEepromByte(c++, oRgb.r);
	writeEepromByte(c++, oRgb.g);
	writeEepromByte(c++, oRgb.b);
	writeEepromByte(c++, wait);
	writeEepromByte(c++, mRgb.r);
	writeEepromByte(c++, mRgb.g);
	writeEepromByte(c++, mRgb.b);
	#ifdef ENABLE_WHITECHANNEL
	writeEepromByte(c++, white);
	#endif
}


/*
 * restores the current state from the eeprom 
 */
void restoreFromEEProm()
{
	int c = 0;
	mode = readEepromByte(c++);
	if (mode == 0) return; // state not saved yet.
	rgb.r = readEepromByte(c++);
	rgb.g = readEepromByte(c++);
	rgb.b = readEepromByte(c++);
	hsl.h = readEepromByte(c++);
	hsl.s = readEepromByte(c++);
	hsl.l = readEepromByte(c++);
	oRgb.r = readEepromByte(c++);
	oRgb.g = readEepromByte(c++);
	oRgb.b = readEepromByte(c++);
	wait = readEepromByte(c++);
	mRgb.r = readEepromByte(c++);
	mRgb.g = readEepromByte(c++);
	mRgb.b = readEepromByte(c++);
	#ifdef ENABLE_WHITECHANNEL
	white = readEepromByte(c++);
	#endif
}


/*
 * Initialization routines, self test and welcome screen. Main loop is implemented in the ISR below
 */
int main(void)
{
	SLED_DDR = (1 << PIND6);

	initSerial();
	initPwm();
	restoreFromEEProm();
	
	setRgb(0,0,0);
	if (mode == 0)
	{
		mode = MODE_FADE_HSL;
		hsl.s = 254;
		hsl.l = 127;
		mRgb.r = 255;
		mRgb.g = 255;
		mRgb.b = 255;
	}	
	
	printHelp();
	setCommandBufferCallback(serialBufferHandler);
	
	// our "main" loop is scheduled by timer1's overflow IRQ
	// @20MHz system clock and a /64 timer pre-scaler the ISR will run at 610.3515625 Hz / every 1.639ms
	TIMSK |= (1 << TOIE1);
	sei();

	while(1);
}


/*
 * Scheduled to run every 1-2ms (depending on your system clock). Transfers the color values into
 * the PWM registers and, depending on the current mode, computes the colorcycling effects
 */
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
		
		else if (mode == MODE_FADE_HSL)
		{
			hsl.h++;
			hslToRgb(&hsl,&rgb);
		}
		
		setRgb(rgb.r,rgb.g,rgb.b);
		#ifdef ENABLE_WHITECHANNEL
		setWhite(white);
		#endif
		SLED_PORT ^= (1<<SLED_PIN);
	}
}