/*
 * UART.h
 *
 *  Created on: Oct 13, 2025
 *      Author: Joaquin Torino
 */

#ifndef DRIVERS_UART_H_
#define DRIVERS_UART_H_

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"

typedef int16_t UART_Handle;

enum UART_Mode {
	UART_RECEIVER,			// Receiver only function
	UART_TRANSMITTER,		// Trasmitter only function
	UART_TRANSCEIVER		// Trasmitter and Receiver function
};

typedef struct {
	pin_t rx;
	pin_t tx;
	pin_t cts;
	pin_t rts;

	uint8_t uartNum;		// Number of UART module to be used

	uint16_t mode;

	uint32_t baudRate; 	// 13 bit register
	uint8_t brfd;			// Correction factor for the baud rate divider

	bool extendedDataBits;	// Set to 0 for the default 8 bit word, true for the extended 9 bit
	bool parityEnable;		// enable/disable parity error detection
	bool parityType; 		// true for odd, false for even

	uint8_t receiveBufferSize;
	uint8_t transmitBufferSize;
} UART_Config;

UART_Handle UART_Init(UART_Config* pConfig);

char UART_GetChar(UART_Handle handle);
void UART_PutChar(UART_Handle handle, uint8_t c);

void UART_Delete(UART_Handle handle);

#endif /* DRIVERS_UART_H_ */
