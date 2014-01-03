/*
 * usart.h
 *
 * Created: 15.09.2013 03:05:59
 *  Author: pulsar
 */ 


#ifndef USART_H_
#define USART_H_

#include<stdbool.h>


typedef void (*buffer_ready_callback)(char* commandBuffer);
void trgbser_def_cb(){}
buffer_ready_callback trgbser_callback = trgbser_def_cb;

char    commandBuffer[16];
uint8_t bufferPos = 0;
const char* trgbser_prompt;
volatile bool bufferReady = false;


void setCommandBufferCallback(buffer_ready_callback cb)
{
	trgbser_callback = cb;
}

void initSerial(void)
{
	UCSRC |= (1 << UCSZ0) | (1 << UCSZ1); // 1 Stop-Bit, 8 Bits
	UCSRB = (1 << TXEN)  | (1 <<  RXEN); // enable rx/tx
	UCSRB |= (1 << RXCIE); // Enable the USART Recieve Complete interrupt (USART_RXC)
	
	/* UBRRL and UBRRH – USART Baud Rate Registers */
	UBRRH = (uint8_t) (UART_BAUD_CALC(UART_BAUD_RATE,F_CPU)>>8);
	UBRRL = (uint8_t) (UART_BAUD_CALC(UART_BAUD_RATE,F_CPU));
	
	trgbser_prompt = PSTR("\r\n< RECEIVED \r\n");
}

void writeCharToSerial(unsigned char c)
{
	// UCSRA – USART Control and Status Register A
	// • Bit 5 – UDRE: USART Data Register Empty
	while(!(UCSRA & (1 << UDRE)));
	UDR = c;
}

void writePgmStringToSerial(const char *pgmString)
{
	while (pgm_read_byte(pgmString) != 0x00) writeCharToSerial((char)pgm_read_byte(pgmString++));
}

void writeStringToSerial(char *str)
{
	while (*str != 0x00) writeCharToSerial(*str++);
}

void writeNewLine()
{
	writeStringToSerial("\r\n");
}

signed char readSerialChar( void )
{
	// UCSRA – USART Control and Status Register A
	// • Bit 7 – RXC: USART Receive Complete
	while ( !(UCSRA & (1<<RXC)) );
	
	return UDR;
}

void waitForBuffer( void )
{
	while (!bufferReady);	
	bufferReady = false;
}


ISR(USART_RX_vect){
	char chrRead;
	chrRead = UDR;  
	commandBuffer[bufferPos++] = chrRead;
	commandBuffer[bufferPos] = 0x00;
	UDR = chrRead;
	if ((bufferPos >= (sizeof(commandBuffer)-1)) || ((chrRead == '\n' || chrRead == '\r'))) 
	{
		bufferPos = 0;
		// writePgmStringToSerial(trgbser_prompt);
		bufferReady = true;
		trgbser_callback(commandBuffer);
	}			
}

#endif /* USART_H_ */