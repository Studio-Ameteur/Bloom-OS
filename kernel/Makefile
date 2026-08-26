TARGET = kernel.bin

all: $(TARGET)

$(TARGET): kernel.asm
	nasm -f bin kernel.asm -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
