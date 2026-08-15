# Bloom-OS

A hobby operating system built entirely from scratch — custom UEFI bootloader, custom kernel, no existing bootloaders or kernels used as a base. Developed as an open, educational OSDev-style project.

## About

This project aims to build a real, working operating system from the ground up:

- A custom UEFI bootloader (no Limine, no GRUB — written from scratch using UEFI protocols)
- A custom kernel (C / x86_64 Assembly)
- Memory management, interrupts, drivers, and eventually a graphical interface
- Long-term goal: basic networking support

## Target hardware

- Development and testing primarily in QEMU (with OVMF UEFI firmware)
- Real-hardware target: a Lenovo ThinkPad T580, booting from a dedicated SD card via the UEFI Boot Menu, alongside an existing Windows 11 installation

## Development notes

Development of this project is done with the assistance of AI (Anthropic's Claude), used as a technical collaborator throughout the design and implementation process. All architectural decisions and final code are reviewed and directed by the author.

## Status

Early development — bootloader and kernel foundations are in progress. Not yet bootable on real hardware.

## License

This project is released under an open license permitting free use, modification, and redistribution — **commercial sale is not permitted** (free distribution only). See [LICENSE](LICENSE) for full terms.

## Author

Savva Poliakov
