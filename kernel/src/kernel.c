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

    printHex((uint64_t)(uintptr_t)&kernel_main);
    uint64_t rsp;

    __asm__ volatile (
        "mov %%rsp, %0"
        : "=r"(rsp)
    );

    printHex(rsp);

    struct frameBuffer *fbPtr = getfbPtr();
    cout("fbBase: ");
    printHex(fbPtr->fbBase);
    cout("\nfbSize: ");
    printHex(fbPtr->fbSize);


    struct memInfo memoryInfo = getMemory();

    cout("\nbitmap: ");
    printHex((uint64_t)(uintptr_t)memoryInfo.bitmap);
    cout("\nendPage: ");
    printHex(memoryInfo.endPage);

    cout("\npml4 root: ");
    printHex((uint64_t)(uintptr_t)getPML4Root());

    cout("\nbootinfoptr: ");
    printHex((uint64_t)(uintptr_t)bootInfoPtr);
    cout("\nmemorymap: ");
    printHex((uint64_t)(uintptr_t)bootInfoPtr->memoryMap);
    cout("\nmemorymap size: ");
    printHex(bootInfoPtr->memoryMapSize);

    cout("\nstack base: ");
    printHex(bootInfoPtr->stackBase);
    cout("\nstack size: ");
    printHex(bootInfoPtr->stackSize);
    cout("\nksegment count: ");
    printHex(bootInfoPtr->kernelSegmentCount);
    cout("\nvirtual address: ");
    printHex(bootInfoPtr->kSegment[0].virtualAddress);
    cout("\nphysical address: ");
    printHex(bootInfoPtr->kSegment[0].physicalAddress);
    cout("\nsize: ");
    printHex(bootInfoPtr->kSegment[0].size);

    for (uint64_t i = 0; i < bootInfoPtr->kernelSegmentCount; i++) {
        if (bootInfoPtr->kSegment[i].physicalAddress != bootInfoPtr->kSegment[i].virtualAddress) {
            goto pagingfailure;
        }
        bool kSegmentSuccess = identityMapRange(bootInfoPtr->kSegment[i].physicalAddress, bootInfoPtr->kSegment[i].size);
        if (!kSegmentSuccess) {
            goto pagingfailure;
        }
    }
    bool stackMapSuccess = identityMapRange(bootInfoPtr->stackBase, bootInfoPtr->stackSize);
    if(!stackMapSuccess) {
        cout("\nstack map failure");
        goto pagingfailure;
    }

    bool fbMapSuccess = identityMapRange(fbPtr->fbBase, fbPtr->fbSize);
    if (!fbMapSuccess) {
        cout("\nfb map failure");
        goto pagingfailure;
    }

    bool bitmapMapSuccess = identityMapRange((uint64_t)(uintptr_t)memoryInfo.bitmap, (memoryInfo.bitmapPages*4096));
    if (!bitmapMapSuccess) {
        cout("\nbitmap map failure");
        goto pagingfailure;
    }

    bool bootInfoMapSuccess = identityMapRange((uint64_t)(uintptr_t)bootInfoPtr, sizeof(*bootInfoPtr));
    if (!bootInfoMapSuccess) {
        cout("\nbootinfo map failure");
        goto pagingfailure;
    }

    bool memorymapMapSuccess = identityMapRange((uint64_t)(uintptr_t)bootInfoPtr->memoryMap, bootInfoPtr->memoryMapSize);
    if (!memorymapMapSuccess) {
        cout("\nmemorymap map failure");
        goto pagingfailure;
    }

    if (!identityMapConventionalMemory(bootInfoPtr)) {
        cout("\nconv memory map failure");
        goto pagingfailure;
    }

    cout("\nmapping completed");

    uint64_t CR4;
        __asm__ volatile (
        "mov %%CR4, %0"
        : "=r"(CR4)
    );
    if (CR4 & (1ULL << 12)) {
        goto pagingfailure;
    }

    struct descriptorTableRegister gdtr;
    struct descriptorTableRegister idtr;

    __asm__ volatile (
        "sgdt %0"
        : "=m"(gdtr)
    );

    __asm__ volatile (
        "sidt %0"
        : "=m"(idtr)
    );

    cout("gdt base: ");
    printHex(gdtr.base);
    cout("\n");

    cout("gdt limit: ");
    printHex(gdtr.limit);
    cout("\n");

    cout("idt base: ");
    printHex(idtr.base);
    cout("\n");

    cout("idt limit: ");
    printHex(idtr.limit);
    cout("\n");

    if (!identityMapRange(gdtr.base, (uint64_t)gdtr.limit + 1)) {
        goto pagingfailure;
    }

    if (!identityMapRange(idtr.base, (uint64_t)idtr.limit + 1)) {
        goto pagingfailure;
    }   

    uint64_t newCR3 = (uint64_t)(uintptr_t)getPML4Root();

    __asm__ volatile (
        "cli\n\t"
        "mov %0, %%cr3"
        :
        : "r"(newCR3)
        : "memory"
    );
    cout("CR3 switch successful!");

pagingfailure:
    while (1) {
        ps2Poll();
        cursorBlink();
    }
}
