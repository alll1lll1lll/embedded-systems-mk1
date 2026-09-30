#include "keyboard.h"

static void rowLow(uint8_t pin) {
  if (pin == 10) GPIOA->BRR = (1U << pin);
  else           GPIOB->BRR = (1U << pin);
}

static void rowRelease(uint8_t pin) {
  if (pin == 10) GPIOA->BSRR = (1U << pin);
  else           GPIOB->BSRR = (1U << pin);
}

void initKeyboard(void) {
  GPIOB->MODER = (GPIOB->MODER & ~(3U << (7 * 2)))  | (1U << (7 * 2));
  GPIOB->MODER = (GPIOB->MODER & ~(3U << (6 * 2)))  | (1U << (6 * 2));
  GPIOB->MODER = (GPIOB->MODER & ~(3U << (3 * 2)))  | (1U << (3 * 2));
  GPIOA->MODER = (GPIOA->MODER & ~(3U << (10 * 2))) | (1U << (10 * 2));
  GPIOB->OTYPER |= (1 << 7) | (1 << 6) | (1 << 3);
  GPIOA->OTYPER |= (1 << 10);

  GPIOB->BSRR = (1 << 7) | (1 << 6) | (1 << 3);
  GPIOA->BSRR = (1 << 10);

  GPIOB->MODER &= ~((3U << (10 * 2)) | (3U << (4 * 2)) | (3U << (5 * 2)));
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << (10 * 2))) | (1U << (10 * 2));
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << (4 * 2)))  | (1U << (4 * 2));
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << (5 * 2)))  | (1U << (5 * 2));

  lastKey = '\0';
  lastScanTime = 0;
}

char readKey(void) {
  const uint8_t rows[] = {7, 6, 10, 3};
  const uint8_t cols[] = {10, 4, 5};
  const char keymap[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
  };

  for (uint8_t i = 0; i < 4; i++) {
    rowLow(rows[i]);
    for (volatile int d = 0; d < 100; d++);

    for (uint8_t j = 0; j < 3; j++) {
      if ((GPIOB->IDR & (1U << cols[j])) == 0) {
        rowRelease(rows[i]);
        return keymap[i][j];
      }
    }
    rowRelease(rows[i]);
  }
  return '\0';
}

char scanKeyboard(void) {
  if (tickCount - lastScanTime < KEY_SCAN_PERIOD_MS) {
    return '\0';
  }
  lastScanTime = tickCount;

  char currentKey = readKey();
  char pressed = '\0';

  if (currentKey != '\0' && currentKey != lastKey) {
    pressed = currentKey;
  }
  lastKey = currentKey;
  return pressed;
}
