#include <stdint.h>
#include <x86intrin.h>
#include "font.h"
#include "scancodes.h"
#include "memory.h"
#include "rendering.h"
#include "structs.h"

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
    struct memInfo memInitInfo;
    memInitInfo = memInit(bootInfoPtr);

    if (memInitInfo.bitmap == NULL) {
        cout("memory initialization failed\n");
    } else {
        if (pageAvaliable(0, memInitInfo.bitmap)) {
            cout("memory initialization failed\n");
        } else {
            void *memPtr = allocPage(1, memInitInfo.bitmap, memInitInfo.endPage);
            if(memPtr == NULL || pageAvaliable((uintptr_t)memPtr / 4096, memInitInfo.bitmap)) {
                cout("memory initialization failed\n");
            } else {
                freePage(&memInitInfo, memPtr, 1);
                if(memPtr == NULL || !pageAvaliable((uintptr_t)memPtr / 4096, memInitInfo.bitmap) || pageAvaliable(0, memInitInfo.bitmap)) {
                    cout("memory intialization failed\n");
                } else {
                    cout("memory initialized\n");
                }
            }
        }
    }

    while (1) {
        ps2Poll();
        cursorBlink();
    }
}
