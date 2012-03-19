#ifndef __MAIN_H__
#define __MAIN_H__

#include "codes.h"

void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code );
uint8_t read_bits(uint8_t count);
void blast_code(const struct IrCode* code);
void delay_ten_us(uint16_t us);
void quickflashLED( void );

#endif

