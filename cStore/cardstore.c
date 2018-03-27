/*
 * cardstore.c
 *
 *  Created on: Feb 6, 2018
 *      Author: william
 */

#include "cardstore.h"
#include <avr/eeprom.h>
#include <string.h>

/*EEMEM uint8_t cards[] = {0x11, 0x33, 0x55, 0x33, 0xde, 0x3f, 0x11, 'c', 'a', 'r', 'd', '1',
		0x11, 0x33, 0x55, 0xf3, 0xde, 0x3f, 0x11, 'c', 'a', 'r', 'd', '2'};
*/

uint8_t card_find(card* c, uint8_t* cell)
{
	uint8_t cell_i = 0;//*cell;
	uint8_t tmpbuf[5];
	while (cell_i < CARDS_STORE_COUNT)
	{
		eeprom_read_block((void*)tmpbuf, (void*)(cell_i*sizeof(card)+2), 5);
		if (memcmp((void*)c+2, (void*)tmpbuf, 5) == 0){
			*cell = cell_i; //return cell number
			eeprom_read_block((void*)c->memo, (void*)(cell_i*sizeof(card)+7), 5);
			return PROC_CARD_OK;
		}
		cell_i++;
	}

	return PROC_CARD_NOT_FOUND;
}

uint8_t card_get(card* c, uint8_t cell)
{
	uint16_t tmp;
	eeprom_read_block((void*)&tmp, (void*)(cell*sizeof(card)), 2);
	if (tmp != 0xFFFF)
	{
		eeprom_read_block((void*)c, (void*)(cell*sizeof(card)),sizeof(card));
		return PROC_CARD_OK;
	}
	return PROC_CARD_EMPTY;
}

/*
 * return first non-empty card slot and it's cell number
 */
uint8_t card_get_first(card* c, uint8_t* cell)
{
	uint8_t cell_i = CARDS_STORE_COUNT-1; //last cell number
	if (card_get_next(c, &cell_i, 1) == PROC_CARD_OK)
	{
		*cell = cell_i; //return cell
		return PROC_CARD_OK;
	}
	return PROC_CARD_NOT_FOUND;
}

uint8_t card_get_next(card* c, uint8_t* cell, int8_t direction)
{
	uint8_t cell_i = *cell; //start position
	uint8_t cuCount = CARDS_STORE_COUNT;
	uint16_t tmp;
	while (cuCount--)
	{
		cell_i += direction; //move cell
		//fix cell pointer
		if (cell_i == CARDS_STORE_COUNT) cell_i = 0;
		else if (cell_i == 0xFF) cell_i = CARDS_STORE_COUNT-1;
		//read synT value
		eeprom_read_block((void*)&tmp, (void*)(cell_i*sizeof(card)), 2);
		if (tmp != 0xFFFF)
		{
			//card found
			eeprom_read_block((void*)c, (void*)(cell_i*sizeof(card)), sizeof(card));
			*cell = cell_i;
			return PROC_CARD_OK;
		}
	}
	return PROC_CARD_NOT_FOUND; //possible only when no saved cards exists
}

uint8_t card_save(card* c, uint8_t cell)
{
	//store card into [cell]
	eeprom_write_block((void*)c, (void*)(cell*sizeof(card)), sizeof(card));
	return PROC_CARD_OK;
}
