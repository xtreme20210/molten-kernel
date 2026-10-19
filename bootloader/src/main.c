#include <efi.h>
#include <efilib.h>
#include "uefi/filesystem.h"
#include "loaders/linux_efi.h"
#include "loaders/moltenLoader.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"efi program success\r\n");

    EFI_STATUS loadStatus = loadMolten(ImageHandle, SystemTable);

    WaitForSingleEvent(SystemTable->ConIn->WaitForKey, 0);
    return EFI_SUCCESS;
} 