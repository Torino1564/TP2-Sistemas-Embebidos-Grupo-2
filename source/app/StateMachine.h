/***************************************************************************//**
  @file     StateMachine.h
  @brief    Main state machine definition. All time definitions are in milliseconds
  @author   Group 2
 ******************************************************************************/

#ifndef _STATE_MACHINE_
#define _STATE_MACHINE_

/*******************************************************************************
*                                ENCABEZADOS
******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "drivers/Timer.h"

/*******************************************************************************
*                                ENUMERACIONES
******************************************************************************/
enum States
{
	IDLE,
	PIN,
	OPEN,
	COOLDOWN,
	USER_MENU,
	CHANGE_PIN,
	ADMIN
};

/*******************************************************************************
*                                  OBJETOS
******************************************************************************/
typedef struct
{
	// Estado actual
	void (*pStateFunc)();
	void (*pStateAfterCooldownFunc)();

	// Cooldown
	ticks cooldownStartTime;
	ticks cooldownTicks;

	// Timeout
	uint8_t maxTimeout;
	uint32_t timeoutTicks;

	// Variables
	bool validID;
	bool validPIN;
	uint8_t remainingAttemps;

	// User Menu
	uint8_t menuState;
	void (*pMenuCalledFrom)();

	// Brightness
	uint8_t brightnessLevel;

} StateMachine;

#endif
