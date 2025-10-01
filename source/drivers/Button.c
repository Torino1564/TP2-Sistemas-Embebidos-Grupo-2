/*
 * Button.c
 *
 *  Created on: Sep 3, 2025
 *      Author: jtori
 */
#include "Button.h"
#include <stdlib.h>
#include <string.h>
#include "Timer.h"

#define START_MAX_BUTTON_COUNT (int)8;

static uint16_t buttonCounter = 0;
static uint16_t maxButtonCount = 0;
static bool semaphore = 0;

typedef struct
{
	pin_t pin;
	uint8_t buttonId;
	uint8_t inputMode;
	uint8_t isrType;

	uint8_t state;
	bool voltageState;
	ticks holdTickInterval;
	ticks ticksPicture;
	ticks debouncingTickInterval;
	service_id debouncingIsrId;
} Button;

static Button* buttonArray;

void ButtonISR(void* user_data)
{
	Button* pButton = (Button*)(user_data);

	if (semaphore)
		return;

	if((pButton->voltageState && pButton->inputMode == INPUT_PULLDOWN) || (!pButton->voltageState && pButton->inputMode == INPUT_PULLUP))
	{
		pButton->ticksPicture = Now();
		pButton->voltageState = !pButton->voltageState;
	}
	else
	{
		ticks now = Now();
		if(now - pButton->ticksPicture >= pButton->holdTickInterval)
		{
			pButton->state = BUTTON_HELD;

		}
		else
		{
			pButton->state = BUTTON_PRESSED;

		}
		pButton->voltageState = pButton->inputMode == INPUT_PULLDOWN;
	}

	gpioSetupISR(pButton->pin, NO_INT, &ButtonISR, pButton);
	TimerSetEnable(pButton->debouncingIsrId, true);
}

void DebouncingISR(void* user_data)
{
	if (semaphore)
		return;


	Button* pButton = (Button*)(user_data);
	gpioSetupISR(pButton->pin, pButton->isrType, &ButtonISR, pButton);
	TimerSetEnable(pButton->debouncingIsrId,false);
}

uint16_t NewButton(pin_t pin, bool activeHigh)
{
	if (buttonCounter + 1 >= maxButtonCount)
	{
		semaphore = 1;
		maxButtonCount += START_MAX_BUTTON_COUNT;
		Button* tempArray = (Button*)calloc(maxButtonCount, sizeof(Button));
		memcpy(buttonArray, tempArray, buttonCounter * sizeof(Button));
		free(buttonArray);
		buttonArray = tempArray;
		for (int i = 0; i < buttonCounter; i++)
		{
			gpioSetupISR(buttonArray[i].pin, buttonArray[i].isrType, &ButtonISR, &buttonArray[i]);
			TimerSetUserData(buttonArray[i].debouncingIsrId, &buttonArray[i]);
			TimerSetEnable(buttonArray[i].debouncingIsrId, 0);
		}
		semaphore = 0;
	}
	const uint16_t buttonId = buttonCounter;
	Button* pButton = &buttonArray[buttonCounter++];

	pButton->pin = pin;
	pButton->inputMode = activeHigh ? INPUT_PULLDOWN : INPUT_PULLUP;
	pButton->isrType = activeHigh ? FLAG_INT_POSEDGE : FLAG_INT_NEGEDGE;
	pButton->voltageState = activeHigh;
	pButton->debouncingTickInterval = 0;
	pButton->debouncingIsrId = 0;
	pButton->state = BUTTON_IDLE;
	pButton->debouncingIsrId = TimerRegisterPeriodicInterruption(&DebouncingISR, MS_TO_TICKS(0.25), pButton);
	pButton->holdTickInterval = MS_TO_TICKS(1000);
	TimerSetEnable(pButton->debouncingIsrId, false);

	gpioMode(pButton->pin, pButton->inputMode);
	//gpioSetSlewRate(pButton->pin, 1); // 1 es low slew rate
	gpioSetupISR(pButton->pin, pButton->isrType, &ButtonISR, &buttonArray[buttonId]);

	return buttonId;
}

bool readButtonStatus(uint16_t buttonId)
{
	return (bool)buttonArray[buttonId].state;
}

uint8_t readButtonData(uint16_t buttonId)
{
	uint8_t temp = buttonArray[buttonId].state;
	buttonArray[buttonId].state= BUTTON_IDLE;
	return temp;
}
