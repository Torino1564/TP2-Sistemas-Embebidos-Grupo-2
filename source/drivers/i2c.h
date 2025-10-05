/*
 * i2c.h
 *
 *  Created on: 4 oct. 2025
 *      Author: plaju
 */

#ifndef DRIVERS_I2C_H_
#define DRIVERS_I2C_H_

#include <stdint.h>
#include "gpio.h"

/*******************************************************************************
 *                                  OBJETOS
 ******************************************************************************/

typedef int8_t i2c_label_t;

typedef enum {
    I2C_OK,
    I2C_ERROR,
    I2C_TIMEOUT
} i2c_status_t;

/*******************************************************************************
 *                                PROTOTIPOS
 ******************************************************************************/

// I2C_Init: inicializa el modulo, setea la frecuencia del bus SCL.
void I2C_Init(uint32_t busFreq, pin_t scl, pin_t sda);

// I2C_Write: recibe el slave que desea escribir, el puntero donde se guardan los bytes que va mandar
// y la cantidad de bytes que se van a mandar
i2c_status_t I2C_Write(uint8_t address, const uint8_t * pData, uint32_t length);

// I2C_Read: recibe el slave de quien va a leer, el puntero donde guardara la data y el largo en bytes del buffer
//que donde se van a recibir los datos.
i2c_status_t I2C_Read(uint8_t address, uint8_t * pData, uint32_t length);



#endif /* DRIVERS_I2C_H_ */
