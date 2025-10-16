/*
 * i2c.c
 *
 *  Created on: 4 oct. 2025
 *      Author: plaju
 */

/*En este apartado vamos a explicar que hace cada registro del perisferico i2c*/

// I2Cx_A1 - I2C address: se guarda el address del slave que va a usar el i2c. El bit cero
// esta reservado, siempre vale 0.
// ESTE REGISTRO SOLO SE MODIFICA SI SE VA A USAR EL MICRO COMO SLAVE. SI LA IDEA ES USAR COMO MAESTRO
// NO MODIFICAR

// I2Cx_F - I2C frequency divider: con este registro se setea el baudrate e implicitamente los tiempos de hold.
// Tiene dos partes: del bit 0 al 5 es el ICR y del bit 6 al 7 es el MUL
// El bitrate se calcula como: freq_bus/(ICR * MUL), en la K64F la frecuencia del bus es 60MHz

// I2Cx_C1 - I2C control: controla la habilitacion del modulo y su comportamiento basico
// IICEN - bit 7: debe ponerse en 1 para habilitar el modulo
// IICIE - bit 6: en 1 su se trabaja con interrupciones
// MST -   bit 5: 1 para establecer en master (ademas genera un start en el bus). 0 para establecer en slave (ademas
//                  genera un stop en el bus)
// TX -    bit 4: 1 si vas a enviar datos, 0 si vas a leer datos
// TXAK -  bit 3: 0 para mandar ACK, 1 para mandar NACK
// RSTA -  bit 2: (W)1 para generar un repeated start
// WUEN -  bit 1: 1 habilita el wakeup cuando coincide el address del slave, 0 normal
// DMAEN-  bit 0: 0 disable, 1 para usar DMA (la memoria)

// I2Cx_S - I2C status: son flags de los estados del modulo. Es de 8bits
// TCF -   bit 7: (R)se pone en 1 cuando se termina la transmision (1 byte)
// IAAS -  bit 6: 1 si el modulo fue direccionado como slave
// BUSY -  bit 5: (R)1 si el bus esta ocupado, 0 bus is idle
// ARBL -  bit 4: (R)1 si se perdio la condicion de master por colision
// RAM -   bit 3: NO ENTENDI-NO LO USAMOS CREO
// SRW -   bit 2: 0 si el master quiere leer del slave o 1 si quiere escribir al slave
// IICIF - bit 1: es un flag que se pone en 1 cuando se completa una transferencia(byte recibido o enviado). Es w1c
// RXAK -  bit 0: despues de una transferencia de info, interpreta el ACK o NACK del slave. Este flag se pone
//                en 0 si el slave mando ACK, pone un 1 si el slave mando NACK.

// I2Cx_D - I2C data I/O: es para escribir o leer los datos del modulo i2c
// DATA -  los 8 bits: para escribir se modifica este registro y se guarda en el shift register del modulo.
//         en el caso de leer, devuelve el ultimo byte recibido por el SDA. La lectura se suele hacer
//         luego del flag IICIF del registro I2Cx_S.

// I2Cx_C2 - I2C control 2: es un registro de configuracion avanzada. Complementario a I2Cx_C1
//           se usa tanto en modo master o esclavo.
// GCAEN - bit 7: en modo esclavo: habilita la respuesta al address 0x00
// ADEXT - bit 6: habilita el address extendido de 10 bits. En 0 solo acepta 7bits.
// HDRS -  bit 5: habilita modo de alta corriente en SDA y SCL. Esto permite mejor forma ante situaciones
//                de muchas cargas o largos buses
// SBRC -  bit 4: en 0 el slave sigue el baudrate del master y clock stretching puede ocurrir. En 1
//                el slave baud rate es independiente del baudrate del master.
// RMEN -  bit 3: habilita la deteccion de addresses en un rango impuesto en I2C_RA
// AD - bit 2, 1 y 0: cuando ADEXT esta en 1, este guarda los 3 bits mas significativos del address extendido

// I2Cx_FLT: es un registro para filtrar flancos pertenecientes a ruido y no datos

// I2Cx_RA - I2C range address: cuando el micro es slave, permite que responda a un rango de addresses
// bit del 1 al 7: es el address superior del rango de addresses. El rango inferior se setea en I2Cx_A1.
//                 este solo es valido si esta configurado el RMEN del I2Cx_C2.

// I2Cx_SMB - SMBus control and status: es para configuracion avanzada de tiempo y voltaje

// I2Cx_A2 - I2C address 2: permite al micro funcionar como slave y responder a una segunda address
// addres del bit 1 al 7: no se que ocurre con este registro en el caso del RMEN en 1

// I2Cx_SLTH - I2C SCL low timeout register high
// I2Cx_SLTL - I2C SCL low timeout register low
// sirven para definir el contador que mide el tiempo que el SCL permanece en low o 0. Esto permite detectar
// timeouts. Ni idea si es necesario utilizarlo.

/*******************************************************************************
 *                                INCLUDES
 ******************************************************************************/
#include "i2c.h"

/*******************************************************************************
 *                               DEFINICIONES
 ******************************************************************************/
#define MAX_CPU_FREQ 60000000 //60MHz de la kinetis?
#define MAX_I2C_ICR 64

#define MAX_I2C_INSTANCES 3 // este es el maximo por hardware, es decir, la kinetis tiene solo 3

#define NULL 0

/*******************************************************************************
 *                                ESTRUCTURAS
 ******************************************************************************/

typedef struct {
	pin_t scl;
	pin_t sda;
	uint16_t baudRate;
	uint8_t mul:2; // 0b00 es 1, 0b01 es 2, 0b11 es 4
	uint8_t icr:6; // de 1 a 64
} i2c_t;

/*******************************************************************************
 *                                VARIABLES
 ******************************************************************************/

static i2c_t* pInstances[MAX_I2C_INSTANCES] = {};

/*******************************************************************************
 *                                FUNCIONES
 ******************************************************************************/

i2c_label_t I2C_Init(uint32_t busFreq, pin_t scl, pin_t sda)
{
	int8_t i2cNum = -1;

	if((scl == PORTNUM2PIN(PE,24) && sda == PORTNUM2PIN(PE,25)) ||
	   (scl == PORTNUM2PIN(PB,0) && sda == PORTNUM2PIN(PB,1)) ||
	   (scl == PORTNUM2PIN(PB,2) && sda == PORTNUM2PIN(PB,3)) ||
	   (scl == PORTNUM2PIN(PD,2) && sda == PORTNUM2PIN(PD,3)))
	{
		i2cNum = 0;
	}
	else if((scl == PORTNUM2PIN(PE,0) && sda == PORTNUM2PIN(PE,1)) ||
			(scl == PORTNUM2PIN(PC,10) && sda == PORTNUM2PIN(PC,11)))
	{
		i2cNum = 1;
	}
	else if((scl == PORTNUM2PIN(PA,12) || (scl == PORTNUM2PIN(PA,14))) && sda == PORTNUM2PIN(PA,13))
	{
		i2cNum = 2;
	}
	else
	{
		return i2cNum;
	}




	for(uint8_t i = 0 ; i < MAX_I2C_INSTANCES ; i++)
	{
		if(pInstances[i] != NULL)
		{
			continue;
		}
		i2c_t * pNew = (i2c_t *)malloc(sizeof(i2c_t));
		switch()
		{
		case 0:

			pNew->scl = ;
			pNew->sda = ;

			break;
		case 1:

			break;

		case 2:

			break;

		default:

			break;
		}

	}
	if(busFreq >= MAX_CPU_FREQ/MAX_I2C_ICR)
	{

	}
	else if(busFreq >= MAX_CPU_FREQ/(2*MAX_I2C_ICR))
	{

	}
	else if(busFreq >= MAX_CPU_FREQ/(4*MAX_I2C_ICR))
	{

	}
	else
	{
		mul = 0b11;
		icr = MAX_I2C_ICR;
	}


}



