#include <efi.h>
#include <efilib.h>
#include "linux_efi.h"
#include "../uefi/filesystem.h"


EFI_STATUS bootLinux(EFI_SYSTEM_TABLE *SystemTable, EFI_HANDLE ImageHandle) {
    EFI_HANDLE kernelHandle;
    EFI_LOADED_IMAGE *loadedImage;
    CHAR16 kernelArgs[] = L"initrd=\\kernels\\initrd.img root=/dev/mapper/ubuntu--vg-ubuntu--lv ro";
    EFI_HANDLE deviceHandle = fsInit(ImageHandle, SystemTable);
    EFI_DEVICE_PATH *devPath = FileDevicePath(deviceHandle, L"\\kernels\\linux.efi");
    EFI_STATUS loadStatus = uefi_call_wrapper(
        SystemTable->BootServices->LoadImage, 6, FALSE, ImageHandle, devPath, NULL, 0, &kernelHandle
    );
    if(EFI_ERROR(loadStatus)) {
        Print(L"error loading kernel, %r\r\n", loadStatus);
    } else {
        Print(L"kernel loaded successfully\r\n");
    }

    uefi_call_wrapper(
        SystemTable->BootServices->HandleProtocol, 3, kernelHandle, &LoadedImageProtocol, (VOID **)&loadedImage
    );

    Print(L"0x%lx", loadedImage->ImageBase);
    Print(L"%lu", loadedImage->ImageSize);

    loadedImage->LoadOptions = kernelArgs;
    loadedImage->LoadOptionsSize = sizeof(kernelArgs);

    uefi_call_wrapper(
        SystemTable->BootServices->StartImage, 3, kernelHandle, NULL, NULL
    );

    uefi_call_wrapper(
        SystemTable->BootServices->UnloadImage, 1, kernelHandle
    );
    return EFI_SUCCESS;
}