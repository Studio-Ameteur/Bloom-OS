#pragma once

#include <stdint.h>

static inline void
OutB(uint16_t Port, uint8_t Value)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(Value), "Nd"(Port));
}

static inline uint8_t
InB(uint16_t Port)
{
    uint8_t Ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(Ret) : "Nd"(Port));
    return Ret;
}

static inline void
OutL(uint16_t Port, uint32_t Value)
{
    __asm__ __volatile__("outl %0, %1" : : "a"(Value), "Nd"(Port));
}

static inline uint32_t
InL(uint16_t Port)
{
    uint32_t Ret;
    __asm__ __volatile__("inl %1, %0" : "=a"(Ret) : "Nd"(Port));
    return Ret;
}
