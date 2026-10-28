#pragma once

#include <stdint.h>

struct frameBuffer {
    uint64_t fbBase;
    uint64_t fbSize;
    uint32_t hRes;
    uint32_t vRes;
    uint32_t ppLine;
    uint32_t pFormat;
};

struct renderInfo {
    struct frameBuffer *fbPtr;
    volatile uint32_t *framebuffer;
    uint64_t bColor;
};

struct memInfo {
    uint8_t *bitmap;
    uint64_t bitmapPages;
    uint64_t endPage;
};

typedef struct {
    uint32_t Type;
    uint32_t Padding;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} MemoryDescriptor;

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