#include <stdint.h>
#include "idt.h"

typedef struct __attribute__((packed)) {
    uint16_t OffsetLow;
    uint16_t Selector;
    uint8_t  Ist;
    uint8_t  TypeAttr;
    uint16_t OffsetMid;
    uint32_t OffsetHigh;
    uint32_t Zero;
} IDT_ENTRY;

typedef struct __attribute__((packed)) {
    uint16_t Limit;
    uint64_t Base;
} IDT_POINTER;

typedef struct {
    uint64_t Ip;
    uint64_t Cs;
    uint64_t Flags;
    uint64_t Sp;
    uint64_t Ss;
} INTERRUPT_FRAME;

static IDT_ENTRY Idt[256];
static IDT_POINTER IdtPtr;

volatile uint64_t TickCount = 0;

static void
OutB(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint16_t
ReadCs(void)
{
    uint16_t Cs;
    __asm__ __volatile__("mov %%cs, %0" : "=r"(Cs));
    return Cs;
}

static void
SetGate(int Num, void *Handler, uint16_t Selector, uint8_t TypeAttr)
{
    uint64_t Addr = (uint64_t)Handler;

    Idt[Num].OffsetLow = Addr & 0xFFFF;
    Idt[Num].Selector = Selector;
    Idt[Num].Ist = 0;
    Idt[Num].TypeAttr = TypeAttr;
    Idt[Num].OffsetMid = (Addr >> 16) & 0xFFFF;
    Idt[Num].OffsetHigh = (Addr >> 32) & 0xFFFFFFFF;
    Idt[Num].Zero = 0;
}

static void
RemapPic(void)
{
    OutB(0x20, 0x11);
    OutB(0xA0, 0x11);
    OutB(0x21, 0x20);
    OutB(0xA1, 0x28);
    OutB(0x21, 0x04);
    OutB(0xA1, 0x02);
    OutB(0x21, 0x01);
    OutB(0xA1, 0x01);
    OutB(0x21, 0xFE);
    OutB(0xA1, 0xFF);
}

static void
InitPit(uint32_t Frequency)
{
    uint32_t Divisor = 1193182 / Frequency;

    OutB(0x43, 0x36);
    OutB(0x40, (uint8_t)(Divisor & 0xFF));
    OutB(0x40, (uint8_t)((Divisor >> 8) & 0xFF));
}

__attribute__((interrupt))
static void
TimerHandler(INTERRUPT_FRAME *Frame)
{
    (void)Frame;
    TickCount++;
    OutB(0x20, 0x20);
}

void
InitIdt(void)
{
    uint16_t Cs = ReadCs();

    for (int i = 0; i < 256; i++) {
        SetGate(i, 0, 0, 0);
    }

    SetGate(32, (void *)TimerHandler, Cs, 0x8E);

    RemapPic();
    InitPit(1000);

    IdtPtr.Limit = sizeof(Idt) - 1;
    IdtPtr.Base = (uint64_t)&Idt;
    __asm__ __volatile__("lidt %0" : : "m"(IdtPtr));
}

void
EnableInterrupts(void)
{
    __asm__ __volatile__("sti");
}

void
Sleep(uint64_t Milliseconds)
{
    uint64_t Target = TickCount + Milliseconds;

    while (TickCount < Target) {
        __asm__ __volatile__("hlt");
    }
}
