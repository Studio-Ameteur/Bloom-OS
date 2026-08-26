#include <efi.h>
#include <efilib.h>
#include "logo.h"
#include "ring.h"

static UINT32 *FrameBuffer;
static UINT32 PixelsPerScanLine;

static VOID
DrawFilledCircle(INT32 CenterX, INT32 CenterY, INT32 Radius, UINT32 Color)
{
    for (INT32 dy = -Radius; dy <= Radius; dy++) {
        for (INT32 dx = -Radius; dx <= Radius; dx++) {
            if (dx * dx + dy * dy <= Radius * Radius) {
                FrameBuffer[(CenterY + dy) * PixelsPerScanLine + (CenterX + dx)] = Color;
            }
        }
    }
}

static VOID
DrawRing(INT32 CenterX, INT32 CenterY, INT32 CompletedStages)
{
    UINT32 PendingColor = 0x00403060;
    UINT32 CompletedColor = 0x00A070E0;

    for (INT32 i = 0; i < RING_SEGMENTS; i++) {
        UINT32 Color = (i < CompletedStages) ? CompletedColor : PendingColor;
        DrawFilledCircle(CenterX + RingOffsetX[i], CenterY + RingOffsetY[i], 8, Color);
    }
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;

    InitializeLib(ImageHandle, SystemTable);

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    Print(L"Bloom-OS bootloader\r\n");
    Print(L"UEFI firmware initialized. Boot Services active.\r\n");

    Status = uefi_call_wrapper(BS->LocateProtocol, 3, &GraphicsOutputProtocol, NULL, (void **)&Gop);
    if (EFI_ERROR(Status)) {
        Print(L"GOP not found: %r\r\n", Status);
        goto hang;
    }

    FrameBuffer = (UINT32 *)Gop->Mode->FrameBufferBase;
    PixelsPerScanLine = Gop->Mode->Info->PixelsPerScanLine;
    UINT32 ScreenWidth = Gop->Mode->Info->HorizontalResolution;
    UINT32 ScreenHeight = Gop->Mode->Info->VerticalResolution;

    UINT32 BackgroundColor = 0x00201040;

    for (UINT32 y = 0; y < ScreenHeight; y++) {
        for (UINT32 x = 0; x < ScreenWidth; x++) {
            FrameBuffer[y * PixelsPerScanLine + x] = BackgroundColor;
        }
    }

    INT32 CenterX = ScreenWidth / 2;
    INT32 CenterY = ScreenHeight / 2;

    UINT32 OffsetX = CenterX - LOGO_WIDTH / 2;
    UINT32 OffsetY = CenterY - LOGO_HEIGHT / 2;

    for (UINT32 y = 0; y < LOGO_HEIGHT; y++) {
        for (UINT32 x = 0; x < LOGO_WIDTH; x++) {
            UINT32 Pixel = BloomLogo[y * LOGO_WIDTH + x];
            UINT8 Alpha = (Pixel >> 24) & 0xFF;
            if (Alpha == 0) {
                continue;
            }
            FrameBuffer[(OffsetY + y) * PixelsPerScanLine + (OffsetX + x)] = Pixel & 0x00FFFFFF;
        }
    }

    DrawRing(CenterX, CenterY, 0);

    for (INT32 Stage = 1; Stage <= RING_SEGMENTS; Stage++) {
        uefi_call_wrapper(BS->Stall, 1, 150000);
        DrawRing(CenterX, CenterY, Stage);
    }

    Print(L"Boot stages complete.\r\n");

hang:
    while (1) {
        __asm__ __volatile__("hlt");
    }

    return EFI_SUCCESS;
}
