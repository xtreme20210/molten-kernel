#pragma once

#include <stdint.h>
#include "structs.h"

struct renderInfo renderInit(struct bootInfo *bootInfo);
void setColor(uint32_t x, uint32_t y, uint32_t color);
void fill(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void drawChar(unsigned char character, uint32_t x, uint32_t y, uint32_t color, uint8_t scale);
void drawString(const char *string, uint32_t x, uint32_t y, uint32_t color, uint8_t scale);
void coutChar(char character);
void cout(const char *string);
void printHex(uint64_t input);
void cursorBlink();
void backspace();