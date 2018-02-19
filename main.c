/*
 * main.c
 *
 *  Created on: Jan 31, 2018
 *      Author: ikhatckevich
 */
#include <avr/io.h>
#include "lcd_5110/nokia5110.h"
#include <util/delay.h>
#include <stdio.h>
#include "interact/menu.h"
#include "interact/buttons.h"
#include "rfid/rfid.h"
#include <avr/pgmspace.h>
#include <string.h>
#include <stdio.h>
#include "cStore/cardstore.h"
#include <avr/sleep.h>
#include "interact/adc.h"


void m_Read(void);
void m_Transmit(void);
void m_Info(void);
void InfoVoltageRefresh(void);
void m_Main(void);
void m_ReadComplete(void);
void m_SelectCell(void);
void m_SelectCellInc(void);
void m_SelectCellDec(void);
void SelectCellRefresh(void);
void m_EnterName(void);
void EnterNameRefresh(void);
void m_EnterNameNextPos(void);
void m_EnterNamePrevPos(void);
void m_EnterNameSelSymbolNext(void);
void m_EnterNameSelSymbolPrev(void);
void m_Save(void); //final save method!
void m_TransmitSelectCellInc(void);
void m_TransmitSelectCellDec(void);
void m_Transmitting(void);
void TransmitCardRefresh(void);

volatile uint8_t btns_pressed;
const PROGMEM MnuEvent MainMenu_Events[] = {m_Read, m_Transmit, m_Info};
const PROGMEM MnuEvent ReadingMenu_Events[] = {m_Main};
const PROGMEM MnuEvent TransmittingMenu_Events[] = {m_Transmit};
const PROGMEM MnuEvent ReadCompleteMenu_Events[] = {m_SelectCell, m_Main};
const PROGMEM MnuEvent SelectCellMenu_Events[] = {m_SelectCellInc, m_SelectCellDec, m_EnterName, m_Main};
const PROGMEM MnuEvent EnterNameMenu_Events[] = {m_EnterNameSelSymbolNext, m_EnterNameSelSymbolPrev, m_EnterNameNextPos, m_EnterNamePrevPos, m_Save, m_Main};
const PROGMEM MnuEvent SelectCellForTransmit_Events[] = {m_TransmitSelectCellInc, m_TransmitSelectCellDec, m_Transmitting, m_Main};

card rCard, wCard;
char tstr[15]; //temp
char t_name[5];
uint8_t t_name_pos = 0;
uint8_t t_cell;

void m_Read(void)
{
	mnu_screen_reset();
	nokia_lcd_clear();
	mnu_items_add_p(PSTR("Reading...\n\n\n\n\n\x1a Exit"), ReadingMenu_Events);
	nokia_lcd_render();
	//_delay_ms(200);
	if(rfid_read((uint8_t*)&rCard, &btns_pressed, BTN_SEL) == RFID_RET_OK) m_ReadComplete();
}

void m_ReadComplete(void)
{
	nokia_lcd_clear();
	mnu_screen_reset();
	mnu_items_add_p(PSTR("Card data:\nSynT: \nMan.ID: \nID: \n\n\x1a Save  \x1a Exit"),ReadCompleteMenu_Events);
	//sprintf_P(tstr, PSTR("%02x%02x"), rCard.synT[0], rCard.synT[1]);
	//nokia_lcd_write_string_at(tstr, 36, 8);
	nokia_lcd_set_cursor(36, 8);
	nokia_lcd_write_hex(rCard.synT, 2);
	//sprintf_P(tstr, PSTR("%02x"), rCard.cardID[0]);
	//nokia_lcd_write_string_at(tstr, 48, 16);
	nokia_lcd_set_cursor(48, 16);
	nokia_lcd_write_hex(rCard.cardID, 1);
	//sprintf_P(tstr, PSTR("%02x%02x%02x%02x"), rCard.cardID[1], rCard.cardID[2], rCard.cardID[3], rCard.cardID[4]);
	//nokia_lcd_write_string_at(tstr, 24, 24);
	nokia_lcd_set_cursor(24, 24);
	nokia_lcd_write_hex(rCard.cardID+1, 4);
	t_cell = 0;
	if (card_find(&rCard, &t_cell) == PROC_CARD_OK)
	{
		//already saved
		sprintf_P(tstr, PSTR("Name:      "));
		memcpy(tstr+6, rCard.memo, 5);
		//rCard
	}
	else
	{
		//not in store
		sprintf_P(tstr, PSTR("<Not saved>"));
	}
	nokia_lcd_write_string_at(tstr, 0, 32);
	nokia_lcd_render();
}

void m_Transmit(void)
{
	nokia_lcd_clear();
	mnu_screen_reset();
	if (card_get(&wCard, t_cell) == PROC_CARD_EMPTY && card_get_first(&wCard, &t_cell) == PROC_CARD_NOT_FOUND)
	{
		mnu_items_add_p(PSTR("Card store is empty.\nNothing to\nselect.\n\n\x1a Exit"), ReadingMenu_Events);
		nokia_lcd_render();
	}
	else
	{
		mnu_items_add_p(PSTR("Select card:\n \x1c\n[  ]: \n \x1e\nID: \n\x1a Start \x1a Exit"), SelectCellForTransmit_Events);
		TransmitCardRefresh();
	}
}

/*
 * show wcard in tcell
 */
void TransmitCardRefresh(void)
{
	sprintf_P(tstr, PSTR("%02u"), t_cell);
	nokia_lcd_write_string_at(tstr, 6, 16);
	sprintf_P(tstr, PSTR("     "));
	memcpy(tstr, wCard.memo, 5);
	nokia_lcd_write_string_at(tstr, 36, 16);
	//sprintf_P(tstr, PSTR("%02x%02x%02x%02x%02x"), wCard.cardID[0], wCard.cardID[1], wCard.cardID[2], wCard.cardID[3], wCard.cardID[4]);
	//nokia_lcd_write_string_at(tstr, 24, 32);
	nokia_lcd_set_cursor(24, 32);
	nokia_lcd_write_hex(wCard.cardID, 5);
	nokia_lcd_render();
}

void m_TransmitSelectCellInc(void)
{
	card_get_next(&wCard, &t_cell, 1);
	TransmitCardRefresh();
}

void m_TransmitSelectCellDec(void)
{
	card_get_next(&wCard, &t_cell, -1);
	TransmitCardRefresh();
}

void m_Transmitting(void)
{
	nokia_lcd_clear();
	mnu_screen_reset();
	mnu_items_add_p(PSTR("Transmit...\n\n\n\n\n\x1a Back"), TransmittingMenu_Events);
	nokia_lcd_render();
	rfid_transmit((uint8_t*)&wCard, &btns_pressed, BTN_SEL);
}


void m_Info(void)
{
	card t_card;
	uint8_t t_cell = CARDS_STORE_COUNT, t_count = 0;

	//calc cards count
	while(t_cell--)
	{
		if (card_get(&t_card, t_cell) == PROC_CARD_OK) t_count++;
	}

	sprintf_P(tstr, PSTR("%02u/%02u"), t_count, CARDS_STORE_COUNT);

	nokia_lcd_clear();
	mnu_screen_reset();
	//sprintf_P(tstr, PSTR("%04umV"), adc_batt_measure());
	mnu_items_add_p(PSTR("Info:\nBatt.:\nStorage:\n\n\n\x1a Exit"), ReadingMenu_Events);
	nokia_lcd_write_string_at(tstr, 54, 16);
	nokia_lcd_render();
	buttons_v_delegate(InfoVoltageRefresh, 32);
}

void InfoVoltageRefresh(void)
{
	sprintf_P(tstr, PSTR("%04umV"), adc_batt_measure());
	nokia_lcd_write_string_at(tstr, 42, 8);
	nokia_lcd_render();
}

void m_SelectCell(void)
{
	//t_cell = 0;
	nokia_lcd_clear();
	mnu_screen_reset();
	mnu_items_add_p(PSTR("Select cell.\nID: \n \x1c\n[  ]\n \x1e\n\x1a Save \x1a Exit"), SelectCellMenu_Events);
	SelectCellRefresh();
}

void SelectCellRefresh(void)
{
	//show id
	//sprintf_P(tstr, PSTR("%02x%02x%02x%02x%02x"), rCard.cardID[0],rCard.cardID[1],rCard.cardID[2],rCard.cardID[3],rCard.cardID[4]);
	//nokia_lcd_write_string_at(tstr, 24, 8);
	nokia_lcd_set_cursor(24, 8);
	nokia_lcd_write_hex(rCard.cardID, 5);
	//show tcell
	sprintf_P(tstr, PSTR("%02u"), t_cell);
	nokia_lcd_write_string_at(tstr, 6, 24);
	//try read card on cell-position
	if (card_get(&wCard, t_cell) == PROC_CARD_EMPTY)
	{
		sprintf_P(tstr, PSTR("<Empty>"));
		sprintf_P(t_name, PSTR("     "));
	}
	else
	{
		sprintf_P(tstr, PSTR("[     ]"));
		memcpy(tstr+1, wCard.memo, 5);
		memcpy(t_name, wCard.memo, 5);
	}
	nokia_lcd_write_string_at(tstr, 30, 24);
	nokia_lcd_render();
}

void m_SelectCellInc(void)
{
	t_cell ++;
	if (t_cell == CARDS_STORE_COUNT) t_cell = 0;
	SelectCellRefresh();
}

void m_SelectCellDec(void)
{
	if (t_cell == 0) t_cell = CARDS_STORE_COUNT;
	t_cell--;
	SelectCellRefresh();
}

void m_EnterName(void)
{
	//sprintf_P(t_name,PSTR("     "));
	t_name_pos = 0;
	//init static data
	nokia_lcd_clear();
	mnu_screen_reset();
	mnu_items_add_p(PSTR("Enter name\nID:\n  \x1c\n\n  \x1e"), EnterNameMenu_Events);
	nokia_lcd_set_cursor(48, 24);
	mnu_items_add_p(PSTR("\x1a"), EnterNameMenu_Events);
	nokia_lcd_set_cursor(0, 24);
	mnu_items_add_p(PSTR("\x18[     ]\n\n\x1a Save \x1a Exit"), EnterNameMenu_Events);
	//show card ID
	//sprintf_P(tstr, PSTR("%02x%02x%02x%02x%02x"), rCard.cardID[0],rCard.cardID[1],rCard.cardID[2],rCard.cardID[3],rCard.cardID[4]);
	//nokia_lcd_write_string_at(tstr, 24, 8);
	nokia_lcd_set_cursor(24, 8);
	nokia_lcd_write_hex(rCard.cardID, 5);
	EnterNameRefresh();
}

void EnterNameRefresh(void)
{
	//sprintf_P(tstr,PSTR("     "));
	tstr[5] = '\0';
	memcpy(tstr, t_name, 5);
	nokia_lcd_write_string_at(tstr, 12, 24);
	nokia_lcd_render();
}
void m_EnterNameNextPos(void)
{
	if (t_name_pos == 4) return;
	mnu_item_move(0, 6, 0);
	mnu_item_move(1, 6, 0);
	t_name_pos++;
	mnu_item_activate(0);
}
void m_EnterNamePrevPos(void){
	if (t_name_pos == 0) return;
	mnu_item_move(0, -6, 0);
	mnu_item_move(1, -6, 0);
	t_name_pos--;
}
void m_EnterNameSelSymbolNext(void)
{
	if(t_name[t_name_pos] == ' ') t_name[t_name_pos] = 'a';
	else if (t_name[t_name_pos] == 'z') t_name[t_name_pos] = ' ';
	else t_name[t_name_pos]++;
	EnterNameRefresh();
}
void m_EnterNameSelSymbolPrev(void)
{
	if(t_name[t_name_pos] == ' ') t_name[t_name_pos] = 'z';
	else if (t_name[t_name_pos] == 'a') t_name[t_name_pos] = ' ';
	else t_name[t_name_pos]--;
	EnterNameRefresh();
}

void m_Save(void)
{
	//save rCard with name = t_name in t_cell
	memcpy((void*)rCard.memo, (void*)t_name, 5); //copy name
	card_save(&rCard, t_cell);
	m_Main();
}


void m_Main(void)
{
	buttons_v_delegate(NULL, 0xFF);
	nokia_lcd_clear();
	mnu_screen_reset();
	mnu_items_add_p(PSTR("Main menu:\n\n\x1a Read\n\x1a Transmit\n\x1a Info"), MainMenu_Events);
	nokia_lcd_render();
}

int main(void)
{
	buttons_init((uint8_t*)&btns_pressed);
	rfid_ioinit();
	nokia_lcd_init();//initialize I/O
	nokia_lcd_clear();
	nokia_lcd_power(1); //power on display

	m_Main(); //show main menu

	while(1){
		//buttons events
		if(btns_pressed&BTN_SEEK)
		{
			//_delay_ms(100);
			btns_pressed &= ~BTN_SEEK;
			mnu_forward();
		}
		if(btns_pressed&BTN_SEL)
		{
			//_delay_ms(100);
			btns_pressed &= ~BTN_SEL;
			mnu_select();
		}
	}
	return 0;
}
