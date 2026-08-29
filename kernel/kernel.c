#include <stdint.h>
#include "boot_info.h"
#include "font8x16.h"
#include "logo.h"
#include "idt.h"

static volatile uint32_t *FrameBuffer;
static uint64_t ScreenWidth;
static uint64_t ScreenHeight;
static uint64_t Stride;

static void
PutPixel(uint32_t x, uint32_t y, uint32_t color)
{
    FrameBuffer[y * Stride + x] = color;
}

static void
FillScreen(uint32_t color)
{
    for (uint64_t y = 0; y < ScreenHeight; y++) {
        for (uint64_t x = 0; x < ScreenWidth; x++) {
            PutPixel(x, y, color);
        }
    }
}

static void
DrawChar(uint32_t x, uint32_t y, char ch, uint32_t color)
{
    if (ch < FONT_FIRST_CHAR || ch > FONT_LAST_CHAR) {
        return;
    }

    const uint8_t *Glyph = Font8x16[ch - FONT_FIRST_CHAR];

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t Bits = Glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (Bits & (1 << (7 - col))) {
                PutPixel(x + col, y + row, color);
            }
        }
    }
}

static void
DrawLogo(uint32_t OffsetX, uint32_t OffsetY)
{
    for (uint32_t y = 0; y < LOGO_HEIGHT; y++) {
        for (uint32_t x = 0; x < LOGO_WIDTH; x++) {
            uint32_t Pixel = BloomLogo[y * LOGO_WIDTH + x];
            uint8_t Alpha = (Pixel >> 24) & 0xFF;
            if (Alpha == 0) {
                continue;
            }
            PutPixel(OffsetX + x, OffsetY + y, Pixel & 0x00FFFFFF);
        }
    }
}

static int
StrLen(const char *s)
{
    int len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

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
PlayTone(uint32_t frequency, uint64_t duration)
{
    uint32_t Divisor = 1193180 / frequency;

    OutB(0x43, 0xB6);
    OutB(0x42, (uint8_t)(Divisor & 0xFF));
    OutB(0x42, (uint8_t)((Divisor >> 8) & 0xFF));

    uint8_t Tmp = InB(0x61);
    OutB(0x61, Tmp | 0x03);

    Sleep(duration);

    Tmp = InB(0x61);
    OutB(0x61, Tmp & 0xFC);
}

static void
PlayStartupChime(void)
{
    PlayTone(523, 150);
    Sleep(60);

    PlayTone(659, 150);
    Sleep(60);

    PlayTone(784, 200);
    Sleep(120);

    PlayTone(1047, 500);
}

void
kmain(BOOT_INFO *Info)
{
    InitIdt();
    EnableInterrupts();

    FrameBuffer = (volatile uint32_t *)Info->FrameBufferBase;
    ScreenWidth = Info->Width;
    ScreenHeight = Info->Height;
    Stride = Info->PixelsPerScanLine;

    uint32_t BackgroundColor = 0x00201040;
    uint32_t TextColor = 0x00E0D0FF;

    FillScreen(BackgroundColor);

    const char *Message = "Welcome to Bloom-OS";
    int Len = StrLen(Message);
    uint32_t TextWidth = Len * FONT_WIDTH;
    uint32_t TextX = (ScreenWidth - TextWidth) / 2;
    uint32_t TextY = (ScreenHeight - FONT_HEIGHT) / 2;

    uint32_t Gap = 30;
    uint32_t LogoX = (ScreenWidth - LOGO_WIDTH) / 2;
    uint32_t LogoY = TextY - Gap - LOGO_HEIGHT;

    DrawLogo(LogoX, LogoY);

    PlayStartupChime();

    for (int i = 0; i < Len; i++) {
        DrawChar(TextX + i * FONT_WIDTH, TextY, Message[i], TextColor);
        Sleep(40);
    }

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
