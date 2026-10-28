#pragma once

#include <stdint.h>
#include "structs.h"

bool pageAvaliable(uint64_t pageNumber, uint8_t *bitmap);
struct memInfo memInit(struct bootInfo *bootInfoPtr);
void *allocPage(uint64_t number, uint8_t *bitmap, uint64_t endPage);
void freePage(struct memInfo *mem, void *address, uint64_t number);