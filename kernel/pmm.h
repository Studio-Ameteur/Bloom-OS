#pragma once

#include <stdint.h>
#include "boot_info.h"

void InitPmm(BOOT_INFO *Info);
void *AllocPage(void);
void FreePage(void *Addr);
uint64_t GetFreePageCount(void);
uint64_t GetTotalPageCount(void);
