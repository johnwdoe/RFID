/*
 * cardstore.h
 *
 *  Created on: Feb 6, 2018
 *      Author: william
 */

#ifndef CSTORE_CARDSTORE_H_
#define CSTORE_CARDSTORE_H_

#include <stdint.h>

#define CARDS_STORE_COUNT (40)
//#define CARDS_STORE_COUNT ((E2END+1)/sizeof(card))

#define PROC_CARD_OK (0)
#define PROC_CARD_NOT_FOUND (1)
#define PROC_CARD_EMPTY (2)

typedef struct
{
	uint8_t synT[2];
	uint8_t cardID[5];
	char memo[5];
}card;

uint8_t card_find(card* c, uint8_t* cell);
uint8_t card_get(card* c, uint8_t cell);
uint8_t card_save(card* c, uint8_t cell);
uint8_t card_get_first(card* c, uint8_t* cell);
uint8_t card_get_next(card* c, uint8_t* cell, int8_t direction);

#endif /* CSTORE_CARDSTORE_H_ */
