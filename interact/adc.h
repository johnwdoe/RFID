/*
 * adc.h
 *
 *  Created on: Feb 18, 2018
 *      Author: william
 */

#ifndef INTERACT_ADC_H_
#define INTERACT_ADC_H_
#include <stdint.h>

#define V_DIV_FACTOR (2)
#define ADC_RESOLUTION (1024)
#define V_REFERENCE (2.56)

uint16_t adc_batt_measure(void);


#endif /* INTERACT_ADC_H_ */
