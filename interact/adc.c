/*
 * adc.c
 *
 *  Created on: Feb 18, 2018
 *      Author: william
 */

#include <avr/io.h>
#include "adc.h"

#define ADC_LSB ((V_REFERENCE*V_DIV_FACTOR)/ADC_RESOLUTION)
#define ADC_LSB_MV (ADC_LSB * 1000)

uint16_t adc_batt_measure(void){
	//float tmp_f;
	ADMUX |= (1<<REFS0 | 1<<REFS1); //reference = 2.56
	ADCSRA |= (1<<ADEN | 1<<ADSC | 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0);
	while (!(ADCSRA & (1<<ADIF))); //wait for adc completion
	//tmp_f = ADC;
	ADCSRA &= ~(1<<ADEN);
	return (ADC * ADC_LSB_MV);
}
