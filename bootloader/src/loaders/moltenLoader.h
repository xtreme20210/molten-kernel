struct frameBuffer {
    uint64_t fbBase;
    uint64_t fbSize;
    uint32_t hRes;
    uint32_t vRes;
    uint32_t ppLine;
    uint32_t pFormat;
};
EFI_STATUS loadMolten(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);