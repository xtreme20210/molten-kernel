CC = gcc
CFLAGS = -I/usr/include/efi -I/usr/include/efi/x86_64 -ffreestanding -fshort-wchar -mno-red-zone -fpic -fno-stack-protector -c
LDFLAGS = -nostdlib -shared -Bsymbolic -z nocombreloc -T /usr/lib/elf_x86_64_efi.lds -L /usr/lib 
LIBS = -lefi -lgnuefi
TARGET = build/main.o
SOTARGET = build/main.so
BUILD_DIR := build
OBJCOPYFLAGS = -j .rodata -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc -O efi-app-x86_64
EFITARGET = esp/EFI/BOOT/BOOTX64.EFI
OCC = objcopy
FSTARGET = build/filesystem.o
LINUXTARGET = build/linux_efi.o
MTARGET = build/moltenLoader.o
HANDOFFTARGET = build/handoff.o


all: $(EFITARGET)

$(BUILD_DIR):
	mkdir -p $@

$(TARGET): src/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) src/main.c -o $(TARGET) 

$(FSTARGET): src/uefi/filesystem.c
	$(CC) $(CFLAGS) src/uefi/filesystem.c -o $(FSTARGET)

$(LINUXTARGET): src/loaders/linux_efi.c
	$(CC) $(CFLAGS) src/loaders/linux_efi.c -o $(LINUXTARGET)

$(MTARGET): src/loaders/moltenLoader.c
	$(CC) $(CFLAGS) src/loaders/moltenLoader.c -o $(MTARGET)

$(HANDOFFTARGET): src/loaders/handoff.S
	$(CC) -c src/loaders/handoff.S -o $(HANDOFFTARGET)

$(SOTARGET): build/main.o $(FSTARGET) $(LINUXTARGET) $(MTARGET) $(HANDOFFTARGET)
	ld $(LDFLAGS) /usr/lib/crt0-efi-x86_64.o $(TARGET) build/filesystem.o build/linux_efi.o build/moltenLoader.o $(HANDOFFTARGET) -o $(SOTARGET) $(LIBS)

$(EFITARGET): build/main.so
	$(OCC) $(OBJCOPYFLAGS) build/main.so $(EFITARGET)


clean: 
	rm -rf $(BUILD_DIR)
	rm -f $(EFITARGET)

run: $(EFITARGET)
	qemu-system-x86_64 \
    -machine q35 \
    -m 4G \
    -enable-kvm \
	-boot order=c \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    -drive if=pflash,format=raw,file=OVMF_VARS.fd \
    -drive format=raw,file=fat:rw:esp \
	-serial stdio \
	-vga std 

.PHONY: all clean run



