#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "structs.h"
#include "rendering.h"

static uint64_t *pml4 = NULL;

uint64_t *getPML4Root(){
    return pml4;
}

uint16_t getPML4(uint64_t vAddr) {
    uint16_t PML4Addr = 0x1FF & (vAddr >> 39);
    return PML4Addr;
}

uint16_t getPDPT(uint64_t vAddr) {
    uint16_t PDPTAddr = 0x1FF & (vAddr >> 30);
    return PDPTAddr;
}

uint16_t getPD(uint64_t vAddr) {
    uint16_t PDAddr = 0x1FF & (vAddr >> 21);
    return PDAddr;
}

uint16_t getPT(uint64_t vAddr) {
    uint16_t PTAddr = 0x1FF & (vAddr >> 12);
    return PTAddr;
}

void *zeroInitPage() {
    uint64_t *pagePtr = allocPage(1);

    if (pagePtr == NULL) {
        return NULL;
    }

    for (uint64_t i = 0; i <512; i++) {
        pagePtr[i] = 0;
    }
    return pagePtr;
}

uint64_t extractpAddr(uint64_t entry) {
        entry &= ~(0xFFF0000000000FFFULL);
        return entry;
}

void addPML4Entry(uint64_t virtualAddress) {
    if (pml4 == NULL) {
        cout("must initialize virtual memory before adding an entry");
        return;
    }
    
    if (pml4[getPML4(virtualAddress)] & (0x01)) {
        return;
    }

    uint64_t *entrypAddr = zeroInitPage();
    if (entrypAddr == NULL) {
        return;
    }
    uint64_t entry = (uint64_t)(uintptr_t)entrypAddr;
    entry |= 0x3;
    pml4[getPML4(virtualAddress)] = entry;
    return;
}

void addPDPTEntry(uint64_t virtualAddress) {
    uint16_t pml4Index = getPML4(virtualAddress);
    if (pml4 == NULL) {
        return;
    }
    if (!(pml4[pml4Index] & (0x01))) {
        addPML4Entry(virtualAddress);
    }
    if (!(pml4[pml4Index] & (0x01))) {
        return;
    }

    uint64_t PDPTpAddr = extractpAddr(pml4[pml4Index]);
    uint64_t *pdpt = (uint64_t *)(uintptr_t)PDPTpAddr;

    uint16_t index = getPDPT(virtualAddress);
    if (pdpt[index] & (0x01)) {
        return;
    }
    void *PDAddr = zeroInitPage();
    if (PDAddr == NULL) {
        return;
    }

    uint64_t entry = (uint64_t)(uintptr_t)PDAddr;
    entry |= 0x3;
    pdpt[getPDPT(virtualAddress)] = entry;
    return;
}

uint64_t *addPDEntry(uint64_t virtualAddress) {
    uint16_t pml4Index = getPML4(virtualAddress);
        if (pml4 == NULL) {
        return NULL;
    }
    if (!(pml4[pml4Index] & (0x01))) {
        addPML4Entry(virtualAddress);
    }
    if (!(pml4[pml4Index] & (0x01))) {
        return NULL;
    }
    uint64_t PDPTpAddr = extractpAddr(pml4[pml4Index]);
    uint64_t *pdpt = (uint64_t *)(uintptr_t)PDPTpAddr;
    uint16_t PDPTindex = getPDPT(virtualAddress); 
    if (pdpt == NULL) {
        return NULL;
    }
    if (!(pdpt[PDPTindex] & (0x01))) {
        addPDPTEntry(virtualAddress);
    }
    if (!(pdpt[PDPTindex] & (0x01))) {
        return NULL;
    }

    uint64_t PDpAddr = extractpAddr(pdpt[PDPTindex]);
    uint64_t *pd = (uint64_t *)(uintptr_t)PDpAddr;
    if (pd[getPD(virtualAddress)] & (0x01)) {
        uint64_t PTaddr = extractpAddr(pd[getPD(virtualAddress)]);
        return (uint64_t *)(uintptr_t)PTaddr;
    }
    void *PTAddr = zeroInitPage();
    if (PTAddr == NULL) {
        return NULL;
    }
        uint64_t entry = (uint64_t)(uintptr_t)PTAddr;
    entry |= 0x3;
    pd[getPD(virtualAddress)] = entry;
    return (uint64_t *)PTAddr;
}

bool mapPage(uint64_t virtualAddress, uint64_t physicalAddress) {
    if (((virtualAddress & (0xFFF)) != 0) || ((physicalAddress & (0xFFF)) != 0)) {
        return false;
    }

    uint64_t *PTptr = addPDEntry(virtualAddress);
    if (PTptr == NULL) {
        return false;
    }
    uint16_t leafIndex = getPT(virtualAddress);

    if (PTptr[leafIndex] & (0x01)) {
        return false;
    }
    PTptr[leafIndex] = (physicalAddress | (0x03));
    return true;
}

bool walkPageTables(uint64_t virtualAddress, uint64_t *physicalAddress) {
    if (pml4 == NULL || physicalAddress == NULL) {
        return false;
    }
    uint64_t PML4Entry = pml4[getPML4(virtualAddress)];
    if (!(PML4Entry & (0x01))) {
        return false;
    }
    uint64_t *pdpt = (uint64_t *)(uintptr_t)extractpAddr(PML4Entry);
    uint64_t PDPTEntry = pdpt[getPDPT(virtualAddress)];
    if (!(PDPTEntry & (0x01))) {
        return false;
    }
    uint64_t *pd = (uint64_t *)(uintptr_t)extractpAddr(PDPTEntry);
    uint64_t PDEntry = pd[getPD(virtualAddress)];
    if (!(PDEntry & (0x01))) {
        return false;
    }
    uint64_t *pt = (uint64_t *)(uintptr_t)extractpAddr(PDEntry);
    uint64_t PTEntry = pt[getPT(virtualAddress)];
    if (!(PTEntry & (0x01))) {
        return false;
    }
    uint64_t physicalTable = extractpAddr(PTEntry);
    physicalTable += (virtualAddress & 0xFFF);
    *physicalAddress = physicalTable;
    return true;
}

bool identityMapRange(uint64_t startAddress, uint64_t size) {
    if (size == 0) {
        return true;
    }
    uint64_t offset = startAddress % 0x1000;
    startAddress &= ~(0xFFF);
    size += offset;
    for (uint64_t i = 0; i < ((size+0xFFF) / 0x1000); i++) {
        uint64_t startOffset = i*0x1000;
        uint64_t result;
        bool walkSuccess = walkPageTables((startAddress+startOffset), &result);
        if (walkSuccess && result==(startAddress+startOffset)) {
            continue;
        } else if (walkSuccess) {
            cout("\nConflicting mapping!\n");

            cout("Virtual address: ");
            printHex(startAddress + startOffset);

            cout("\nExisting physical address: ");
            printHex(result);

            cout("\n");
            return false;
        }
        bool success = mapPage((startAddress+startOffset), (startAddress+startOffset));
        if (!success) {
            return false;
        }
    }
    return true;
}

bool identityMapConventionalMemory(struct bootInfo *bootInfoPtr) {
    if (bootInfoPtr == NULL || bootInfoPtr->memoryMap == NULL || bootInfoPtr->descriptorSize == 0) {
        return false;
    }
    for (uint64_t i=0; i < bootInfoPtr->memoryMapSize / bootInfoPtr->descriptorSize; i++ ) {
        uint8_t *address = bootInfoPtr->memoryMap;
        address += (i * bootInfoPtr->descriptorSize);
        MemoryDescriptor *descriptor = (MemoryDescriptor *)address;

        if (descriptor->Type == 7) {
            bool success = identityMapRange(descriptor->PhysicalStart, descriptor->NumberOfPages * 4096);
            if (!success) {
                return false;
            }
        }
    }
    return true;
}

bool vmemInit() {
    pml4 = zeroInitPage();
    if (pml4 == NULL) {
        return false;
    }
    return true;
}