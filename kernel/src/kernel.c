#include <stdint.h>

void ioout(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}


void kernel_main() {
    ioout(0x3F9, 0x00);
    ioout(0xF3B, 0x80);
    ioout(0xF38, 0x01);
    ioout(0xF39, 0x00);
    ioout(0xF3B, 0x03);
    ioout(0xF3A, 0xC7);
    ioout(0x3FC, 0x03);
    ioout(0x3F8, 'A');
    while (1) {

    }
}