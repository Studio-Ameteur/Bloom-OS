#pragma once

#include <stdint.h>

typedef struct {
    uint8_t Bus;
    uint8_t Device;
    uint8_t Function;
    uint16_t VendorId;
    uint16_t DeviceId;
    uint32_t Abar;
    int Found;
} AHCI_LOCATION;

AHCI_LOCATION FindAhciController(void);
