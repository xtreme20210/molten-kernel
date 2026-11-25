#pragma once

#include <stdint.h>

void *zeroInitPage();
bool vmemInit();
bool mapPage(uint64_t virtualAddress, uint64_t physicalAddress);
void addPML4Entry(uint64_t virtualAddress);
void addPDPTEntry(uint64_t virtualAddress);
void addPDEntry(uint64_t virtualAddress);
