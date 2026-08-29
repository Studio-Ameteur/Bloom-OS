#include <stdint.h>
#include "boot_info.h"
#include "font8x16.h"
#include "logo.h"
#include "idt.h"
#include "keyboard.h"

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

static void
FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    for (uint32_t row = 0; row < h; row++) {
        for (uint32_t col = 0; col < w; col++) {
            PutPixel(x + col, y + row, color);
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
PlayPhrase(const uint32_t *Freqs, const uint64_t *Durations, int Count)
{
    uint32_t Divisor = 1193180 / Freqs[0];

    OutB(0x43, 0xB6);
    OutB(0x42, (uint8_t)(Divisor & 0xFF));
    OutB(0x42, (uint8_t)((Divisor >> 8) & 0xFF));

    uint8_t Tmp = InB(0x61);
    OutB(0x61, Tmp | 0x03);

    for (int i = 0; i < Count; i++) {
        int Steps = 6;
        uint32_t StartFreq = Freqs[i];
        uint32_t EndFreq = (i + 1 < Count) ? Freqs[i + 1] : Freqs[i];
        uint64_t StepDuration = Durations[i] / Steps;

        for (int s = 0; s < Steps; s++) {
            uint32_t CurrentFreq = StartFreq + (EndFreq - StartFreq) * s / Steps;
            Divisor = 1193180 / CurrentFreq;
            OutB(0x42, (uint8_t)(Divisor & 0xFF));
            OutB(0x42, (uint8_t)((Divisor >> 8) & 0xFF));
            Sleep(StepDuration);
        }
    }

    Tmp = InB(0x61);
    OutB(0x61, Tmp & 0xFC);
}

static const uint32_t Group1Freqs[] = {220, 262, 330};
static const uint64_t Group1Durations[] = {350, 350, 450};

static const uint32_t Group2Freqs[] = {294, 349, 440};
static const uint64_t Group2Durations[] = {300, 300, 450};

static const uint32_t Group3Freqs[] = {392, 330};
static const uint64_t Group3Durations[] = {300, 300};

static const uint32_t Group4Freqs[] = {523};
static const uint64_t Group4Durations[] = {500};

static const uint32_t Group5Freqs[] = {440, 659, 880};
static const uint64_t Group5Durations[] = {250, 250, 900};

static void
PlayStartupChime(void)
{
    PlayPhrase(Group1Freqs, Group1Durations, 3);
    Sleep(150);

    PlayPhrase(Group2Freqs, Group2Durations, 3);
    Sleep(150);

    PlayPhrase(Group3Freqs, Group3Durations, 2);
    Sleep(200);

    PlayPhrase(Group4Freqs, Group4Durations, 1);
    Sleep(250);

    PlayPhrase(Group5Freqs, Group5Durations, 3);
}

void
kmain(BOOT_INFO *Info)
{
    InitIdt();
    InitKeyboard();
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

    uint32_t Margin = 20;
    uint32_t LineSpacing = 4;
    uint32_t CursorX = Margin;
    uint32_t CursorY = TextY + FONT_HEIGHT + 40;

    #define MAX_LINES 256
    static uint32_t LineEndX[MAX_LINES];
    int CurrentLine = 0;

    for (;;) {
        char c = KeyboardGetChar();

        if (c == 0) {
            __asm__ __volatile__("hlt");
            continue;
        }

        if (c == '\b') {
            if (CursorX > Margin) {
                CursorX -= FONT_WIDTH;
                FillRect(CursorX, CursorY, FONT_WIDTH, FONT_HEIGHT, BackgroundColor);
            } else if (CurrentLine > 0) {
                CurrentLine--;
                CursorY -= FONT_HEIGHT + LineSpacing;
                CursorX = LineEndX[CurrentLine];
            }
        } else if (c == '\n') {
            if (CurrentLine < MAX_LINES - 1) {
                LineEndX[CurrentLine] = CursorX;
                CurrentLine++;
            }
            CursorX = Margin;
            CursorY += FONT_HEIGHT + LineSpacing;
        } else {
            DrawChar(CursorX, CursorY, c, TextColor);
            CursorX += FONT_WIDTH;

            if (CursorX + FONT_WIDTH > ScreenWidth - Margin) {
                if (CurrentLine < MAX_LINES - 1) {
                    LineEndX[CurrentLine] = CursorX;
                    CurrentLine++;
                }
                CursorX = Margin;
                CursorY += FONT_HEIGHT + LineSpacing;
            }
        }
    }
}
