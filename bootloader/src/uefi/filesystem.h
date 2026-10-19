#pragma once
struct fs {
    EFI_HANDLE deviceHandle;
    EFI_FILE_HANDLE root;
};
EFI_STATUS fsLoad(EFI_FILE_HANDLE root, CHAR16 *filePath, VOID *buffer, UINTN *size);
struct fs fsInit(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);