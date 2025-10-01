#include "SUART.h"

/*
 * This driver keeps track of its objects and is responsible for their lifetime.
 * The init functions return handles to Instaciated objects
 */

#define MAX_SUART_INSTANCES 5

typedef struct {
	pin_t tx;
	pin_t rx;
	uint16_t baudRate;
	uint8_t numDataBits;
	bool parity;
} SUART;

static SUART* pInstances[MAX_SUART_INSTANCES] = {};

SUART_Handle SUART_InitEx (pin_t rx, pin_t tx, uint16_t baudRate, uint8_t numDataBits, bool parity)
{
	for (uint8_t i = 0; i < MAX_SUART_INSTANCES; i++)
	{
		if (instances[i] != NULL)
			continue;
		SUART* pNew = (SUART*)malloc(sizeof(SUART));
		pNew->rx = rx;
		pNew->tx = tx;
		pNew->baudRate = baudRate;
		pNew->numDataBits = numDataBits;
		pNew->parity = parity;

		return (int16_t)i;
	}
	return -1;
}

SUART_Handle SUART_Init (pin_t rx, pin_t tx, uint16_t baudRate)
{
	return SUART_InitEx(rx, tx, baudRate, 8, FALSE);
}

void SUART_Delete(SUART_Handle handle)
{
	if (instances[handle])
	{
		free(instances[handle]);
		instances[handle] = NULL;
	}
}
