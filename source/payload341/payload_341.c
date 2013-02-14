/*                                                                                                                                                                                 
 * Copyright (C) 2010 drizzt
 *
 * Authors:
 * drizzt <drizzt@ibeglab.org>
 * flukes1
 * kmeaw
 * D_Skywalk
 * Estwald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ppu-lv2.h>
#include "payload.h"
#include "payload_341.h"
#include "syscall8.h"
#include "hvcall_341.h"
#include "mm_341.h"

#include "payload_groove_hermes_bin.h"
#include "umount_341_bin.h"

#define CONFIG_USE_SYS8PERMH4 1

#undef SYSCALL_BASE 
#undef NEW_POKE_SYSCALL
#undef NEW_POKE_SYSCALL_ADDR
#undef PAYLOAD_OFFSET

#define SYSCALL_BASE                    0x80000000002EB128ULL
#define NEW_POKE_SYSCALL                813
#define NEW_POKE_SYSCALL_ADDR           0x80000000001BB93CULL   // where above syscall is in lv2
#define PAYLOAD_OFFSET                  0x7e0000 //0x50B3C

#define PAYLOAD_OFFSET_UMOUNT           (0x50B3C + 0x14)
#define PERMS_OFFSET                    0x505d0
#define UMOUNT_SYSCALL_OFFSET           (0x1B9218 +0x8) // SYSCALL (838)

#define PATCH_JUMP(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc))
#define PATCH_CALL(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc) | 1)

static int lv2_unpatch_bdvdemu_341(void);
static int lv2_patch_bdvdemu_341(uint32_t flags);
static int lv2_patch_storage_341(void);
static int lv2_unpatch_storage_341(void);

static int poke_syscall = 7;

extern char path_name[MAXPATHLEN];
extern char temp_buffer[8192];

static u64 peekq(u64 addr)
{
    lv2syscall1(6, addr);
    return_to_user_prog(u64);

}


static void pokeq(u64 addr, u64 val)
{
    lv2syscall2(poke_syscall, addr, val);
}

static void lv1_poke(u64 address, u64 value)
{
    lv2syscall2(7, HV_BASE_341 + address, value);
}

static void pokeq32(u64 addr, uint32_t val)
{
    uint32_t next = peekq(addr) & 0xffffffff;
    pokeq(addr, (((u64) val) << 32) | next);
}

static inline void _poke(u64 addr, u64 val)
{
    pokeq(0x8000000000000000ULL + addr, val);
}

static inline void _poke32(u64 addr, uint32_t val)
{
    pokeq32(0x8000000000000000ULL + addr, val);
}

int is_firm_341(void)
{
    u64 addr = peekq((SYSCALL_BASE + NEW_POKE_SYSCALL * 8));
    // check address first
    if(addr < 0x8000000000000000ULL || addr > 0x80000000007FFFFFULL || (addr & 3)!=0)
        return 0;
    addr = peekq(addr);

    if(addr == NEW_POKE_SYSCALL_ADDR) return 1;

    return 0;
}

extern u64 syscall_base;

int is_payload_loaded_341(void)
{

    u64 v = peekq(0x8000000000017CD0ULL);
    syscall_base = SYSCALL_BASE;
        
    if(v != 0x4E8000203C608001ULL) return HERMES_PAYLOAD; // if syscall 8 is modified payload is resident...

    return ZERO_PAYLOAD;
}

void set_bdvdemu_341(int current_payload)
{
    lv2_unpatch_bdvdemu = lv2_unpatch_bdvdemu_341;
    lv2_patch_bdvdemu   = lv2_patch_bdvdemu_341;
    lv2_patch_storage = lv2_patch_storage_341;
    lv2_unpatch_storage = lv2_unpatch_storage_341;
}

static inline void lv2_memcpy( u64 to, const u64 from, size_t sz)
{
    lv2syscall3(NEW_POKE_SYSCALL, to, from, sz);
}

static inline void lv2_memset( u64 dst, const u64 val, size_t sz)
{

    u64 *tmp = memalign(32, (sz*(sizeof(u64))) );
    if(!tmp)
        return;

    memset(tmp, val, sz);
    
    lv2syscall3(NEW_POKE_SYSCALL, dst, (u64) tmp, sz);

    free(tmp);
}

/*
-- 3.55
00195A68  F8 21 FF 01 7C 08 02 A6  FB C1 00 F0 FB E1 00 F8
00195A78  EB C2 FE 28 7C 7F 1B 78  38 60 03 2D FB A1 00 E8

-- 4.30
001B6950  F8 21 FF 01 7C 08 02 A6  FB C1 00 F0 FB E1 00 F8
001B6960  EB C2 FE 88 7C 7F 1B 78  38 60 03 2D FB A1 00 E8
*/

static inline void install_lv2_memcpy()
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 50; n++) {

    /* install memcpy */
    /* This does not work on some PS3s */
        pokeq(NEW_POKE_SYSCALL_ADDR, 0x4800000428250000ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4182001438a5ffffULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0x7cc428ae7cc329aeULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x4bffffec4e800020ULL);
        usleep(5000);
    }
}

static inline void remove_lv2_memcpy()
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 50; n++) {
    /* restore syscall */
    //remove_new_poke();
   
        pokeq(NEW_POKE_SYSCALL_ADDR, 0xF821FF017C0802A6ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0xFBC100F0FBE100F8ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0xEBC205407C7F1B78ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x3860032dfba100e8ULL);
        usleep(5000);
    }

}

static void install_new_poke(void)
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 50; n++) {
      
        // install poke with icbi instruction
        pokeq(NEW_POKE_SYSCALL_ADDR, 0xF88300007C001FACULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4C00012C4E800020ULL);
        usleep(5000);
    }

    poke_syscall = NEW_POKE_SYSCALL;
}

static void remove_new_poke(void)
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 50; n++) {
    
        pokeq(NEW_POKE_SYSCALL_ADDR, 0xF821FF017C0802A6ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0xFBC100F0FBE100F8ULL);
        usleep(5000);
    }
}

static u64 mmap_lpar_addr;

#define LV1_PATCH_ADDR 0x16f000ULL

static int map_lv1(void)
{
    int result = lv1_undocumented_function_114_341(0, 0xC, HV_SIZE_341, &mmap_lpar_addr);
    if (result != 0) {
        return 0;
    }

    result = mm_map_lpar_memory_region_341(mmap_lpar_addr, HV_BASE_341, HV_SIZE_341, 0xC, 0);
    if (result) {
        return 0;
    }

    return 1;
}

static void unmap_lv1(void)
{
    if (mmap_lpar_addr != 0)
        lv1_undocumented_function_115_341(mmap_lpar_addr);
}

static int lv2_call_payload(u64 addr)
{
    lv2syscall3(9, (u64) addr, 0,0); /* call new syscall and jump to exploit */
    return_to_user_prog(int);
}

void load_payload_341(int mode)
{
    install_lv2_memcpy();
    /* WARNING!! It supports only payload with a size multiple of 8 */
    
    lv2_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_OFFSET,
                   (u64) payload_groove_hermes_bin, 
                   payload_groove_hermes_bin_size);

    u64 data= 0x7C6903A64E800420ULL;
    lv2_memcpy(0x8000000000017CE0ULL, (u64) &data, 8);

    remove_lv2_memcpy();
    usleep(250000);
    __asm__("sync");
    lv2_call_payload(0x80000000007e0000ULL);
    usleep(250000);

    /** Rancid-o: Fix 0x8001003C error (incorrect version in sys_load_param) - It is present in the new game updates **/
    _poke(0x2821FC, 0x386000007C6307B4);
    _poke32(0x282204, 0x4E800020);

}


/******************************************************************************************************************************************************/
/* STORAGE FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

static int is_patched = 0;

static u64 save_lv2_storage_patch;
static u64 save_lv1_storage_patches[4];

static int lv2_patch_storage_341(void)
{
    install_new_poke();
    if (!map_lv1()) {
        remove_new_poke();
        return -1;
    }
     
    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 enable syscall storage
    save_lv2_storage_patch= peekq(0x80000000002CF880ULL);
    save_lv1_storage_patches[0] = peekq(HV_BASE_341 + 0x16f3b8ULL);
    save_lv1_storage_patches[1] = peekq(HV_BASE_341 + 0x16f3dcULL);
    save_lv1_storage_patches[2] = peekq(HV_BASE_341 + 0x16f454ULL);
    save_lv1_storage_patches[3] = peekq(HV_BASE_341 + 0x16f45cULL);

    int n;
    for(n = 0; n < 20; n++) {
        pokeq32(0x80000000002CF880ULL, 0x40000000);

        lv1_poke(0x16f3b8ULL, 0x7f83e37860000000ULL);
        lv1_poke(0x16f3dcULL, 0x7f85e37838600001ULL);
        lv1_poke(0x16f454ULL, 0x7f84e3783be00001ULL);
        lv1_poke(0x16f45cULL, 0x9be1007038600000ULL);
        usleep(5000);
    }
     
    remove_new_poke();
    unmap_lv1();

    is_patched = 1;

    return 0;

}

static int lv2_unpatch_storage_341(void)
{
    if(!is_patched) return -1;

    install_new_poke();
    if (!map_lv1()) {
        remove_new_poke();
        return -1;
    }

    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 disable syscall storage

    int n;
    for(n = 0; n < 20; n++) {
        pokeq(0x80000000002CF880ULL, save_lv2_storage_patch);

        lv1_poke(0x16f3b8ULL, save_lv1_storage_patches[0]);
        lv1_poke(0x16f3dcULL, save_lv1_storage_patches[1]);
        lv1_poke(0x16f454ULL, save_lv1_storage_patches[2]);
        lv1_poke(0x16f45cULL, save_lv1_storage_patches[3]);
        usleep(5000);
    }
 
    remove_new_poke();
    unmap_lv1();

    return 0;

}

/******************************************************************************************************************************************************/
/* BDVDEMU FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

#define LV2MOUNTADDR_341 0x80000000003EE504ULL
//0xff0 => 0x116c (458098 - 459204)

static int lv2_unpatch_bdvdemu_341(void)
{
    int n;
    int flag = 0;
 
    char * mem = temp_buffer;
    memset(mem, 0, 0xff0);
    
    sys8_memcpy( (u64) mem, LV2MOUNTADDR_341, 0xff0);

    for(n = 0; n< 0xff0; n+= 0x100)
    {
        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29))
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_341 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }  
      
        }
        else if(!memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29)) 
        {
            if(!memcmp(mem + n + 0x69, "dev_bdvd", 9) || !memcmp(mem + n + 0x69, "temp_usb", 9))
            {
                sys8_memcpy(LV2MOUNTADDR_341 + n + 0x69, (u64) (mem + n + 0x79), 11);
                sys8_memset(LV2MOUNTADDR_341 + n + 0x79, 0ULL, 12);
                flag+=10;
            }
        }
        else if(!memcmp(mem + n, "CELL_FS_UTILITY:HDD0", 21))
        {
           if(!memcmp(mem + n + 0x69, "dev_bdvd", 9)  && !memcmp(mem + n + 0x79, "esp_bdvd", 9) && peekq(0x80000000007EF028ULL)==0x494F533A50415441ULL)
           {
               sys8_memcpy(LV2MOUNTADDR_341 + n, 0x80000000007EF020ULL , 0xF4);
               _poke32(UMOUNT_SYSCALL_OFFSET, 0xFBA100E8); // UMOUNT RESTORE
               pokeq(0x80000000007EF028ULL, 0ULL);
               flag+=10;
           }
        }
    }
    
    if((mem[0] == 0) && (flag == 0))
        return -1;
    else
        return flag;

}


static int lv2_patch_bdvdemu_341(uint32_t flags)
{
    int n;
    int flag = 0;
    int usb = -1;
    int pos=-1;
    int pos2 = -1;

    char * mem = temp_buffer;
    memset(mem, 0, 0xff0);

    sys8_memcpy((u64) mem, LV2MOUNTADDR_341, 0xff0);

    for(n = 0; n < 11; n++) 
    {
        if(flags == (1 << n))
        {
            usb = n - 1;
            break;
        }
    }
    
    if(usb >= 0) {
        sprintf(path_name, "CELL_FS_IOS:USB_MASS_STORAGE00%c", 48 + usb);
        sprintf(&path_name[128], "dev_usb00%c", 48 + usb);
    }


    for(n = 0; n< 0xff0; n+= 0x100)
    {
        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29))
        {
            pos2 = n;
       
            if(usb >= 0)
                sys8_memcpy(LV2MOUNTADDR_341 + n + 0x69, (u64) "temp_bdvd", 10);
            flag++;
        }
        else if(usb >= 0 && !memcmp(mem + n, path_name, 32) && (usb >= 0 || !memcmp(mem + n + 0x69, "dev_bdvd", 9)))
        {
            sys8_memcpy(LV2MOUNTADDR_341 + n + 0x69, (u64) "dev_bdvd\0\0", 11);
            sys8_memcpy(LV2MOUNTADDR_341 + n + 0x79, (u64) &path_name[128], 11);
            flag+=10;
        }
        else if(usb < 0 && !memcmp(mem + n, "CELL_FS_UTILITY:HDD0", 21)
                && !memcmp(mem + n + 0x48, "CELL_FS_UFS", 11)
                && !memcmp(mem + n + 0x69, "dev_hdd0", 9))
        {
            pos = n;
        }
    }

    if(pos>0 && pos2>0) {
      u64 dat;

      sys8_memcpy(0x80000000007EF020ULL , LV2MOUNTADDR_341 + pos2, 0xf4);
      dat= LV2MOUNTADDR_341 + (u64) pos2;
      sys8_memcpy(0x80000000007EF000ULL , (u64) &dat, 0x8);
      dat= 0x8000000000000000ULL + (u64)UMOUNT_SYSCALL_OFFSET;
      sys8_memcpy(0x80000000007EF008ULL , (u64) &dat, 0x8);
      n=(int) 0xFBA100E8; // UMOUNT RESTORE
      sys8_memcpy(0x80000000007EF010ULL , (u64) &n, 0x4);
      
      sys8_memcpy(LV2MOUNTADDR_341 + pos2, ((u64) mem) + pos, 0xf4);
      
      sys8_memcpy(LV2MOUNTADDR_341 + pos2 + 0x69, (u64) "dev_bdvd\0\0", 11);
      sys8_memcpy(LV2MOUNTADDR_341 + pos2 + 0x79, (u64) "esp_bdvd\0\0", 11);

      sys8_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_OFFSET_UMOUNT, // copy umount routine
                  (u64) umount_341_bin, 
                  umount_341_bin_size);

      PATCH_CALL(UMOUNT_SYSCALL_OFFSET, (PAYLOAD_OFFSET_UMOUNT)); // UMOUNT ROUTINE PATCH
      
      flag = 100;
    }
    
    
    if(flag < 11)
        return -1;

    return 0;
}

