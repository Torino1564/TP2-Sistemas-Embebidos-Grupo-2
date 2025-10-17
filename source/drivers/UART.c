/*
 * UART.c
 *
 *  Created on: Oct 13, 2025
 *      Author: Joaquin Torino
 */

#include "UART.h"
#include <stdlib.h>
#include <string.h>
#include "hardware.h"

#define MAX_UART_MODULES 6

typedef struct {
	UART_Config config;
	uint8_t rxFifoSize;
	uint8_t txFifoSize;

	char* pReceiveBuffer;
	uint8_t receiveBufferPointer;
	uint8_t receiveBufferBarrier;
	uint8_t availableBytes;

	char* pTransmitBuffer;
	uint8_t transmitBufferPointer;
	uint8_t transmitBufferBarrier;

	bool transmitting;

	bool newData;
	uint8_t newDataByteSize;
} UART;

static UART* modules[MAX_UART_MODULES];
static UART_Type* configRegisters[MAX_UART_MODULES] = UART_BASE_PTRS;

#define CORE_CLOCK 		120000000
#define SYSTEM_CLOCK 	CORE_CLOCK / 2
#define BUS_CLOCK 		CORE_CLOCK / 2

#define DEFAULT_BUFFER_SIZE 64u
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void UART_TransmitBuffer_Impl(uint8_t numUart)
{
	UART_Type* pUCR = configRegisters[numUart];
	UART* pUART = modules[numUart];
	const volatile uint8_t* s1 = &pUCR->S1;
	uint8_t transmitBarrier = pUART->transmitBufferBarrier;
	uint8_t transmitPointer = pUART->transmitBufferPointer;
	uint8_t bufferSize = pUART->config.transmitBufferSize;
	uint8_t available_bytes = 0;
	if (transmitPointer == transmitBarrier)
	{
		// Disable Transmit Interrupts
		pUCR->C2 = pUCR->C2 & ~UART_C2_TIE_MASK;
		pUART->transmitting = 0;
		return;
	}
	else
	{
		// Barrier in fron of pointer
		if (transmitBarrier > transmitPointer)
		{
			available_bytes = transmitPointer + (bufferSize - transmitBarrier);
		}
		// Barrier behind pointer
		else
		{
			available_bytes = transmitPointer - transmitBarrier;
		}

		// This is the number of bytes that the transmitter has yet to send
		uint8_t remaining_to_send = bufferSize - available_bytes;

		// This is the maximum number it can send in this specific interruption. This is the remaining bytes to send clamped to the fifo size
		uint8_t allowed_to_send = MIN(remaining_to_send, pUART->txFifoSize - pUCR->TCFIFO);

		// Wait for the data shift register to be empty
		while(!(*s1 & UART_S1_TDRE_MASK)) {}

		// Send each byte to the fifo one by one, waiting for the register to be ready each time
		for (int i = 0; i < allowed_to_send; i++)
		{
			pUCR->D = pUART->pTransmitBuffer[(transmitPointer + i) % bufferSize];
			while(!(*s1 & UART_S1_TDRE_MASK)) {}
		}

		// Adjust the new value for the transmit pointer and available bytes variables
		pUART->transmitBufferPointer = (transmitPointer + allowed_to_send) % bufferSize;
		pUART->availableBytes = available_bytes + allowed_to_send;

		if (transmitPointer != transmitBarrier)
		{
			pUCR->C2 |= UART_C2_TIE(1);
		}
	}
}

static void UARTX_ERR_IRQImpl(uint8_t numUart)
{
	UART_Type* pUCR = configRegisters[numUart];
	UART* pUART = modules[numUart];
	volatile uint8_t s1 = pUCR->S1;

	if (s1 & UART_S1_OR_MASK)
	{
		pUART->pReceiveBuffer[pUART->receiveBufferPointer++] = pUCR->D;
	}
}

static void UARTX_RX_TX_IRQImpl(uint8_t numUart)
{
	UART_Type* pUCR = configRegisters[numUart];
	UART* pUART = modules[numUart];
	volatile uint8_t s1 = pUCR->S1;

	// Test for each type of interrupt
	// Transmitter complete interrupt
	if (pUART->transmitting && (s1 & UART_S1_TDRE_MASK))
	{
		UART_TransmitBuffer_Impl(numUart);
	}
	// Received data interrupt
	else if (s1 & UART_S1_RDRF_MASK)
	{
		// TODO: FIX
		pUART->pReceiveBuffer[pUART->receiveBufferPointer++] = pUCR->D;
	}
}

static uint8_t MapFIFOSizeToBytes(const uint8_t XXFIFOSIZE)
{
	switch (XXFIFOSIZE)
	{
	case 0b000:
		return 1;
	case 0b001:
		return 4;
	case 0b010:
		return 8;
	case 0b011:
		return 16;
	case 0b100:
		return 32;
	case 0b101:
		return 64;
	case 0b110:
		return 128;
	default:
		return 0;
	}
}

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
	modules[pConfig->uartNum] = (UART*)calloc(1, sizeof(UART));
	UART* pUART = modules[pConfig->uartNum];
	pUART->config = *pConfig;

	uint64_t uart_module_clock = BUS_CLOCK;

	SIM->SCGC4 |= SIM_SCGC4_UART0(1);

	// Enable Clock Gating and interrupts

	switch (pConfig->uartNum)
	{
	case 0:
			SIM->SCGC4 |= SIM_SCGC4_UART0(1);
			SIM->SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
			SIM->SOPT5 |= (SIM_SOPT5_UART0TXSRC(1) | SIM_SOPT5_UART0RXSRC(1));
			uart_module_clock = SYSTEM_CLOCK;
			NVIC_EnableIRQ(UART0_RX_TX_IRQn);
			break;
	case 1:
			SIM->SCGC4 |= SIM_SCGC4_UART1(1);
			SIM->SOPT5 &= ~(SIM_SOPT5_UART1TXSRC_MASK | SIM_SOPT5_UART1RXSRC_MASK);
			SIM->SOPT5 |= (SIM_SOPT5_UART1TXSRC(1) | SIM_SOPT5_UART1RXSRC(1));
			uart_module_clock = SYSTEM_CLOCK;
			NVIC_EnableIRQ(UART1_RX_TX_IRQn);
			break;
	case 2:
			SIM->SCGC4 |= SIM_SCGC4_UART2(1);
			NVIC_EnableIRQ(UART2_RX_TX_IRQn);
			break;
	case 3:
			SIM->SCGC4 |= SIM_SCGC4_UART3(1);
			NVIC_EnableIRQ(UART3_RX_TX_IRQn);
			break;
	case 4:
			SIM->SCGC1 |= SIM_SCGC1_UART4(1);
			NVIC_EnableIRQ(UART4_RX_TX_IRQn);
			break;
	case 5:
			SIM->SCGC1 |= SIM_SCGC1_UART5(1);
			NVIC_EnableIRQ(UART5_RX_TX_IRQn);
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
	pUART->rxFifoSize = MapFIFOSizeToBytes(UART_PFIFO_RXFIFOSIZE(pUCR->PFIFO));
	pUART->txFifoSize = MapFIFOSizeToBytes(UART_PFIFO_TXFIFOSIZE(pUCR->PFIFO));

	// Buffer initialization
	if (pUART->config.transmitBufferSize == 0)
		pUART->config.transmitBufferSize = DEFAULT_BUFFER_SIZE;

	if (pUART->config.receiveBufferSize == 0)
		pUART->config.receiveBufferSize = DEFAULT_BUFFER_SIZE;

	pUART->pReceiveBuffer = (char*)calloc(pUART->config.receiveBufferSize, 1);
	pUART->pTransmitBuffer = (char*)calloc(pUART->config.transmitBufferSize, 1);

	pUART->availableBytes = pUART->config.transmitBufferSize;

	// Interrupt Setup
	pUCR->C2 |= UART_C2_RIE(1);
	pUCR->C3 |= UART_C3_ORIE(1);
	// Enable transmitter/receiver
	pUCR->C2 |=	(pConfig->mode == UART_RECEIVER ? UART_C2_RE(1) :
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

bool UART_WriteData(UART_Handle handle, const uint8_t* pData, uint8_t size)
{
	// Check if there is room in the buffer
	UART* pUART = modules[handle];
	if (pUART->availableBytes < size)
	{
		return 0;
	}

	// There is room for the data
	// Copy data to the buffer
	uint8_t barrier = pUART->transmitBufferBarrier;
	uint8_t pointer = pUART->transmitBufferPointer;
	if (barrier >= pointer)
	{
		// Copy in two parts
		uint8_t firstBatchSize = MIN(pUART->config.receiveBufferSize - barrier, size);
		memcpy(pUART->pTransmitBuffer + barrier, pData, firstBatchSize);

		// Second Part
		if (firstBatchSize < size)
		{
			uint8_t secondBatchSize = size - firstBatchSize;
			memcpy(pUART->pTransmitBuffer, pData, secondBatchSize);
		}
	}
	else
	{
		memcpy(pUART->pTransmitBuffer + barrier, pData, size);
	}

	pUART->availableBytes -= size;
	pUART->transmitBufferBarrier = (pUART->transmitBufferBarrier + size) % pUART->config.transmitBufferSize;

	pUART->transmitting = 1;
	UART_TransmitBuffer_Impl(handle);

	return 1;
}

#define UARTX_RX_TX_IRQ_IMPL(x)				\
__ISR__ UART##x##_RX_TX_IRQHandler(void)	\
{											\
	UARTX_RX_TX_IRQImpl(x);					\
}

#define UARTX_ERR_IRQ_IMPL(x)				\
__ISR__ UART##x##_ERR_IRQHandler(void)		\
{											\
	UARTX_ERR_IRQImpl(x);					\
}

#define UARTX_IRQ_IMPL(x) 		\
		UARTX_RX_TX_IRQ_IMPL(x)	\
		UARTX_ERR_IRQ_IMPL(x)


UARTX_IRQ_IMPL(0)
UARTX_IRQ_IMPL(1)
UARTX_IRQ_IMPL(2)
UARTX_IRQ_IMPL(3)
UARTX_IRQ_IMPL(4)
UARTX_IRQ_IMPL(5)

