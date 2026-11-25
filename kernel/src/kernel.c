#include <stdint.h>
#include <x86intrin.h>
#include "font.h"
#include "scancodes.h"
#include "memory.h"
#include "rendering.h"
#include "structs.h"
#include "paging.h"

bool shiftHeld = false;

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
            backspace();
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

void kernel_main(struct bootInfo *bootInfoPtr) {
    struct renderInfo initRenderInfo;
    initRenderInfo = renderInit(bootInfoPtr);
    for (int i = 0; i < (initRenderInfo.fbPtr->fbSize / 4); i++) {
        initRenderInfo.framebuffer[i] = initRenderInfo.bColor;
    }

    drawString("Welcome to Molten kernel!", 25, 25, 0x00000000, 2);

    cout("initializing memory...\n");
    memInit(bootInfoPtr);
    cout("initializing virtual memory...\n");
    bool success = vmemInit();
    if (success == true) {
        cout("virtual memory initialized, mapping test...");
        void *physicalPage = allocPage(1);
        bool mapSuccess = mapPage(0x40000000, (uint64_t)(uintptr_t)physicalPage);
        if (mapSuccess == true) {
            cout("mapPage success!");
        } else {
            cout("mapPage failed");
        }
    }

    while (1) {
        ps2Poll();
        cursorBlink();
    }
}
