#pragma once

#include <stdint.h>

typedef struct {
    int Found;
    int CodecFound;
    int DacFound;
    int PinFound;
    int PathLinked;
    int StreamStarted;
} HDA_STATUS;

HDA_STATUS HdaInit(uint32_t Bar0);
void HdaPlayTestTone(void);
