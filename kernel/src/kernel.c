#include <stdint.h>

struct frameBuffer {
    uint64_t fbBase;
    uint64_t fbSize;
    uint32_t hRes;
    uint32_t vRes;
    uint32_t ppLine;
    uint32_t pFormat;
};

void ioout(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

void setColor(struct frameBuffer *fbPtr, uint8_t x, uint8_t y, uint8_t color) {
    volatile uint32_t *framebuffer = (uint32_t *)(uintptr_t)fbPtr->fbBase;
    uint32_t pLocation = (x + y * fbPtr->ppLine);
    framebuffer[pLocation] = color;
}

void fill(struct frameBuffer *fbPtr, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    volatile uint32_t *framebuffer = (uint32_t *)(uintptr_t)fbPtr->fbBase;
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    for (int yi = (y1 * fbPtr->ppLine); yi < (y2 * fbPtr->ppLine); yi += fbPtr->ppLine) {
        for (int xi = x1; xi < x2; xi++) {
            framebuffer[yi + xi] = color;
        }
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

    uint32_t *framebuffer = (uint32_t *)(uintptr_t)fbPtr->fbBase;
    for (int i = 0; i< (fbPtr->fbSize / 4); i++) {
        if (fbPtr->pFormat = 1) {
            framebuffer[i] = 0x00FF6600;
        }
        if (fbPtr->pFormat = 0) {
            framebuffer[i] = 0x000066FF;
        }

    }
    while (1) {

    }
}