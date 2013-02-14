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
#include "payload_431.h"
#include "syscall8.h"

#include "payload_sky_431_bin.h"
#include "umount_431_bin.h"

#define CONFIG_USE_SYS8PERMH4 1

#undef SYSCALL_BASE 
#undef NEW_POKE_SYSCALL
#undef NEW_POKE_SYSCALL_ADDR
#undef PAYLOAD_OFFSET

#define SYSCALL_BASE                    0x800000000035DBE0ULL
#define NEW_POKE_SYSCALL                813
#define NEW_POKE_SYSCALL_ADDR           0x80000000001B6958ULL   // where above syscall is in lv2
#define PAYLOAD_OFFSET                  0x3d90
#define PERMS_OFFSET                    0x3560
#define UMOUNT_SYSCALL_OFFSET           (0x1B5070 +0x8) // SYSCALL (838)

#define PATCH_JUMP(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc))
#define PATCH_CALL(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc) | 1)

static int lv2_unpatch_bdvdemu_431(void);
static int lv2_patch_bdvdemu_431(uint32_t flags);
static int lv2_patch_storage_431(void);
static int lv2_unpatch_storage_431(void);

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

int is_firm_431(void)
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

int is_payload_loaded_431(void)
{
    u64 addr = peekq((SYSCALL_BASE + 36 * 8));
    syscall_base = SYSCALL_BASE;
    addr = peekq(addr);
    if(peekq(addr - 0x20) == 0x534B313000000000ULL) //SK10 HEADER
        return SKY10_PAYLOAD;

    return ZERO_PAYLOAD;
}

void set_bdvdemu_431(int current_payload)
{
    lv2_unpatch_bdvdemu = lv2_unpatch_bdvdemu_431;
    lv2_patch_bdvdemu   = lv2_patch_bdvdemu_431;
    lv2_patch_storage   = lv2_patch_storage_431;
    lv2_unpatch_storage = lv2_unpatch_storage_431;
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
        pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0xebc2fe887c7f1b78ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x3860032dfba100e8ULL);
        usleep(5000);
    }
}


u8 lv1_peek_poke_call_routines[136] = {
	0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x01, 0x00, 0x10, 0x39, 0x60, 0x00, 0xB6, 0x44, 0x00, 0x00, 0x22, 
	0x7C, 0x83, 0x23, 0x78, 0xE8, 0x01, 0x00, 0x10, 0x7C, 0x08, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20, 
	0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x01, 0x00, 0x10, 0x39, 0x60, 0x00, 0xB7, 0x44, 0x00, 0x00, 0x22, 
	0x38, 0x60, 0x00, 0x00, 0xE8, 0x01, 0x00, 0x10, 0x7C, 0x08, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20, 
	0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x01, 0x00, 0x10, 0x7D, 0x4B, 0x53, 0x78, 0x44, 0x00, 0x00, 0x22, 
	0xE8, 0x01, 0x00, 0x10, 0x7C, 0x08, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x17, 0x0C, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x14, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x17, 0x1C, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x3C, 0x80, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x17, 0x5C, 0x00, 0x00, 0x00, 0x00
};

void load_payload_431(int mode)
{

//    _poke((u32) (SYSCALL_BASE + 8 * 8) ,      0x8000000000001788ULL);
    _poke((u32) (SYSCALL_BASE + 9 * 8) ,      0x8000000000001790ULL);
    _poke((u32) (SYSCALL_BASE + 10 * 8),       0x8000000000001798ULL);
    
    install_lv2_memcpy();
    /* install lv1 peek/poke/call */
    lv2_memcpy(0x800000000000171C,
                   (u64) lv1_peek_poke_call_routines, 
                   sizeof(lv1_peek_poke_call_routines));

    /* WARNING!! It supports only payload with a size multiple of 8 */
    lv2_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_OFFSET,
                   (u64) payload_sky_431_bin, 
                   payload_sky_431_bin_size);

    remove_lv2_memcpy();

    /* BASIC PATCHES SYS36 */
    // by 2 anonymous people
    _poke32(0x0571E8, 0x60000000); // already set in E3 "nop"
    PATCH_JUMP(0x0571F0, 0x57288); // already set in E3
    _poke32(0x05ABAC, 0x60000000); // already set in E3 "nop"
    _poke32(0x05ABC0, 0x60000000); // already set in E3 "nop"
//    lv2poke(0x800000000005ABACULL,0x60000000E8610188ULL); different patch method 
//    lv2poke(0x800000000005ABA0ULL,0x600000005463063EULL);

    _poke(  0x057174, 0x63FF003D60000000);  // fix 8001003D error  "ori     %r31, %r31, 0x3D\n nop\n"
    _poke32(0x05723C, 0x3BE00000);  // fix 8001003E error -- 3.55 ok in 0x055F64 "li      %r31, 0"
    PATCH_JUMP(0x057240, 0x5714C);          // fix E3 4.30 added error
    
    /** Rancid-o: Fix 0x8001003C error (incorrect version in sys_load_param) - It is present in the new game updates **/
    _poke(0x2979E4, 0x386000007C6307B4);
    _poke32(0x2979EC, 0x4E800020);

    /*
        -002c3cf0  f8 01 00 b0 7c 9c 23 78  7c 7d 1b 78 4b d8 aa 1d  |....|.#x|}.xK...|
        +002c3cf0  f8 01 00 b0 7c 9c 23 78  4b d4 01 88 4b d8 aa 1d  |....|.#xK...K...| (openhook jump - 0x3E80)
    */
    
    PATCH_JUMP(0x2C3D04, (PAYLOAD_OFFSET+0xF0)); // patch openhook
   

    /*
        -0035dc20  80 00 00 00 00 33 bf 88  80 00 00 00 00 33 bf 88  |.....3.......3..|
        +0035dc20  80 00 00 00 00 00 41 28  80 00 00 00 00 33 bf 88  |......A(.....3..|

        -0035dd00  80 00 00 00 00 33 bf 88  80 00 00 00 00 33 bf 88  |.....3.......3..|
        +0035dd00  80 00 00 00 00 00 3e 58  80 00 00 00 00 33 bf 88  |......>X.....3..|
    */
    _poke((u32) (SYSCALL_BASE + 8 * 8), 0x8000000000000000ULL + (u64) (PAYLOAD_OFFSET + 0x398)); // syscall_8_desc - sys8
    _poke((u32) (SYSCALL_BASE + 36 * 8), 0x8000000000000000ULL + (u64) (PAYLOAD_OFFSET + 0xC8)); // syscall_map_open_desc - sys36


#ifdef CONFIG_USE_SYS8PERMH4
    PATCH_JUMP(PERMS_OFFSET, (PAYLOAD_OFFSET+0x2a8));
#endif

}


/******************************************************************************************************************************************************/
/* STORAGE FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

static int is_patched = 0;

static u64 save_lv2_storage_patch;
static u64 save_lv1_storage_patches[4];

static int lv2_patch_storage_431(void)
{
    lv1_reg regs_i, regs_o;

    // test if LV1 Peek is supported

    memset(&regs_i, 0, sizeof(regs_i));

    regs_i.reg11 = 0xB6; 
    sys8_lv1_syscall(&regs_i, &regs_o);

    if(((int) regs_o.reg3) <0) {
        return -1;
    }

    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 enable syscall storage
    save_lv2_storage_patch= peekq(0x80000000002E9228ULL);
    pokeq32(0x80000000002E9228ULL, 0x40000000);

    regs_i.reg3 = 0x16fa60; regs_i.reg4 = 0x7f83e37860000000ULL;
    regs_i.reg11 = 0xB6; 
    sys8_lv1_syscall(&regs_i, &regs_o); save_lv1_storage_patches[0]= regs_o.reg4;
    regs_i.reg11 = 0xB7; sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fa84; regs_i.reg4 = 0x7f85e37838600001ULL;
    regs_i.reg11 = 0xB6; 
    sys8_lv1_syscall(&regs_i, &regs_o); save_lv1_storage_patches[1]= regs_o.reg4;
    regs_i.reg11 = 0xB7; sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fafc; regs_i.reg4 = 0x7f84e3783be00001ULL;
    regs_i.reg11 = 0xB6; 
    sys8_lv1_syscall(&regs_i, &regs_o); save_lv1_storage_patches[2]= regs_o.reg4;
    regs_i.reg11 = 0xB7; sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fb04; regs_i.reg4 = 0x9be1007038600000ULL;
    regs_i.reg11 = 0xB6; 
    sys8_lv1_syscall(&regs_i, &regs_o); save_lv1_storage_patches[3]= regs_o.reg4;
    regs_i.reg11 = 0xB7; sys8_lv1_syscall(&regs_i, &regs_o);

    is_patched = 1;

    return 0;
}

static int lv2_unpatch_storage_431(void)
{
    lv1_reg regs_i, regs_o;

    if(!is_patched) return -1;

    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 disable syscall storage
    pokeq(0x80000000002E9228ULL, save_lv2_storage_patch);

    regs_i.reg11 = 0xB7;

    regs_i.reg3 = 0x16fa60; regs_i.reg4 = save_lv1_storage_patches[0];
    sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fa84; regs_i.reg4 = save_lv1_storage_patches[1];
    sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fafc; regs_i.reg4 = save_lv1_storage_patches[2];
    sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fb04; regs_i.reg4 = save_lv1_storage_patches[3];
    sys8_lv1_syscall(&regs_i, &regs_o);

    return 0;
}


/******************************************************************************************************************************************************/
/* BDVDEMU FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

#define LV2MOUNTADDR_431 0x8000000000458098ULL
//0xff0 => 0x116c (458098 - 459204)

extern int noBDVD;

static int lv2_unpatch_bdvdemu_431(void)
{
    int n;
    int flag = 0;
 
    char * mem = temp_buffer;
    memset(mem, 0, 0x116c);
    
    sys8_memcpy( (u64) mem, LV2MOUNTADDR_431, 0x116c);

    for(n = 0; n< 0x116c; n+= 0x118)
    {
        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29))
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_431 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }  
      
        }
        else if(!noBDVD && !memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29)) 
        {
            if(!memcmp(mem + n + 0x69, "dev_bdvd", 9) || !memcmp(mem + n + 0x69, "temp_usb", 9))
            {
                sys8_memcpy(LV2MOUNTADDR_431 + n + 0x69, (u64) (mem + n + 0x79), 11);
                sys8_memset(LV2MOUNTADDR_431 + n + 0x79, 0ULL, 12);
                flag+=10;
            }
        } else if(noBDVD && !memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29))
        {
           if(!memcmp(mem + n + 0x69, "dev_bdvd", 9)  && !memcmp(mem + n + 0x79, "esp_bdvd", 9) && peekq(0x80000000007EF028ULL)==0x494F533A50415441ULL)
           {
               sys8_memcpy(LV2MOUNTADDR_431 + n, 0x80000000007EF020ULL , 0x108);
               _poke32(UMOUNT_SYSCALL_OFFSET, 0xFBA100E8); // UMOUNT RESTORE
               pokeq(0x80000000007EF028ULL, 0ULL);
               flag+=10;
           }
        } else if(!memcmp(mem + n, "CELL_FS_UTILITY:HDD0", 21))
        {
           if(!memcmp(mem + n + 0x69, "dev_bdvd", 9)  && !memcmp(mem + n + 0x79, "esp_bdvd", 9) && peekq(0x80000000007EF028ULL)==0x494F533A50415441ULL)
           {
               sys8_memcpy(LV2MOUNTADDR_431 + n, 0x80000000007EF020ULL , 0x108);
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


static int lv2_patch_bdvdemu_431(uint32_t flags)
{
    int n;
    int flag = 0;
    int usb = -1;
    int pos=-1;
    int pos2 = -1;

    char * mem = temp_buffer;
    memset(mem, 0, 0x116c);

    sys8_memcpy((u64) mem, LV2MOUNTADDR_431, 0x116c);

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

    for(n = 0; n< 0x116c; n+= 0x118)
    {
        if(noBDVD && !memcmp(mem + n, "CELL_FS_UTILITY:HDD1", 21) 
            && !memcmp(mem + n + 0x69, "dev_bdvd", 9))
        {
            pos2 = n;
            
            flag++;
        }
        else 
        if(!noBDVD && !memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29) && (usb >= 0 || !memcmp(mem + n + 0x69, "dev_bdvd", 9)))
        {
            pos2 = n;
            if(usb >= 0)
                sys8_memcpy(LV2MOUNTADDR_431 + n + 0x69, (u64) "temp_bdvd", 10);
            flag++;
        }
        else if(usb >= 0 && !memcmp(mem + n, path_name, 32))
        {
            if(noBDVD) pos = n;
            else {
                sys8_memcpy(LV2MOUNTADDR_431 + n + 0x69, (u64) "dev_bdvd\0\0", 11);
                sys8_memcpy(LV2MOUNTADDR_431 + n + 0x79, (u64) &path_name[128], 11);
            }
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

      sys8_memcpy(0x80000000007EF020ULL , LV2MOUNTADDR_431 + pos2, 0x108);
      dat= LV2MOUNTADDR_431 + (u64) pos2;
      sys8_memcpy(0x80000000007EF000ULL , (u64) &dat, 0x8);
      dat= 0x8000000000000000ULL + (u64)UMOUNT_SYSCALL_OFFSET;
      sys8_memcpy(0x80000000007EF008ULL , (u64) &dat, 0x8);
      n=(int) 0xFBA100E8; // UMOUNT RESTORE
      sys8_memcpy(0x80000000007EF010ULL , (u64) &n, 0x4);
      
      sys8_memcpy(LV2MOUNTADDR_431 + pos2, ((u64) mem) + pos, 0x108);
      
      sys8_memcpy(LV2MOUNTADDR_431 + pos2 + 0x69, (u64) "dev_bdvd\0\0", 11);
      sys8_memcpy(LV2MOUNTADDR_431 + pos2 + 0x79, (u64) "esp_bdvd\0\0", 11);

      sys8_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_OFFSET + 0x500ULL, // copy umount routine
                  (u64) umount_431_bin, 
                  umount_431_bin_size);

      PATCH_CALL(UMOUNT_SYSCALL_OFFSET, (PAYLOAD_OFFSET+0x500)); // UMOUNT ROUTINE PATCH
      
      flag = 100;
    }
    
    if(flag < 11)
        return -1;

    return 0;
}

