#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "main.h"

#define KEY_SCAN_PERIOD_MS  50

void initKeyboard(void);
char readKey(void);
char scanKeyboard(void);

extern char lastKey;
extern uint32_t lastScanTime;

#endif
