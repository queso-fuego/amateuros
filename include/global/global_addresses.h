/*
 * global_addresses.h: Hold definitions for global memory addresses
 */
#pragma once

#define KEY_INFO_ADDRESS               0x1600
#define RTC_DATETIME_AREA              0x1610
#define IRQ0_SLEEP_TIMER_TICKS_AREA    0x1700
#define CURRENT_PAGE_DIR_ADDRESS       0x1800
#define PHYS_MEM_MAX_BLOCKS            0x1804
#define PHYS_MEM_USED_BLOCKS           0x1808
#define GDT_ADDRESS                    0x1810
#define SCRATCH_BLOCK_ADDRESS          0x3000
#define BOOT_BLOCK_ADDRESS             0x7C00
#define SUPERBLOCK_ADDRESS             0x8C00
#define SMAP_NUMBER_ADDRESS            0xA500
#define SMAP_ENTRIES_ADDRESS           0xA504
#define BOOTLOADER_FIRST_INODE_ADDRESS 0xB000
#define VBE_MODE_INFO_ADDRESS          0xC000
#define USER_GFX_INFO_ADDRESS          0xC200
#define FONT_ADDRESS                   0xD000  
#define FONT_WIDTH                     0xD000 
#define FONT_HEIGHT (FONT_WIDTH+1)
#define MEMMAP_AREA                    0x30000
#define KERNEL_ADDRESS                 0x100000 // 1MB
