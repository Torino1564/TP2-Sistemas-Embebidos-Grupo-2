/*****************************************************************************
  @file     App.c
  @brief    Main Application
  @author   Group 2
  @version  1.0 - coding
 ******************************************************************************/

/*******************************************************************************
 *                                ENCABEZADOS
 ******************************************************************************/
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "StateMachine.h"
#include "hardware.h"
#include "drivers/Timer.h"
#include "drivers/UART.h"

/*******************************************************************************
 *                                MACROS
 ******************************************************************************/

#define MAX_STRING_LENGHT 16

/*******************************************************************************
 *                                VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *                           FUNCIONES GLOBALES
 ******************************************************************************/

static UART_Handle uart0;
static UART_Handle uart3;
static UART_Handle uart4;

/* Función de inicialización */
void App_Init (void)
{

	TimerInit();

	//NVIC_SetPriority(UART4_RX_TX_IRQn, 5);
	//NVIC_SetPriority(UART3_RX_TX_IRQn, 6);
	SIM->CLKDIV2 |= SIM_CLKDIV2_USBDIV(1);
	// UART 0 config
	{
		UART_Config uart_config = {};
		uart_config.baudRate = 9600;
		uart_config.tx = PORTNUM2PIN(PB, 17);
		uart_config.rx = PORTNUM2PIN(PB, 16);
		uart_config.uartNum = 0;
		uart_config.mode = UART_TRANSCEIVER;

		uart0 = UART_Init(&uart_config);
	}

	// Uart 3 Configuration
	{
		UART_Config uart_config = {};
		uart_config.baudRate = 9600;
		uart_config.rx = PORTNUM2PIN(PC, 17);
		uart_config.tx = PORTNUM2PIN(PC, 16);
		uart_config.uartNum = 3;
		uart_config.mode = UART_TRANSCEIVER;

		uart3 = UART_Init(&uart_config);
	}

	// Uart 4 Configuration
	{
		UART_Config uart_config = {};
		uart_config.baudRate = 9600;
		uart_config.rx = PORTNUM2PIN(PE, 25);
		uart_config.tx = PORTNUM2PIN(PE, 24);
		uart_config.uartNum = 4;
		uart_config.mode = UART_TRANSCEIVER;

		uart4 = UART_Init(&uart_config);
	}
}

static char buffer[128] = {};
static uint16_t buffer_offset = 0;
char* view = buffer;

/* Función que se llama constantemente en un ciclo infinito */
void App_Run (void)
{
	static ticks prev = 0;

	if (Now() - prev > MS_TO_TICKS(1000))
	{
		prev = Now();
		UART_WriteString(uart0, "A");
		//UART_PutChar(uart0, 'a');
	}

	if (UART_PollNewData(uart0) > 0)
	{
		char retval;
		uint16_t size;
		bool err;
		UART_GetData(uart0, &retval, &size, &err);
		Sleep(1);
	}

}

/*******************************************************************************
 ******************************************************************************/
