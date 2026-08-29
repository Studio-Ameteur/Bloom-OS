#include <stdint.h>
#include "mouse.h"
#include "idt.h"

static void
OutB(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t
InB(uint16_t port)
{
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void
WaitInputBufferEmpty(void)
{
    while (InB(0x64) & 0x02) {
    }
}

static void
WaitOutputBufferFull(void)
{
    while (!(InB(0x64) & 0x01)) {
    }
}

static void
MouseWrite(uint8_t Value)
{
    WaitInputBufferEmpty();
    OutB(0x64, 0xD4);
    WaitInputBufferEmpty();
    OutB(0x60, Value);
    WaitOutputBufferFull();
    InB(0x60);
}

static volatile int AccumDx = 0;
static volatile int AccumDy = 0;
static volatile uint8_t MouseButtons = 0;
static volatile int MouseDataReady = 0;

static uint8_t PacketBytes[3];
static int PacketIndex = 0;

static void
MouseIrqHandler(void)
{
    uint8_t Data = InB(0x60);

    if (PacketIndex == 0 && !(Data & 0x08)) {
        return;
    }

    PacketBytes[PacketIndex] = Data;
    PacketIndex++;

    if (PacketIndex < 3) {
        return;
    }

    PacketIndex = 0;

    uint8_t Status = PacketBytes[0];

    if (Status & 0xC0) {
        return;
    }

    int Dx = PacketBytes[1];
    int Dy = PacketBytes[2];

    if (Status & 0x10) {
        Dx -= 256;
    }
    if (Status & 0x20) {
        Dy -= 256;
    }

    AccumDx += Dx;
    AccumDy += Dy;
    MouseButtons = Status & 0x07;
    MouseDataReady = 1;
}

void
InitMouse(void)
{
    WaitInputBufferEmpty();
    OutB(0x64, 0xA8);

    WaitInputBufferEmpty();
    OutB(0x64, 0x20);
    WaitOutputBufferFull();
    uint8_t Status = InB(0x60);
    Status |= 0x02;
    Status &= ~0x20;

    WaitInputBufferEmpty();
    OutB(0x64, 0x60);
    WaitInputBufferEmpty();
    OutB(0x60, Status);

    MouseWrite(0xF6);
    MouseWrite(0xF4);

    RegisterIrqHandler(12, MouseIrqHandler);
    UnmaskIrq(12);
}

int
MouseGetDelta(int *Dx, int *Dy, uint8_t *Buttons)
{
    if (!MouseDataReady) {
        return 0;
    }

    *Dx = AccumDx;
    *Dy = AccumDy;
    *Buttons = MouseButtons;

    AccumDx = 0;
    AccumDy = 0;
    MouseDataReady = 0;

    return 1;
}
