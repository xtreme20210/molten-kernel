#include <stdint.h>
#include "font.h"

struct frameBuffer {
    uint64_t fbBase;
    uint64_t fbSize;
    uint32_t hRes;
    uint32_t vRes;
    uint32_t ppLine;
    uint32_t pFormat;
};

volatile uint32_t *framebuffer;
uint32_t cursorX = 25;
uint32_t cursorY = 25;

void ioout(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

void setColor(struct frameBuffer *fbPtr, uint32_t x, uint32_t y, uint32_t color) {
    uint32_t pLocation = (x + y * fbPtr->ppLine);
    framebuffer[pLocation] = color;
}

void fill(struct frameBuffer *fbPtr, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
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

void drawChar(struct frameBuffer *fbPtr, unsigned char character, uint32_t x, uint32_t y, uint32_t color, uint8_t scale) {
    for (int i = 0; i < 8; i++) {
        uint8_t row = font8x8_basic[character][i];
        for (int ix = 0; ix < 8; ix++) {
            if (row & (1 << ix)) {
                fill(fbPtr, (ix*scale + x), (i*scale + y), (ix*scale + x + scale), (i*scale + y + scale), color);
            }
        }
    }
}

void drawString(struct frameBuffer *fbPtr, const char *string, uint32_t x, uint32_t y, uint32_t color, uint8_t scale) {
    uint32_t xog = x;
    for (int i = 0; string[i] != '\0'; i++) {
        drawChar(fbPtr, string[i], x, y, color, scale);
        x += 8*scale;
    }
}

void cout(struct frameBuffer *fbPtr, const char *string) {
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] == '\n' || (cursorX + 8) > fbPtr->hRes) {
            cursorX = 0;
            cursorY += 10;
            if (string[i] == '\n') {
                continue;
            }
        }
        drawChar(fbPtr, string[i], cursorX, cursorY, 0x00000000, 1);
        cursorX += 8;
    }
}

void kernel_main(struct frameBuffer *fbPtr) {
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
        if (fbPtr->pFormat = 1) {
            framebuffer[i] = 0x004A0011;
        }
        if (fbPtr->pFormat = 0) {
            framebuffer[i] = 0x004A0011;
        }

    }
    cout(fbPtr, "Welcome to Molten kernel!\n");
    while (1) {

    }
}