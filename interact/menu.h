/*
 * menu.h
 *
 *  Created on: Feb 1, 2018
 *      Author: william
 */

#ifndef INTERACT_MENU_H_
#define INTERACT_MENU_H_

#define MAX_MENU_ITEMS	(6)
#include <stdint.h>
#include <avr/pgmspace.h>

typedef struct
{
	uint8_t x;
	uint8_t y;
	char icon;
	void* evt;
}mnuItem;

typedef struct
{
	uint8_t items_count;
	uint8_t active_item;
	mnuItem* items;
}mnuCtx;

typedef void (*MnuEvent)(void);

void mnu_forward(void);
void mnu_select(void);
void mnu_screen_reset(void);
void mnu_items_add_p(const char* text, const MnuEvent* evtptrs);
void mnu_item_move(uint8_t index, int8_t dx, int8_t dy);
void mnu_item_activate(uint8_t index);

#endif /* INTERACT_MENU_H_ */
