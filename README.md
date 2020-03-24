
# NeoGeo-Libretro

## Introduction

Libretro NeoGeo core. Work in progress.

## Credits

This would not have been possible without the following people generously sharing their code:

* Karl Stenerud - [Musashi 68000 Emulation Core](https://github.com/kstenerud/Musashi)
* Juergen Buchmueller - Z80 Emulation Core
* Jarek Burczynski & Tatsuyuki Satoh - YM2610 Emulation Core
* [The MAME Development Team](http://www.mamedev.org/) - MAME, an invaluable source of knowledge about arcade machines.

## Installation

### Core files

Copy the `neogeo_libretro` library to `RetroArch/cores`.

### The INFO file (cosmetic)

Copy `neogeo_libretro.info` to folder `RetroArch/info`

### Required BIOS Files

To function the core needs a BIOS from a NeoGeo machine. The BIOS files should be installed in a `neogeo` folder under RetroArch's system folder.

> **&#128211; Note:** The hashes are given to help you verify the files have not been tampered with, the emulator doesn't verify them.

#### Zoom ROM

> **&#128211; Note:** Need one in the following table

|Description        | Filename  | SHA1                                     |
|-------------------|-----------|------------------------------------------|
| Y Zoom ROM        | ng-lo.rom | 2b1c719531dac9bb503f22644e6e4236b91e7cfc |
| Y Zoom ROM (MAME) | 000-lo.lo | 5992277debadeb64d1c1c64b0a92d9293eaf7e4a |

#### BIOS

> **&#128211; Note:** You need at least one in the following table. If several BIOSes are available, it will be possible to choose which to run in the Core Options Menu.
The files will be automatically byte swapped if needed. 

| Description                | Filename       | SHA1                                     |
|----------------------------|----------------|------------------------------------------|
| Front Loader BIOS          | neocd_f.rom    | a5f4a7a627b3083c979f6ebe1fabc5d2df6d083b |
| Front Loader BIOS (SMKDAN) | neocd_sf.rom   | c99c44a43bded1bff4570b30b74975601bd3f94e |
| Top Loader BIOS            | neocd_t.rom    | cc92b54a18a8bff6e595aabe8e5c360ba9e62eb5 |
| Top Loader BIOS (SMKDAN)   | neocd_st.rom   | d463b3a322b9674f9e227a21e43898019ce0e642 |
| CDZ BIOS                   | neocd_z.rom    | b0f1c4fa8d4492a04431805f6537138b842b549f |
| CDZ BIOS (SMKDAN)          | neocd_sz.rom   | 41ca1c031b844a46387be783ac862c76e65afbb3 |
| Front Loader BIOS (MAME)   | front-sp1.bin  | 53bc1f283cdf00fa2efbb79f2e36d4c8038d743a |
| Top Loader BIOS (MAME)     | top-sp1.bin    | 235f4d1d74364415910f73c10ae5482d90b4274f |
| CDZ BIOS (MAME)            | neocd.bin      | 7bb26d1e5d1e930515219cb18bcde5b7b23e2eda |
| Universe 3.2               | uni-bioscd.rom | 5158b728e62b391fb69493743dcf7abbc62abc82 |

### Rom Images



### The Core Options Menu

* **Region:** Change your NeoGeo's region. (Changing this will reset the machine)
* **BIOS Select:** Select the BIOS to use here if you have several (Changing this will reset the machine)

## For Developers

### Project Dependencies

* A C compiler
* CMake > 3.1
* zlib
* MSYS (Windows)

The project uses custom cmake finders in the folder `cmakescripts` to locate the libraries.

### Compiling

* Eventually, edit the CFLAGS in CMakeLists.txt to suit your platform (Raspberry Pi...)
* Invoke CMake: `cmake -G "Unix Makefiles" .` or `cmake -G "MSYS Makefiles" .` (Windows)
* If all went well, build: `make -j 4`
* Copy the resulting library in `RetroArch/cores`
* Done! (see the user section for the rest)

## Tested platforms

* x64 / Windows / GCC 9.1
* x64 / Debian Linux / GCC 9.1
* Raspberry Pi 3 / Debian Linux / GCC 8.2
* x64 / macOS / Clang 10.0

## Known problems

