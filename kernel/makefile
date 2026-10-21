CC = gcc
CFLAGS = -c -ffreestanding -fno-stack-protector -mno-red-zone -fno-pie -Wshadow
TARGET = build/kernel.o
ELFTARGET = build/kernel.elf
LINKER = kernel.ld

all: $(ELFTARGET)

$(TARGET): src/kernel.c
	$(CC) $(CFLAGS) src/kernel.c -o $(TARGET)

$(ELFTARGET): $(TARGET) $(LINKER)
	ld -m elf_x86_64 -T $(LINKER) -o $(ELFTARGET) $(TARGET)
	cp $(ELFTARGET) /home/willf/Documents/code/molten/bootloader/esp/EFI/molten/kernel.elf