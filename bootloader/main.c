#include <efi.h>
#include <efilib.h>
#include "logo.h"

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

    Print(L"GOP found.\r\n");
    Print(L"Current mode: %dx%d, PixelFormat=%d\r\n",
        Gop->Mode->Info->HorizontalResolution,
        Gop->Mode->Info->VerticalResolution,
        Gop->Mode->Info->PixelFormat);

    UINT32 *FrameBuffer = (UINT32 *)Gop->Mode->FrameBufferBase;
    UINT32 PixelsPerScanLine = Gop->Mode->Info->PixelsPerScanLine;
    UINT32 ScreenWidth = Gop->Mode->Info->HorizontalResolution;
    UINT32 ScreenHeight = Gop->Mode->Info->VerticalResolution;

    UINT32 BackgroundColor = 0x00201040;

    for (UINT32 y = 0; y < ScreenHeight; y++) {
        for (UINT32 x = 0; x < ScreenWidth; x++) {
            FrameBuffer[y * PixelsPerScanLine + x] = BackgroundColor;
        }
    }

    UINT32 OffsetX = (ScreenWidth - LOGO_WIDTH) / 2;
    UINT32 OffsetY = (ScreenHeight - LOGO_HEIGHT) / 2;

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

hang:
    while (1) {
        __asm__ __volatile__("hlt");
    }

    return EFI_SUCCESS;
}
