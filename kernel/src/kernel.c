#include <stdint.h>
#include <x86intrin.h>
#include "font.h"
#include "scancodes.h"

struct frameBuffer {
    uint64_t fbBase;
    uint64_t fbSize;
    uint32_t hRes;
    uint32_t vRes;
    uint32_t ppLine;
    uint32_t pFormat;
};

struct lineData {
    uint32_t x;
    uint32_t y;
};

struct lineData lines[100];
volatile uint32_t *framebuffer;
struct frameBuffer *fbPtr;
bool shiftHeld = false;
uint32_t lineCount = 0;
uint32_t bColor = 0x004A0011;
const uint32_t indentX = 25;
const uint32_t indentY = 65;
uint32_t cursorX = indentX;
uint32_t cursorY = indentY;
bool cursorFilled = false;

void ioout(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile (
        "inb %1, %0"
        : "=a"(result)
        : "Nd"(port)
    );
    return result;
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
    for (int yi = (y1 * fbPtr->ppLine); yi <= (y2 * fbPtr->ppLine); yi += fbPtr->ppLine) {
        for (int xi = x1; xi <= x2; xi++) {
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
    uint32_t xog = x;
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

void printHex(uint8_t input) {
    const char hex[] = "0123456789ABCDEF";
    char print[3];
    print[0] = hex[input >> 4];
    print[1] = hex[input & 0x0F];
    print[2] = '\0';
    cout(print);
}

void ps2Poll() {
    uint8_t ps2Status = inb(0x64);
    if (ps2Status & 0x01) {
        uint8_t ps2Code = inb(0x60);
        if (ps2Code == 0x2A) {
            shiftHeld = true;
        } else if (ps2Code == 0xAA) {
            shiftHeld = false;
        }
        if (ps2Code == 0x0E) {
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
        if ((ps2Code >> 7) == 0 && scToAscii[ps2Code] != '\0') {
            if (shiftHeld == true) {
                coutChar(scToAsciiShifted[ps2Code]);
            } else {
                coutChar(scToAscii[ps2Code]);
            }
        }
    }
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

void kernel_main(struct frameBuffer *bootfbPtr) {
    fbPtr = bootfbPtr;
    ioout(0x3F9, 0x00);
    ioout(0xF3B, 0x80);
    ioout(0xF38, 0x01);
    ioout(0xF39, 0x00);
    ioout(0xF3B, 0x03);
    ioout(0xF3A, 0xC7);
    ioout(0x3FC, 0x03);
    ioout(0x3F8, 'A');

    framebuffer = (volatile uint32_t *)(uintptr_t)fbPtr->fbBase;
    for (int i = 0; i < (fbPtr->fbSize / 4); i++) {
        if (fbPtr->pFormat == 1) {
            framebuffer[i] = bColor;
        }
        if (fbPtr->pFormat == 0) {
            framebuffer[i] = bColor;
        }
    }
    drawString("Welcome to Molten kernel!", 25, 25, 0x00000000, 2);

    uint8_t ps2Status = inb(0x64);
    uint8_t input;
    if (ps2Status == 0) {
        input = inb(0x60);
    }


    while (1) {
        ps2Poll();
        cursorBlink();
    }
}