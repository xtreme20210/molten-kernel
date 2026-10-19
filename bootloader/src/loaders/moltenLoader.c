#include <efi.h>
#include <efilib.h>
#include <elf.h>
#include "moltenLoader.h"
#include "../uefi/filesystem.h"
#include "handoff.h"

EFI_STATUS loadMolten(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    struct fs fsVars;
    fsVars = fsInit(ImageHandle, SystemTable);
    Print(L"fsInit returned\r\n");
    EFI_FILE_HANDLE kernelHandle;
    EFI_STATUS openStatus = uefi_call_wrapper(
        fsVars.root->Open, 5, fsVars.root, &kernelHandle, L"\\EFI\\molten\\kernel.elf", EFI_FILE_MODE_READ, 0
    );

    if (EFI_ERROR(openStatus)) {
        Print(L"error opening kernel file, %r\r\n", openStatus);
        return (EFI_INVALID_PARAMETER);
    }
    Print(L"opened kernel file\r\n");

    UINT8 buffer[64];
    UINTN size = 4;
    EFI_STATUS readStatus = uefi_call_wrapper(
        kernelHandle->Read, 3, kernelHandle, &size, buffer
    );

    if (size != 4 || EFI_ERROR(readStatus)) {
        Print(L"error reading kernel file, %r\r\n", readStatus);
        return EFI_LOAD_ERROR;
    }

    if (buffer[0] != 0x7F || buffer[1] != 'E' || buffer[2] != 'L' || buffer[3] != 'F') {
        Print(L"not an ELF file");
        return EFI_INVALID_PARAMETER;
    }
    Print(L"kernel file found\r\n");

    EFI_STATUS setPosStatus = uefi_call_wrapper(
        kernelHandle->SetPosition, 2, kernelHandle, 0
    );
    if (EFI_ERROR(setPosStatus)) {
        Print(L"error setting position");
        return EFI_LOAD_ERROR;
    }


    Elf64_Ehdr kernelHeader;
    UINTN bufferSize = sizeof(kernelHeader);
    EFI_STATUS readELFStatus = uefi_call_wrapper(
        kernelHandle->Read, 3, kernelHandle, &bufferSize, &kernelHeader
    );

    if (EFI_ERROR(readELFStatus) || bufferSize != sizeof(kernelHeader)) {
        Print(L"error reading kernel file, %r\r\n", readELFStatus);
    }

    if(kernelHeader.e_ident[EI_CLASS] != ELFCLASS64 || 
        kernelHeader.e_ident[EI_DATA] != ELFDATA2LSB || 
        kernelHeader.e_machine != EM_X86_64 || 
        kernelHeader.e_type != ET_EXEC || 
        kernelHeader.e_phentsize != sizeof(Elf64_Phdr)) {
        Print(L"wrong file headers");
        return EFI_INVALID_PARAMETER;
    }

    for (int i=0; i<kernelHeader.e_phnum; i++) {

        uefi_call_wrapper(
            kernelHandle->SetPosition, 2, kernelHandle, (kernelHeader.e_phoff + kernelHeader.e_phentsize * i)
        );

        Elf64_Phdr header;
        UINTN headerSize = kernelHeader.e_phentsize;
        uefi_call_wrapper(
            kernelHandle->Read, 3, kernelHandle, &headerSize, &header
        );

        Print(L"p_type: %x\r\n", header.p_type);
        Print(L"p_offset: 0x%x\r\n", header.p_offset);
        Print(L"p_vaddr: 0x%x\r\n", header.p_vaddr);
        Print(L"p_paddr: 0x%x\r\n", header.p_paddr);
        Print(L"p_filesz: %d\r\n", header.p_filesz);
        Print(L"p_memsz: %d\r\n", header.p_memsz);

        if (header.p_type == PT_LOAD) {
            Print(L"loading into memory...\r\n");
            EFI_PHYSICAL_ADDRESS address = header.p_paddr;
            EFI_STATUS mAllocStatus = uefi_call_wrapper(
                SystemTable->BootServices->AllocatePages, 4, AllocateAddress, EfiLoaderData, (header.p_memsz + 4096 - 1) / 4096, &address
            );
            if (EFI_ERROR(mAllocStatus)) {
                Print(L"error allocating memory\r\n");
                return EFI_LOAD_ERROR;
            }
            Print(L"successfully allocated memory\r\n");

            UINTN amountRead = header.p_filesz;

            uefi_call_wrapper(
                kernelHandle->SetPosition, 2, kernelHandle, header.p_offset
            );

            EFI_STATUS mLoadStatus = uefi_call_wrapper(
                kernelHandle->Read, 3, kernelHandle, &amountRead, (VOID *)(UINTN)address
            );
            if (EFI_ERROR(mLoadStatus)) {
                Print(L"error loading kernel into memory\r\n");
                return EFI_LOAD_ERROR;
            }
            if (header.p_memsz > header.p_filesz) {
                uefi_call_wrapper(
                    SystemTable->BootServices->SetMem, 3, (VOID *)(UINTN)(address + header.p_filesz), (header.p_memsz - header.p_filesz), 0
                );
            }
            Print(L"loaded segment into memory\r\n");
        }
    }

    EFI_PHYSICAL_ADDRESS stackAddress = 0;
    UINT8 pages = 4;
    EFI_STATUS stackAllocStatus = uefi_call_wrapper(
        SystemTable->BootServices->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, pages, &stackAddress
    );
    if (EFI_ERROR(stackAllocStatus)) {
        Print(L"error allocating stack\r\n");
        return EFI_LOAD_ERROR;
    }
    stackAddress += (pages * 4096);

    UINTN memoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *memoryMap =NULL;
    UINTN mapKey;
    UINTN descriptorSize;
    UINT32 descriptorVersion;
    EFI_STATUS getMapStatus = uefi_call_wrapper(
        SystemTable->BootServices->GetMemoryMap, 5, &memoryMapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion
    );
    if (EFI_ERROR(getMapStatus) && getMapStatus != EFI_BUFFER_TOO_SMALL) {
        Print(L"error on first memory map request\r\n");
        return EFI_LOAD_ERROR;
    }
    memoryMapSize += 8 * sizeof(EFI_MEMORY_DESCRIPTOR);
    UINTN bufferCapacity = memoryMapSize;
    EFI_STATUS buffAllocStatus = uefi_call_wrapper(
        SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, memoryMapSize, &memoryMap
    );
    if (EFI_ERROR(buffAllocStatus)) {
        Print(L"error allocating pool\r\n");
        return EFI_LOAD_ERROR;
    }
    UINT8 retryCounter = 0;
retry:
    EFI_STATUS getMapFinalStatus = uefi_call_wrapper(
        SystemTable->BootServices->GetMemoryMap, 5, &memoryMapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion
    );
    if (EFI_ERROR(getMapFinalStatus)) {
        if (retryCounter == 0) {
            Print(L"error getting memory map\r\n");
        }
        return EFI_LOAD_ERROR;
    }

    // no more efi! 
    if (retryCounter == 0) {
        Print(L"exiting boot services...\r\n");
    }
    EFI_STATUS exitBootStatus = uefi_call_wrapper(
        SystemTable->BootServices->ExitBootServices, 2, ImageHandle, mapKey
    );
    if (EFI_ERROR(exitBootStatus) && exitBootStatus != EFI_INVALID_PARAMETER) {
        return EFI_LOAD_ERROR;
    } else if (exitBootStatus == EFI_INVALID_PARAMETER) {
        retryCounter++;
        if (retryCounter < 5) {
            memoryMapSize = bufferCapacity;
            goto retry;
        }
        return EFI_INVALID_PARAMETER;
    }

    // the moment we've all been waiting for. the kernel takes over.
    kernelHandoff(stackAddress, kernelHeader.e_entry);

}

