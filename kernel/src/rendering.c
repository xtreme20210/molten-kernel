#include <stdint.h>
#include <x86intrin.h>
#include "rendering.h"
#include "font.h"
#include "scancodes.h"
#include "memory.h"
#include "structs.h"

const uint32_t indentX = 25;
const uint32_t indentY = 65;
uint32_t cursorX = indentX;
uint32_t cursorY = indentY;
static volatile uint32_t *framebuffer;
struct frameBuffer *fbPtr;
uint32_t lineCount = 0;
uint32_t bColor = 0x004A0011;
struct lineData lines[100];
bool cursorFilled = false;

struct renderInfo renderInit(struct bootInfo *bootInfo) {
    struct renderInfo initRenderInfo;
    initRenderInfo.fbPtr = &bootInfo->fbInfo;
    fbPtr = initRenderInfo.fbPtr;
    initRenderInfo.framebuffer = (volatile uint32_t *)(uintptr_t)initRenderInfo.fbPtr->fbBase;
    framebuffer = initRenderInfo.framebuffer;
    initRenderInfo.bColor = bColor;
    return initRenderInfo;
}



void setColor(uint32_t x, uint32_t y, uint32_t color) {
    uint32_t pLocation = (x + y * fbPtr->ppLine);
    framebuffer[pLocation] = color;
}

void fill(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    if (x1 > x2) {
        uint32_t temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        uint32_t temp = y1;
        y1 = y2;
        y2 = temp;
    }
    for (int yi = (y1 * fbPtr->ppLine); yi < (y2 * fbPtr->ppLine); yi += fbPtr->ppLine) {
        for (int xi = x1; xi < x2; xi++) {
            framebuffer[yi + xi] = color;
        }
    }
}

void drawChar(unsigned char character, uint32_t x, uint32_t y, uint32_t color, uint8_t scale) {
    for (int i = 0; i < 8; i++) {
        uint8_t row = font8x8_basic[character][i];
        for (int ix = 0; ix < 8; ix++) {
            if (row & (1 << ix)) {
                fill((ix*scale + x), (i*scale + y), (ix*scale + x + scale), (i*scale + y + scale), color);
            }
        }
    }
}

void drawString(const char *string, uint32_t x, uint32_t y, uint32_t color, uint8_t scale) {
    for (int i = 0; string[i] != '\0'; i++) {
        drawChar(string[i], x, y, color, scale);
        x += 8*scale;
    }
}

void coutChar(char character) {
    if (character == '\n' || (cursorX + 8) > fbPtr->hRes) {
        fill(cursorX, cursorY, cursorX+8, cursorY+8, bColor);
        lines[lineCount].x = cursorX;
        lines[lineCount].y = cursorY;
        cursorX = indentX;
        cursorY += 10;
        lineCount++;
        if (character == '\n') {
            return;
        }
    }
    fill(cursorX, cursorY, cursorX+8, cursorY+8, bColor);
    drawChar(character, cursorX, cursorY, 0x00000000, 1);
    cursorX += 8;
}

void cout(const char *string) {
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] == '\n' || (cursorX + 8) > fbPtr->hRes) {
            lines[lineCount].x = cursorX;
            lines[lineCount].y = cursorY;
            cursorX = indentX;
            cursorY += 10;
            lineCount++;
            if (string[i] == '\n') {
                continue;
            }
        }
        drawChar(string[i], cursorX, cursorY, 0x00000000, 1);
        cursorX += 8;
    }
}

void printHex(uint64_t input) {
    const char hex[] = "0123456789ABCDEF";
    char print[17];
    for (int i=0; i<16; i++) {
        print[i] = hex[input >> 60 - i*4 & 0x0F];
    }

    print[16] = '\0';
    cout(print);
}

void cursorBlink() {
    if ((__rdtsc() % 2000000000ULL) > 1000000000ULL) {
        if (!cursorFilled) {
            fill(cursorX, cursorY, cursorX + 8, cursorY + 8, 0x00000000);
        }
        cursorFilled = true;
    } else {
        if (cursorFilled) {
            fill(cursorX, cursorY, cursorX+8, cursorY+8, bColor);
        }
        cursorFilled = false;
    }
}

void backspace() {
    if (cursorX == indentX && cursorY > 8 && lineCount >0) {
        fill(cursorX+8, cursorY, cursorX-8, cursorY+8, bColor);
        lineCount--;
        cursorX = lines[lineCount].x;
        cursorY = lines[lineCount].y;
    } else if (cursorX >= (indentX+8)) {
        fill(cursorX-8, cursorY, cursorX+8, cursorY+8, bColor);
        cursorX -= 8;
    }
}