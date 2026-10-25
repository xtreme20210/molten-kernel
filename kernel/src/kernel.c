#include <stdint.h>
#include <x86intrin.h>
#include "font.h"
#include "scancodes.h"

typedef struct {
    uint32_t Type;
    uint32_t Padding;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} MemoryDescriptor;

struct memInfo {
    uint8_t *bitmap;
    uint64_t bitmapPages;
    uint64_t endPage;
};

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

struct bootInfo {
    struct frameBuffer fbInfo;

    void *memoryMap;
    uint64_t memoryMapSize;
    uint64_t descriptorSize;
    uint32_t descriptorVersion;
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
uint64_t usablePages = 0;


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

bool pageAvaliable(uint64_t pageNumber, uint8_t *bitmap) {
    uint64_t byteIndex = pageNumber/8;
    uint8_t bitIndex = pageNumber%8;

    if (bitmap[byteIndex] & (0x01 << bitIndex)) {
        return false;
    } 
    return true;
}

struct memInfo memInit(struct bootInfo *bootInfoPtr) {
    uint64_t endPage = 0;
    uint64_t bitmapAddress = 0;
    uint64_t bitmapPage = 0;
    for (uint64_t i=0; i < bootInfoPtr->memoryMapSize / bootInfoPtr->descriptorSize; i++ ) {
        uint8_t *address = bootInfoPtr->memoryMap;
        address += (i * bootInfoPtr->descriptorSize);
        MemoryDescriptor *descriptor = (MemoryDescriptor *)address;

        if (descriptor->Type == 7) {
            uint64_t lastPage = descriptor->PhysicalStart / 4096 + descriptor->NumberOfPages;
            usablePages += descriptor->NumberOfPages;
            if (lastPage > endPage) {
                endPage = lastPage;
            }

        }
    }

    uint64_t bitmapSize = (endPage+7)/8;
    uint64_t bitmapPages = (bitmapSize + 4095) / 4096;

    for (uint64_t i=0; i < bootInfoPtr->memoryMapSize / bootInfoPtr->descriptorSize; i++ ) {
        uint8_t *address = bootInfoPtr->memoryMap;
        address += (i * bootInfoPtr->descriptorSize);
        MemoryDescriptor *descriptor = (MemoryDescriptor *)address;
        if (descriptor->NumberOfPages >= bitmapPages && descriptor->Type == 7) {
            bitmapAddress = descriptor->PhysicalStart + ((descriptor->NumberOfPages - bitmapPages)*4096);
            uint64_t lastPage = descriptor->PhysicalStart / 4096 + descriptor->NumberOfPages;
            bitmapPage = lastPage - bitmapPages;
        }
    }

    if (bitmapAddress == 0) {
        cout("could not find suitable region for bitmap\n");
        struct memInfo memInitFailed;
        memInitFailed.bitmap = NULL;
        memInitFailed.bitmapPages = 0;
        memInitFailed.endPage = 0;
        return memInitFailed;
    }

    uint8_t *bitmap = (uint8_t *)(uintptr_t)bitmapAddress;
    for (uint64_t i = 0; i < bitmapSize; i++) {
        bitmap[i]=0xFF;
    }



    for (uint64_t i=0; i < bootInfoPtr->memoryMapSize / bootInfoPtr->descriptorSize; i++ ) {
        uint8_t *address = bootInfoPtr->memoryMap;
        address += (i * bootInfoPtr->descriptorSize);
        MemoryDescriptor *descriptor = (MemoryDescriptor *)address;
        if (descriptor->Type == 7) {
            for (uint64_t ii=0; ii<descriptor->NumberOfPages; ii++) {
                uint64_t page = descriptor->PhysicalStart / 4096 + ii;
                bitmap[page/8] &= ~(1 << page%8);
            }
        }
    }

    for (uint64_t i=0; i < bitmapPages; i++) {
        uint64_t page = bitmapPage + i;
        bitmap[page/8] |= (1 << page%8);
    }

    bitmap[0] |= 1;
    struct memInfo initMemInfo;
    initMemInfo.bitmap = bitmap;
    initMemInfo.bitmapPages = bitmapPages;
    initMemInfo.endPage = endPage;
    return initMemInfo;
}

void *allocPage(uint64_t number, uint8_t *bitmap, uint64_t endPage) {
    if(number == 0) {
        cout("you cannot allocate 0 pages\n");
        return NULL;
    }
    if(number > endPage) {
        cout("you cannot allocate more memory than the system has free\n");
        return NULL;
    }
    for (uint64_t i=1; i+number <= endPage; i++) {
        bool sectionValid = true;
        for (uint64_t ii=0; ii < number; ii++) {
            bool avaliable = pageAvaliable(i + ii, bitmap);
            if (!avaliable) {
                sectionValid = false;
                break;
            }
        }
        if (sectionValid) {
            for (uint64_t ii = 0; ii < number; ii++)
            bitmap[(i+ii)/8] |= (1 << (i+ii)%8);
            return (void *)(uintptr_t)(i*4096);
        }
    }
    cout("failed to allocate pages\n");
    return NULL;
}

void freePage(struct memInfo *mem, void *address, uint64_t number) {
    uint64_t pageNumber = (uintptr_t)address/4096;
    if(number == 0) {
        cout("you cannot free 0 pages\n");
        return;
    }
    if ((uint64_t)address%4096 != 0) {
        cout("address is not page aligned\n");
        return;
    }
    if (address == NULL) {
        cout("you cannot free a null address\n");
        return;
    }
    if (pageNumber > mem->endPage || number > mem->endPage - pageNumber) {
        cout("outside memory bounds\n");
        return;
    }
    for (uint64_t i = pageNumber; i < pageNumber+number; i++) {
        mem->bitmap[i/8] &= ~(1 << i%8);
    }
    return;
}

void kernel_main(struct bootInfo *bootInfoPtr) {
    fbPtr = &bootInfoPtr->fbInfo;

    framebuffer = (volatile uint32_t *)(uintptr_t)fbPtr->fbBase;
    for (int i = 0; i < (fbPtr->fbSize / 4); i++) {
        framebuffer[i] = bColor;
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
