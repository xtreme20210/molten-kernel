#include <stdint.h>
#include <x86intrin.h>
#include "memory.h"
#include "font.h"
#include "scancodes.h"
#include "structs.h"
#include "rendering.h"

uint64_t usablePages = 0;

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