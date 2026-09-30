#include "main.h"
#include "tm1637.h"
#include "keyboard.h"

#define CODE_LEN        4
#define MAX_ATTEMPTS    3
#define RESULT_TIME_MS  3000
#define LOCK_TIME_MS    10000
#define BLINK_MS        250

static const char SECRET_CODE[CODE_LEN] = {'1', '2', '3', '4'};

typedef enum {
  ST_INPUT,
  ST_GRANTED,
  ST_DENIED,
  ST_LOCKED
} State;

volatile uint32_t tickCount;
char lastKey;
uint32_t lastScanTime;

static State    state = ST_INPUT;
static char     entered[CODE_LEN];
static uint8_t  enteredLen = 0;
static uint8_t  failCount = 0;
static uint32_t stateStart = 0;
static uint32_t lastBlink = 0;
static uint8_t  lastShownSec = 0xFF;

void osSystickHandler(void) {
  tickCount++;
}

void initGPIO(void) {
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;

  const uint8_t leds[] = {LED_INPUT_PIN, LED_GREEN_PIN, LED_RED_PIN};
  for (int i = 0; i < 3; i++) {
    uint8_t p = leds[i];
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (p * 2))) | (1U << (p * 2));
    GPIOA->OTYPER &= ~(1U << p);
  }
  GPIOA->BRR = (1U << LED_INPUT_PIN) | (1U << LED_GREEN_PIN) | (1U << LED_RED_PIN);
}

void initUSART2(void) {
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  GPIOA->MODER = (GPIOA->MODER & ~(0xF << 4)) | (0xA << 4);
  GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFF << 8)) | (1 << 8) | (1 << 12);
  USART2->BRR = 417;
  USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void initSysTick(void) {
  SysTick->LOAD = 47999;
  SysTick->VAL = 0;
  SysTick->CTRL = (1 << 2) | (1 << 1) | (1 << 0);
}

int _write(int file, uint8_t *ptr, int len) {
  (void)file;
  for (int i = 0; i < len; i++) {
    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = ptr[i];
  }
  return len;
}

static void ledOn(uint8_t pin)     { GPIOA->BSRR = (1U << pin); }
static void ledOff(uint8_t pin)    { GPIOA->BRR  = (1U << pin); }
static void ledToggle(uint8_t pin) { GPIOA->ODR ^= (1U << pin); }

static void showInput(void) {
  uint8_t seg[4];
  for (int i = 0; i < 4; i++) {
    seg[i] = (i < enteredLen) ? digit_codes[entered[i] - '0'] : SEG_DASH;
  }
  tm1637_display_segments(seg);
}

static void showOpen(void) {
  const uint8_t seg[4] = {SEG_O, SEG_P, SEG_E, SEG_n};
  tm1637_display_segments(seg);
}

static void showError(void) {
  const uint8_t seg[4] = {SEG_E, SEG_r, SEG_r, SEG_BLANK};
  tm1637_display_segments(seg);
}

static void showLockTimer(uint8_t sec) {
  uint8_t seg[4];
  seg[0] = SEG_L;
  seg[1] = SEG_BLANK;
  seg[2] = (sec >= 10) ? digit_codes[(sec / 10) % 10] : SEG_BLANK;
  seg[3] = digit_codes[sec % 10];
  tm1637_display_segments(seg);
}

static void enterState(State s) {
  state = s;
  stateStart = tickCount;
  ledOff(LED_GREEN_PIN);
  ledOff(LED_RED_PIN);

  switch (s) {
    case ST_INPUT:
      enteredLen = 0;
      ledOff(LED_INPUT_PIN);
      showInput();
      printf("Enter %d-digit code\n", CODE_LEN);
      break;
    case ST_GRANTED:
      ledOff(LED_INPUT_PIN);
      ledOn(LED_GREEN_PIN);
      showOpen();
      printf("ACCESS GRANTED\n");
      break;
    case ST_DENIED:
      ledOff(LED_INPUT_PIN);
      ledOn(LED_RED_PIN);
      showError();
      printf("ACCESS DENIED (%d/%d)\n", failCount, MAX_ATTEMPTS);
      break;
    case ST_LOCKED:
      ledOff(LED_INPUT_PIN);
      ledOn(LED_RED_PIN);
      lastBlink = tickCount;
      lastShownSec = 0xFF;
      printf("LOCKED for %d s\n", LOCK_TIME_MS / 1000);
      break;
  }
}

static void checkCode(void) {
  if (enteredLen == CODE_LEN && memcmp(entered, SECRET_CODE, CODE_LEN) == 0) {
    failCount = 0;
    enterState(ST_GRANTED);
  } else {
    failCount++;
    if (failCount >= MAX_ATTEMPTS) {
      failCount = 0;
      enterState(ST_LOCKED);
    } else {
      enterState(ST_DENIED);
    }
  }
}

static void handleKey(char key) {
  if (state != ST_INPUT) {
    return;
  }

  if (key >= '0' && key <= '9') {
    if (enteredLen < CODE_LEN) {
      entered[enteredLen++] = key;
      ledOn(LED_INPUT_PIN);
      showInput();
      printf("Key: %c (%d/%d)\n", key, enteredLen, CODE_LEN);
    }
  } else if (key == '*') {
    if (enteredLen > 0) {
      enteredLen--;
      showInput();
      if (enteredLen == 0) ledOff(LED_INPUT_PIN);
      printf("Backspace (%d/%d)\n", enteredLen, CODE_LEN);
    }
  } else if (key == '#') {
    checkCode();
  }
}

static void updateState(void) {
  uint32_t elapsed = tickCount - stateStart;

  switch (state) {
    case ST_GRANTED:
    case ST_DENIED:
      if (elapsed >= RESULT_TIME_MS) {
        enterState(ST_INPUT);
      }
      break;

    case ST_LOCKED:
      if (elapsed >= LOCK_TIME_MS) {
        enterState(ST_INPUT);
        break;
      }
      if (tickCount - lastBlink >= BLINK_MS) {
        lastBlink = tickCount;
        ledToggle(LED_RED_PIN);
      }
      {
        uint8_t sec = (uint8_t)((LOCK_TIME_MS - elapsed + 999) / 1000);
        if (sec != lastShownSec) {
          lastShownSec = sec;
          showLockTimer(sec);
        }
      }
      break;

    default:
      break;
  }
}

int main(void) {
  initGPIO();
  initUSART2();
  initSysTick();
  initKeyboard();
  tm1637_init();

  printf("Access control system started\n");
  enterState(ST_INPUT);

  while (1) {
    char key = scanKeyboard();
    if (key != '\0') {
      handleKey(key);
    }
    updateState();
  }
  return 0;
}
