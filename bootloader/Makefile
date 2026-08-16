ARCH        = x86_64
EFIINC      = /usr/include/efi
EFIINCS     = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB         = /usr/lib
EFILIB      = /usr/lib
EFI_CRT_OBJS = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS     = $(EFILIB)/elf_$(ARCH)_efi.lds

CFLAGS      = $(EFIINCS) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -DEFI_FUNCTION_WRAPPER
LDFLAGS     = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFILIB) $(EFI_CRT_OBJS)

TARGET      = main.efi
OBJS        = main.o

all: $(TARGET)

main.o: main.c
	gcc $(CFLAGS) -c main.c -o main.o

main.so: main.o
	ld $(LDFLAGS) main.o -o main.so -lefi -lgnuefi

$(TARGET): main.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) main.so $(TARGET)

clean:
	rm -f *.o *.so $(TARGET)

.PHONY: all clean
