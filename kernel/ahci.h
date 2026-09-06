#pragma once

#include <stdint.h>

int AhciInit(uint32_t Abar);
int AhciReadSectors(uint64_t Lba, uint32_t Count, void *Buffer);
