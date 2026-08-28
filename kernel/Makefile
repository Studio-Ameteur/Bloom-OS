TARGET = kernel.bin

all: $(TARGET)

start.o: start.asm
	nasm -f elf64 start.asm -o start.o

kernel.o: kernel.c boot_info.h font8x16.h
	gcc -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -m64 -Wall -c kernel.c -o kernel.o

kernel.elf: start.o kernel.o linker.ld
	ld -T linker.ld -o kernel.elf start.o kernel.o

$(TARGET): kernel.elf
	objcopy -O binary kernel.elf $(TARGET)

clean:
	rm -f *.o kernel.elf $(TARGET)

.PHONY: all clean
