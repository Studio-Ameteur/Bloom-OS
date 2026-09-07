#pragma once

#include <stdint.h>

typedef struct {
    uint32_t Cluster;
    uint32_t Size;
    int Found;
} FAT_FILE;

int FatInit(void);
FAT_FILE FatFindFile(const char *ShortName);
int FatReadFile(FAT_FILE File, void *Buffer, uint32_t MaxBytes);
