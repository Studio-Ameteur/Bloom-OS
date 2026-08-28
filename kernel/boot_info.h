#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>

typedef struct {
    uint64_t FrameBufferBase;
    uint64_t FrameBufferSize;
    uint64_t Width;
    uint64_t Height;
    uint64_t PixelsPerScanLine;
} BOOT_INFO;

#endif
