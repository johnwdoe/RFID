/*
 * menu.c
 *
 *  Created on: Feb 1, 2018
 *      Author: william
 */

#include "menu.h"

#include "../lcd_5110/nokia5110.h"
#include <stdio.h>

mnuItem mnuItems[MAX_MENU_ITEMS];
mnuCtx menu_cache = {.items = mnuItems};

void mnu_forward(void)
{
	//activate next menu item
	mnu_item_activate((menu_cache.active_item+1 == menu_cache.items_count) ? 0 : menu_cache.active_item+1);
}


void mnu_select(void)
{
	void(*f)(void) = menu_cache.items[menu_cache.active_item].evt;
	if (f == NULL) return; //do nothing
	f();
}

void mnu_screen_reset(void)
{
	menu_cache.items_count = 0;
	menu_cache.active_item = 0;
}

void mnu_items_add_p(const char* text, const MnuEvent* evtptrs)
{
	char c;
	//add char
	while ((c=pgm_read_byte(text++)) != 0x00)
	{
		if (c < ' ' && c != '\n')
		{
			//add to context
			menu_cache.items[menu_cache.items_count].evt = pgm_read_ptr(&evtptrs[menu_cache.items_count]);
			menu_cache.items[menu_cache.items_count].icon = c;
			nokia_lcd_get_cursor(&menu_cache.items[menu_cache.items_count].x, &menu_cache.items[menu_cache.items_count].y);
			if (menu_cache.items_count == menu_cache.active_item) c++;
			menu_cache.items_count++;
		}
		nokia_lcd_write_char(c);
	}
}

void mnu_item_move(uint8_t index, int8_t dx, int8_t dy)
{
	if (index >= menu_cache.items_count) return; //out of array
	//print space on current position
	nokia_lcd_set_cursor(menu_cache.items[index].x, menu_cache.items[index].y);
	nokia_lcd_write_char(' ');
	//write sign on new position
	menu_cache.items[index].x += dx;
	menu_cache.items[index].y += dy;
	nokia_lcd_set_cursor(menu_cache.items[index].x, menu_cache.items[index].y);
	nokia_lcd_write_char(menu_cache.items[index].icon);
	nokia_lcd_render();
}

void mnu_item_activate(uint8_t index)
{
	//inactivate current item
	nokia_lcd_set_cursor(menu_cache.items[menu_cache.active_item].x, menu_cache.items[menu_cache.active_item].y);
	nokia_lcd_write_char(menu_cache.items[menu_cache.active_item].icon);
	menu_cache.active_item = index;
	nokia_lcd_set_cursor(menu_cache.items[menu_cache.active_item].x, menu_cache.items[menu_cache.active_item].y);
	nokia_lcd_write_char(menu_cache.items[menu_cache.active_item].icon+1);
	nokia_lcd_render();
}

