CC = gcc
CFLAGS = -c -ffreestanding -fno-stack-protector -mno-red-zone -fno-pie
TARGET = build/kernel.o
ELFTARGET = build/kernel.elf
LINKER = kernel.ld

all: $(ELFTARGET)

$(TARGET): src/kernel.c
	$(CC) $(CFLAGS) src/kernel.c -o $(TARGET)

$(ELFTARGET): $(TARGET) $(LINKER)
	ld -m elf_x86_64 -T $(LINKER) -o $(ELFTARGET) $(TARGET)
	cp $(ELFTARGET) /home/willf/Documents/code/molten/bootloader/esp/EFI/molten/kernel.elf

run: $(ELFTARGET)
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