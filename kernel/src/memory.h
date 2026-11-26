#pragma once

#include <stdint.h>
#include "structs.h"

bool pageAvaliable(uint64_t pageNumber, uint8_t *bitmap);
void memInit(struct bootInfo *bootInfoPtr);
void *allocPage(uint64_t number);
void freePage(void *address, uint64_t number);
struct memInfo getMemory();