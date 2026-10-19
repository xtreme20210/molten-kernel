#include <efi.h>
#include <efilib.h>
#include "filesystem.h"

typedef struct {
    EFI_HANDLE deviceHandle;
    EFI_FILE_HANDLE root;
} fs;

EFI_STATUS fsLoad(EFI_FILE_HANDLE root, CHAR16 *filePath, VOID *buffer, UINTN *size) {
    EFI_FILE_HANDLE file;
    Print(L"opening test file\r\n");
    EFI_STATUS openStatus = uefi_call_wrapper(
        root->Open, 5, root, &file, filePath, EFI_FILE_MODE_READ, 0
    );
    if(EFI_ERROR(openStatus)) {
        Print(L"failed to open file");
        return EFI_LOAD_ERROR;
    }
    Print(L"opened test file\r\n");
    Print(L"reading test file\r\n");
    uefi_call_wrapper( 
        file->Read, 3, file, size, buffer
    );

    Print(L"%a\r\n", buffer);

    uefi_call_wrapper(
        file->Close, 1, file
    );
}

struct fs fsInit(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_LOADED_IMAGE *loadedImage;
    EFI_STATUS status;
    EFI_BOOT_SERVICES *bootServices = SystemTable->BootServices;
    status = uefi_call_wrapper(
        bootServices->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (VOID **)&loadedImage
    );
    if(EFI_ERROR(status)) {
        Print(L"error getting status");
        return (struct fs){0};
    }
    Print(L"%r\r\n", status);
    Print(L"%p\r\n", loadedImage->ImageBase);
    Print(L"%lu\r\n", loadedImage->ImageSize);

    EFI_FILE_IO_INTERFACE *fileInterface;
    EFI_STATUS devStatus;

    devStatus = uefi_call_wrapper(
        bootServices->HandleProtocol, 3, loadedImage->DeviceHandle, &FileSystemProtocol, (VOID **)&fileInterface
    );
    if(EFI_ERROR(devStatus)) {
        Print(L"error getting status\r\n");
        return (struct fs){0};
    }
    Print(L"%r\r\n", devStatus);

    EFI_FILE_HANDLE root;
    Print(L"opening volume\r\n");
    EFI_STATUS openVolumeStatus = uefi_call_wrapper(
        fileInterface->OpenVolume, 2, fileInterface, &root
    );
    if (openVolumeStatus != EFI_SUCCESS) {
        Print(L"failed to open volume");
        return (struct fs){0};
    }
    Print(L"opened volume\r\n");

    UINTN fileSize = 127;
    CHAR8 buffer[128];

    struct fs fsReturn;
    fsReturn.deviceHandle = loadedImage->DeviceHandle;
    fsReturn.root = root;

    return fsReturn;
}

