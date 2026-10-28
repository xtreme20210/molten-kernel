CC = gcc
CFLAGS = -c -ffreestanding -fno-stack-protector -mno-red-zone -fno-pie -Wshadow
TARGET = build/kernel.o
ELFTARGET = build/kernel.elf
LINKER = kernel.ld
MEMTARGET = build/memory.o
RTARGET = build/rendering.o

all: $(ELFTARGET)

$(TARGET): src/kernel.c
	mkdir -p build
	$(CC) $(CFLAGS) src/kernel.c -o $(TARGET)

$(MEMTARGET): src/memory.c
	$(CC) $(CFLAGS) src/memory.c -o $(MEMTARGET)

$(RTARGET): src/rendering.c
	$(CC) $(CFLAGS) src/rendering.c -o $(RTARGET)

$(ELFTARGET): $(TARGET) $(LINKER) $(MEMTARGET) $(RTARGET)
	ld -m elf_x86_64 -T $(LINKER) -o $(ELFTARGET) $(TARGET) $(MEMTARGET) $(RTARGET)
	cp $(ELFTARGET) /home/willf/Documents/code/molten/bootloader/esp/EFI/molten/kernel.elf

clean:
	rm -rf build/