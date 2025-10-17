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

static UART_Handle uart3;
static UART_Handle uart4;

/* Función de inicialización */
void App_Init (void)
{
	TimerInit();

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

	// Uart 0 Configuration
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

/* Función que se llama constantemente en un ciclo infinito */
void App_Run (void)
{
	const char* sent = "Testing";
	Sleep(MS_TO_TICKS(100));
	UART_WriteData(uart4, (uint8_t*)sent, strlen(sent));
	//Sleep(MS_TO_TICKS(50));
	uint8_t result = UART_GetChar(uart3);
	if (sent == result)
	{
		Sleep(MS_TO_TICKS(100));
	}
	else
	{
		Sleep(MS_TO_TICKS(100));
	}
}

/*******************************************************************************
 ******************************************************************************/
