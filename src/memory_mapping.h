#ifndef memory_mapping_h
#define memory_mapping_h

// *********************************************
//
//              68K CPU memory mapping
//
// *********************************************

// - https://wiki.neogeodev.org/index.php?title=68k_memory_map

// ***** ROM_BANK1 *****
// 1MB
// vectors table + program ROM
#define ROM_BANK1_START 0x000000
#define ROM_BANK1_END   0x0FFFFF
#define ROM_BANK1_SIZE	(1024*1024)

// swappable vector
#define ROM_VECTOR_TABLE_SIZE	0x80

//***** WORK_RAM *****
// 64KB
// up to 10F2FF: user ram
// 10F300 to 10FFFF: bios ram
#define WORK_RAM_START  0x100000
#define WORK_RAM_END    0x10FFFF
#define WORK_RAM_SIZE	(64*1024)

#define WORK_RAM_MIRROR_START  0x110000
#define WORK_RAM_MIRROR_END    0x1FFFFF

// ***** ROM_BANK2 *****
// 1MB
// also use for bank switch
#define ROM_BANK2_START 0x200000
#define ROM_BANK2_END   0x2FFFFF

// ***** IO_BUS *****
// I/O
#define IO_PORTS_START   0x300000
#define IO_PORTS_END     0x3FFFFF

// ***** I/O
#define REG_P1CNT       0x300000
#define REG_DIPSW       0x300001
#define REG_SYSTYPE		0x300081
#define REG_SOUND       0x320000
#define REG_STATUS_A    0x320001
#define REG_P2CNT       0x340000
#define REG_STATUS_B    0x380000
#define REG_POUTPUT     0x380001
#define REG_CRDBANK     0x380011
#define REG_SLOT        0x380021
#define REG_LEDLATCHES  0x380031
#define REG_LEDDATA     0x380041
#define REG_RTCCTRL     0x380051
#define REG_RESETCC1	0x380061
#define REG_RESETCC2	0x380063
#define REG_RESETCL1	0x380065
#define REG_RESETCL2	0x380067
#define REG_SETCC1		0x3800E1
#define REG_SETCC2		0x3800E3
#define REG_SETCL1		0x3800E5
#define REG_SETCL2		0x3800E7
#define IO_REG_END		REG_SETCL2

// ***** SYSTEM_SETTINGS (outputs only)
#define SYS_REG_START	0x3A0001
#define REG_NOSHADOW    SYS_REG_START
#define REG_SHADOW      0x3A0011
#define REG_SWPBIOS     0x3A0003
#define REG_SWPROM      0x3A0013
#define REG_CRDUNLOCK1  0x3A0005
#define REG_CRDLOCK1    0x3A0015
#define REG_CRDLOCK2    0x3A0007
#define REG_CRDUNLOCK2  0x3A0017
#define REG_CRDREGSEL   0x3A0009
#define REG_CRDNORMAL   0x3A0019
#define REG_BRDFIX      0x3A000B
#define REG_CRTFIX      0x3A001B
#define REG_SRAMLOCK    0x3A000D
#define REG_SRAMULOCK   0x3A001D
#define REG_PALBANK1    0x3A000F
#define REG_PALBANK0    0x3A001F
#define SYS_REG_END		REG_PALBANK0

// ***** VIDEO_BUS *****
#define VID_REG_START	0x3C0000
#define REG_VRAMADDR    VID_REG_START
#define REG_VRAMRW      0x3C0002
#define REG_VRAMMOD     0x3C0004
#define REG_LSPCMODE    0x3C0006
#define REG_TIMERHIGH   0x3C0008
#define REG_TIMERLOW    0x3C000A
#define REG_IRQACK      0x3C000C
#define REG_TIMERSTOP   0x3C000E
#define VID_REG_END		REG_TIMERSTOP

// ***** PALETTES RAM *****
// 8KB
#define PALETTES_RAM_START  0x400000
#define PALETTES_RAM_END    0x401FFF
#define PALETTES_RAM_SIZE	(8*1024)

#define PALETTES_RAM_MIRROR_START  0x402000
#define PALETTES_RAM_MIRROR_END    0x403FFF

// ***** MEMCARD *****
// 8MB max
#define MEMCARD_START   0x800000
#define MEMCARD_END     0xBFFFFF
#define MEMCARD_SIZE	(8*1024*1024)

// ***** SYSTEM_ROM *****
// 128KB
#define SYSTEM_ROM_START	0xC00000
#define SYSTEM_ROM_END		0xC1FFFF
#define SYSTEM_ROM_SIZE		(128*1024)

#define SYSTEM_ROM_MIRROR_START  0xC20000
#define SYSTEM_ROM_MIRROR_END    0xC3FFFF

// ***** BACKUP_RAM *****
// 64KB
#define BACKUP_RAM_START    0xD00000
#define BACKUP_RAM_END      0xD0FFFF
#define BACKUP_RAM_SIZE		(64*1024)

#define BACKUP_RAM_MIRROR_START    0xD10000
#define BACKUP_RAM_MIRROR_END      0xD3FFFF

//*****************************************
//              Z80 MEMORY MAPPING
//*****************************************
// ROM: first 32KB directly accessible
#define Z80_M1_ROM_START	0x0000
#define Z80_M1_ROM_SIZE		0x8000

// BANK3: 16KB
#define Z80_BANK3       3
#define Z80_BANK3_OFFSET    0x8000
#define Z80_BANK3_SIZE      0x4000

// BANK2: 8KB
#define Z80_BANK2       2
#define Z80_BANK2_OFFSET    0xC000
#define Z80_BANK2_SIZE      0x2000

// BANK1: 4KB
#define Z80_BANK1       1
#define Z80_BANK1_OFFSET    0xE000
#define Z80_BANK1_SIZE      0x1000

// BANK0: 2KB
#define Z80_BANK0       0
#define Z80_BANK0_OFFSET    0xF000
#define Z80_BANK0_SIZE      0x0800

// RAM: 2KB
#define Z80_RAM_OFFSET      0xF800
#define Z80_RAM_SIZE        0x0800


#endif /* memory_mapping_h */
