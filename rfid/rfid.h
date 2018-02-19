/*
 * rfid.h
 *
 *  Created on: May 13, 2013
 *      Author: william
 *
 */

#ifndef RFID_H_
#define RFID_H_

#include <stdint.h>

#define RFID_MAIN_FREQ (125000)

#define RFID_RET_OK (0x00)
#define RFID_RET_ERR (0xFF)
#define RFID_RET_ABORTED (0xFF)


/*
 * *data - ссылка на начало массива;
 * *ctl - ссылка на переменную флагов управления;
 * abortmask - маска останова (как только (*ctl & stopmask == stopmask) процедура завершается)
 * */
void rfid_ioinit(void); //initialize IO directions
uint8_t rfid_read(uint8_t *data, volatile uint8_t *ctl, uint8_t abortmask); //start reading card
void rfid_transmit(uint8_t *data, volatile uint8_t *ctl, uint8_t abortmask); //start transmitting card

#endif /* RFID_H_ */
