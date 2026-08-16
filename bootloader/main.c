#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    Print(L"Bloom-OS bootloader\r\n");
    Print(L"UEFI firmware initialized. Boot Services active.\r\n");

    while (1) {
        __asm__ __volatile__("hlt");
    }

    return EFI_SUCCESS;
}
