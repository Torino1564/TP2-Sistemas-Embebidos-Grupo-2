/*
 * UART.c
 *
 *  Created on: Oct 13, 2025
 *      Author: Joaquin Torino
 */

#include "UART.h"
#include <stdlib.h>
#include "hardware.h"

#define MAX_UART_MODULES 6

typedef struct {
	UART_Config config;
	uint8_t rxFifoSize;
	uint8_t txFifoSize;

	char* pReceiveBuffer;
	char* pTransmitBuffer;
} UART;

static UART* modules[MAX_UART_MODULES];
static UART_Type* configRegisters[MAX_UART_MODULES] = UART_BASE_PTRS;

#define CORE_CLOCK 		120000000
#define SYSTEM_CLOCK 	CORE_CLOCK / 2
#define BUS_CLOCK 		CORE_CLOCK / 2

UART_Handle UART_Init(UART_Config* pConfig)
{
	// Assert empty slot
	if (modules[pConfig->uartNum] != 0)
	{
		return -1;
	}

	// TODO: This should not be here
	SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV1(1)  // Divide core/system by 2
	              | SIM_CLKDIV1_OUTDIV2(1)  // Divide bus by 2
	              | SIM_CLKDIV1_OUTDIV4(3); // Divide flash by 4 → 30 MHz


	// Pin Configuration
	gpioMode(pConfig->rx, INPUT);
	gpioMux(pConfig->rx, 3);
	gpioMode(pConfig->tx, OUTPUT);
	gpioMux(pConfig->tx, 3);

	if (pConfig->cts != 0)
		gpioMux(pConfig->cts, 3);
	if (pConfig->rts != 0)
		gpioMux(pConfig->rts, 3);

	if (pConfig->uartNum == 0)
	{
		if (pConfig->cts == PORTNUM2PIN(PA, 0))
			gpioMux(pConfig->cts, 2);
		if (pConfig->rx == PORTNUM2PIN(PA, 1))
			gpioMux(pConfig->rx, 2);
		if (pConfig->tx == PORTNUM2PIN(PA, 2))
			gpioMux(pConfig->tx, 2);
		if (pConfig->rts == PORTNUM2PIN(PA, 3))
			gpioMux(pConfig->rts, 2);
	}

	// Register UART in driver
	modules[pConfig->uartNum] = (UART*)malloc(sizeof(UART));
	UART* pUART = modules[pConfig->uartNum];
	pUART->config = *pConfig;

	uint64_t uart_module_clock = BUS_CLOCK;

	SIM->SCGC4 |= SIM_SCGC4_UART0(1);
	// Enable Clock Gating
	switch (pConfig->uartNum){
	case 0:
			SIM->SCGC4 |= SIM_SCGC4_UART0(1);
			SIM->SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
			SIM->SOPT5 |= (SIM_SOPT5_UART0TXSRC(1) | SIM_SOPT5_UART0RXSRC(1));
			uart_module_clock = SYSTEM_CLOCK;
			break;
	case 1:
			SIM->SCGC4 |= SIM_SCGC4_UART1(1);
			SIM->SOPT5 &= ~(SIM_SOPT5_UART1TXSRC_MASK | SIM_SOPT5_UART1RXSRC_MASK);
			SIM->SOPT5 |= (SIM_SOPT5_UART1TXSRC(1) | SIM_SOPT5_UART1RXSRC(1));
			uart_module_clock = SYSTEM_CLOCK;
			break;
	case 2:
			SIM->SCGC4 |= SIM_SCGC4_UART2(1);
			break;
	case 3:
			SIM->SCGC4 |= SIM_SCGC4_UART3(1);
			break;
	case 4:
			SIM->SCGC1 |= SIM_SCGC1_UART4(1);
			break;
	case 5:
			SIM->SCGC1 |= SIM_SCGC1_UART5(1);
			break;
	}

	// UART Register Pointer
	UART_Type* pUCR = configRegisters[pConfig->uartNum];

	// Turn off Transmitter/receiver for configuration
	pUCR->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);

	// Baud rate configuration
	uint16_t sbr = (uint16_t)(((double)uart_module_clock / (double)pConfig->baudRate) - (double)pConfig->brfd/(double)32);

	pUCR->BDH = (uint8_t)((0b0001111100000000 & sbr) >> 8);
	pUCR->BDL = (uint8_t)(0b0000000011111111 & sbr);
	pUCR->C1 = UART_C1_PT(pConfig->parityType) | UART_C1_PE(pConfig->parityEnable);

	// FIFO Configuration
	pUCR->PFIFO |= UART_PFIFO_TXFE(1) | UART_PFIFO_RXFE(1);
	pUCR->CFIFO |= UART_CFIFO_TXFLUSH(1) | UART_CFIFO_RXFLUSH(1);
	pUART->rxFifoSize = UART_PFIFO_RXFIFOSIZE(pUCR->PFIFO);
	pUART->txFifoSize = UART_PFIFO_TXFIFOSIZE(pUCR->PFIFO);

	// Buffer initialization
	if (pUART->config.transmitBufferSize == 0)
		pUART->config.transmitBufferSize = 16;

	if (pUART->config.receiveBufferSize == 0)
		pUART->config.receiveBufferSize = 16;

	pUART->pReceiveBuffer = (char*)malloc(pUART->config.receiveBufferSize);
	pUART->pTransmitBuffer = (char*)malloc(pUART->config.transmitBufferSize);

	// Enable transmitter/receiver
	pUCR->C2 = UART_C2_TIE(1) | UART_C2_TCIE(1) | UART_C2_RIE(1) |
			(pConfig->mode == UART_RECEIVER ? UART_C2_RE(1) :
			( pConfig->mode == UART_TRANSMITTER ? UART_C2_TE(1):(
			UART_C2_TE(1) | UART_C2_RE(1))));


	return pConfig->uartNum;
}


char UART_GetChar(UART_Handle handle)
{
	// UART Register Pointer
	UART_Type* pUCR = configRegisters[handle];
	while (!(pUCR->S1 & UART_S1_RDRF_MASK))
	{
		return pUCR->D;
	}
	return 0;
}


void UART_PutChar(UART_Handle handle, uint8_t c)
{
	// UART Register Pointer
	UART_Type* pUCR = configRegisters[handle];
	while (!(pUCR->S1 & UART_S1_TDRE_MASK));

	pUCR->D = c;
}
