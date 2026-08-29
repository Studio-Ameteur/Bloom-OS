#pragma once

#include <stdint.h>

extern volatile uint64_t TickCount;

void InitIdt(void);
void EnableInterrupts(void);
void Sleep(uint64_t Milliseconds);
