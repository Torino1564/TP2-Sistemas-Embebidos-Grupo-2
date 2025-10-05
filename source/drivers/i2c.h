/*
 * i2c.h
 *
 *  Created on: 4 oct. 2025
 *      Author: plaju
 */

#ifndef DRIVERS_I2C_H_
#define DRIVERS_I2C_H_

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

#endif /* DRIVERS_I2C_H_ */
