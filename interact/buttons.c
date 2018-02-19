/*
 * buttons.c
 *
 *  Created on: Feb 1, 2018
 *      Author: william
 */

#include "buttons.h"
#include <avr/interrupt.h>

volatile uint8_t* ext_btns_mask_ptr;
volatile uint8_t ovf_cnt;

void buttons_init(uint8_t* mask)
{
	BTNS_DDR &= ~(BTN_SEEK | BTN_SEL); //pins as input
	BTNS_PORT |= (BTN_SEEK | BTN_SEL); //enable pull-up
	//configure interrupt
	MCUCR |= (1<<ISC11); //on falling edge
	GICR |= (1<<INT1); //INT1
	ext_btns_mask_ptr = mask;
	ovf_cnt = BTNS_DEBOUNCE_OVFS;
	//configure timer 0
	TCCR0 |= (1<<CS02 | 1<<CS00); //div by 1024
	TIMSK |= (1<<TOIE0); //enable OVF interrupt
	sei();
}

ISR(INT1_vect)
{
	uint8_t pState;
	if (!ovf_cnt) //process only when timeout expired
	{
		ovf_cnt = BTNS_DEBOUNCE_OVFS; //reset timeout
		pState = (~BTNS_PIN) & (BTN_SEEK | BTN_SEL);
		if (pState & ~(BTN_INTERRUPT)) pState &= ~(BTN_INTERRUPT);
		*ext_btns_mask_ptr = pState; //write external pressed buttons mask
	}
}

ISR(TIMER0_OVF_vect)
{
	if (ovf_cnt) ovf_cnt--;
}
