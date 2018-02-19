/*
 * buttons.h
 *
 *  Created on: Feb 1, 2018
 *      Author: william
 */

#ifndef INTERACT_BUTTONS_H_
#define INTERACT_BUTTONS_H_

#include <stdint.h>
#include <avr/io.h>

#define BTNS_DEBOUNCE_OVFS (8)
#define BTNS_REPEAT_TIMEOUT_OVFS (61) /*~2s delay*/
#define BTNS_REPEAT_OVFS (8) /*~1/4 s*/

#define BTNS_DDR (DDRD)
#define BTNS_PORT (PORTD)
#define BTNS_PIN (PIND)

#define BTN_SEEK (1<<PD5)
#define BTN_SEL (1<<PD3)
#define BTN_INTERRUPT (BTN_SEL)

void buttons_init(uint8_t* mask);
void buttons_v_delegate(void* fn, uint8_t t_repeate);

#endif /* INTERACT_BUTTONS_H_ */
