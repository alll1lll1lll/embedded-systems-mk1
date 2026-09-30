#ifndef TM1637_H
#define TM1637_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "main.h"

#define TM1637_CLK_PIN  6
#define TM1637_DIO_PIN  7

#define TM1637_CMD1  0x40
#define TM1637_CMD2  0xC0
#define TM1637_CMD3  0x8F

#define SEG_BLANK  0x00
#define SEG_DASH   0x40
#define SEG_O      0x3F
#define SEG_P      0x73
#define SEG_E      0x79
#define SEG_n      0x54
#define SEG_r      0x50
#define SEG_L      0x38

extern const uint8_t digit_codes[];

void tm1637_init(void);
void tm1637_start(void);
void tm1637_stop(void);
void tm1637_write_byte(uint8_t byte);
void tm1637_display_digit(uint8_t digit, uint8_t data);
void tm1637_display_number(int number);
void tm1637_display_segments(const uint8_t seg[4]);
void tm1637_clear(void);
void delay_us(uint32_t us);

#endif
