/*
 * payload_sky.S -- Adds system call 36 and 8 code on custom firmware
 *                  It contains hermes, pl3 and psgroove parts of code
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Aaron Lindsay (Aaron')
 * Copyright (C) (subdub)
 * Copyright (C) (Hermes)
 * Copyright (C) Miguel Boton (Waninkoko)
 * Copyright (C) (D_Skywalk)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 * This payload is a modified version of the original PSJailbreak's payload.
 * The people behing PSJailbrak are the original authors and copyright holders
 * of the code they wrote.
 *
 *******************************************************************************
 *
 * Note some developers with close source projects that does not respect the GPL
 *
 * If you benefit from this code, then you SHOULD contribute back.
 * And it is very clear how I expect you to contribute back - by providing full
 * access to the COMPLETE system which is using my code. At the core of GPL, the
 * intention is that someone else should be able to make modifications to such a
 * system and build other things from it.
 *
 * No source? Syscall8 and new hook is not welcomed in your disturbing projects.
 * I am not interested in any line from your source, but scene play together... 
 *
 * - D_Skywalk, syscall8 + hook_table for 3.55 developer -
 *
 ********************************************************************************
 */

#include "firmware_symbols.h"

#define BASE        0xEF48
#define PATHSTR_LEN    0x9
#define PATHUSB_LEN    0xB //11

#define DEBUG_HOOK 0
#define SYS8_CONFIG 0 // we need research some hash and ptr from 3.55...
#define SYS8_PERMH4 0 // testing, we need this on 3.55?

#define SYS8_VERSION 0x7
#define PERM_MODE 2  // 0 -> restricted 1-> original 2-> new for some games

#define ABS(target) ((target) - (BASE) - .)

// Dynamic macros to load a label into a register
#define MEM_BASE(dest) \
    li dest, 1; \
    rldicr dest, dest, 63, 0;
#define LOAD_LABEL(base, dest, source, address) \
    oris    dest, source, ((base) + (address))@h; \
    ori    dest, dest, ((base) + (address))@l;
#define LOAD_LABEL2(dest, source, address) \
    LOAD_LABEL(BASE, dest, source, address)
#define LOADI_LABEL2(dest, address) \
    LOAD_LABEL2(dest, dest, address)

/* Patch Table Macros */
#define PATCH_DATA(offset, data...) .long offset; .long data
#define PATCH_BRANCH(offset, op, target) .long offset; op (. + (target - offset))

.org 0
destination_code:
return1_h4:
    li      %r3, 1
    blr
return0_h4:
    li      %r3, 0
    blr
.org 0x10
    .byte 'S', 'K', '1','0'

.org 0x30
syscall36_map_dvd:
    stdu    %sp, -0xD0(%sp)
    mflr    %r0
    std    %r0, 0xE0(%sp)
    std    %r31, 0xC8(%sp)
    addi    %r4, %sp, 0x70
    bl    ABS(pathdup_from_user) // strdup %r3 from userspace and store the new pointer into %r1[0x70]
    MEM_BASE(%r31)
    LOADI_LABEL2(%r31, buffer)
    ld    %r3, 0(%r31)
    cmpdi    %r3, 0 // if game_path != NULL: free(game_path)
    beq    l_game_path_null
    li    %r4, 0x27
    bl    ABS(free)

l_game_path_null:
    li    %r4, 0x27
    li    %r3, 0x800
    bl    ABS(alloc)    // alloc (2048)
    std    %r3, 0(%r31)
    ld    %r4, 0x70(%sp)
    bl    ABS(strcpy)
    ld    %r3, 0x70(%sp)
    li    %r4, 0x27
    bl    ABS(free)
    ld    %r3, 0(%r31)
    bl    ABS(strlen)
    ld    %r4, 0(%r31)
    add    %r3, %r4, %r3
    std    %r3, 8(%r31)
    li    %r3, 0    // return 0
    ld    %r31, 0xC8(%sp)
    ld    %r0, 0xE0(%sp)
    addi    %sp, %sp, 0xD0
    mtlr    %r0
    blr

.org 0xC8
    .long 0x80000000
    .long BASE + syscall36_map_dvd
buffer:
    .quad 0
    .quad 0

/*********************************************************************/

.org 0xF0
open_hook:
    //overwriten by the jump
    mr        %r29, %r3 

    MEM_BASE(%r31)
    
    LOAD_LABEL2(%r27, %r31, str_open_cmp)
    ld        %r26, 0(%r27)
    cmpldi    %r26, 0 // if open_table is null go to optimized hook sky 
    beq        hook_open_sky

open_table_loop:
// hermes loop with love :D
    mr      %r3, %r29 //restore ptr? (loop)
    ld        %r4, 0(%r26)  // get compare path (%r27 open_table and %r26 contains the 1st ptr)
    cmpldi    %r4, 0 // check null ptr
    beq        hook_open_sky // null? then we finished...
    
    lwz        %r5, 16(%r26) // get len of compare path
    bl      ABS(strncmp)

    cmpldi    %r3, 0 // if ptr is null
    bne        open_table_nextvalue

    lwz        %r5, 16(%r26) 
    add        %r4, %r29, %r5 // &open_path[compare_path_len]

    ld        %r3, 8(%r26) // get new path
    mr      %r29, %r3    // changes open_path
    lwz        %r5, 20(%r26) // get len of new path

    add        %r3, %r3, %r5 // prepare for copy

    bl        ABS(strcpy)
    b        return_org_open_func

open_table_nextvalue:
    addi    %r26, %r26, 24 // next ptr?
    //mr      %r3, %r29 // move here?
    b        open_table_loop

/*********************************************************************/

hook_open_sky:
//start new code

check_path_dvd:
    LOAD_LABEL2(%r4, %r31, aDev_bdvd)
    li    %r5, PATHSTR_LEN
    bl    ABS(strncmp)
    cmpldi    %r3, 0
    bne    check_path_app //if not bdvd check app_home

    LOADI_LABEL2(%r31, buffer)

    ld    %r3, 0(%r31)
    cmpldi    %r3, 0
    beq    return_org_open_func //if null exit
    ld    %r3, 8(%r31)  //buffer + 1? save len?
    addi    %r4, %r29, PATHSTR_LEN
    bl    ABS(strcpy)
    ld    %r29, 0(%r31)
    b     return_org_open_func //process finished ok, go out now :D

check_path_app:
    mr    %r3, %r29

    LOAD_LABEL2(%r4, %r31, aApp_home)
    li    %r5, PATHSTR_LEN
    bl    ABS(strncmp)
    cmpldi    %r3, 0
    bne    return_org_open_func

    LOADI_LABEL2(%r31, buffer)

    ld    %r3, 0(%r31)
    cmpldi    %r3, 0
    beq    return_org_open_func //if null exit
    ld    %r3, 8(%r31)  //buffer + 1? save len?
    addi    %r4, %r29, PATHSTR_LEN
    bl    ABS(strcpy)
    ld    %r29, 0(%r31)

return_org_open_func: // end
    mr    %r3, %r29 // overwrite
    b    ABS(memory_patch_func)

aDev_bdvd:    .string    "/dev_bdvd"
aApp_home:    .string    "/app_home"

/*********************************************************************/
#if SYS8_CONFIG //you need change all ptrs and jumps, but code is ported
.org 0x200
patching_mem_hermes:
    mflr    %r0
    stdu    %sp, -0x1A0(%sp)
    std     %r27, 0x78(%sp)
    std     %r28, 0x80(%sp)
    std     %r29, 0x88(%sp)
    std     %r30, 0x90(%sp)
    std     %r31, 0x98(%sp)
    std     %r0, 0x1B0(%sp)
    mr      %r29, %r3
    mr      %r30, %r4
    MEM_BASE(%r31)
    ld      %r28, rtoc_entry_2(%rtoc)
    ld      %r28, 0x68(%r28)
    ld      %r28, 0x18(%r28)
    ld      %r27, rtoc_entry_1(%rtoc)
    ld      %r9, 0x18(%r29)
    lwz     %r9, 0x30(%r9)
    rldicl  %r9, %r9, 48,16
    cmpwi   %r9, 0x29
    bne     loc_50E64
    ld      %r4, 0x10(%r28)
    rldicr  %r5, %r4, 24,39
    rldicl  %r5, %r5, 8,56
    cmpwi   %r5, 0xFF
    beq     loc_50E38
    ori     %r4, %r4, 3
    std     %r4, 0x10(%r28)
    li      %r3, 6
    stw     %r3, 0(%r30)
    b       loc_50E48

 loc_50E38:
    ori     %r4, %r4, 2
    std     %r4, 0x10(%r28)
    li      %r3, 0x2C
    stw     %r3, 0(%r30)

 loc_50E48:
    lwz     %r5, 4(%r28)
    ld      %r4, 8(%r28)
    ld      %r3, 0(%r27)
    add     %r9, %r3, %r5
    std     %r9, 0(%r27)
    bl      ABS(memcpy)
    b       loc_50F24

 loc_50E64:

    mr      %r3, %r29
    mr      %r4, %r30
    bl      ABS(unk_4E81C)
    mr      %r29, %r31
    LOADI_LABEL2(%r29, patch_flag)
    lwz     %r3, 0(%r29)
    lwz     %r5, 4(%r28)
    add     %r3, %r3, %r5
    stw     %r3, 0(%r29)
    ld      %r4, 0x10(%r28)
    rldicr  %r5, %r4, 24,39
    rldicl  %r5, %r5, 8,56
    cmpwi   %r5, 0xFF
    bne     loc_50F24
    ld      %r3, 0(%r27)
    li      %r4, 0
    li      %r6, 0

 loc_50EAC:
    add     %r7, %r3, %r4
    lwz     %r5, 0(%r7)
    xor     %r6, %r6, %r5
    addi    %r4, %r4, 4
    cmpldi  %r4, 0x400
    bne     loc_50EAC
    lwz     %r3, 0(%r29)
    rldicr  %r6, %r6, 32,31
    or      %r6, %r6, %r3
    li      %r3, 0
    stw     %r3, 0(%r29)
    mr      %r7, %r31
    LOADI_LABEL2(%r7, memory_patch_table) #hermes

 loc_50EE4:

    ld      %r3, 0(%r7)
    cmpldi  %r3, 0
    beq     loc_50F24
    addi    %r7, %r7, 0x10
    cmpld   %r3, %r6
    bne     loc_50EE4
    ld      %r5, -8(%r7)
    ld      %r7, 0(%r27)

loc_50F04:

    lwz     %r3, 0(%r5)
    cmplwi  %r3, 0
    beq     loc_50F24
    cmplwi  %r3, 1  // skip patch  n bytes (it is a trick used to skip conditional patch)
    beq        is_type_1
    cmplwi  %r3, 2  // non skip patch (it is a trick used to skip conditional patch. In this case patch successfull)
    beq        is_type_2

    lwz     %r4, 4(%r5)
    add     %r3, %r3, %r7
    stw     %r4, 0(%r3)
    addi    %r5, %r5, 8
    b       loc_50F04

is_type_1:
    lwz     %r3, 4(%r5)
    add        %r5, %r5, %r3
    addi    %r5, %r5, 8 // skip n+8 bytes (non patch)
    b        loc_50F04
is_type_2:
    addi    %r5, %r5, 8 // skip 8 bytes (patch)
    b        loc_50F04

loc_50F24:

    li      %r3, 0
    ld      %r27, 0x78(%sp)
    ld      %r28, 0x80(%sp)
    ld      %r29, 0x88(%sp)
    ld      %r30, 0x90(%sp)
    ld      %r31, 0x98(%sp)
    ld      %r0, 0x1B0(%sp)
    addi    %sp, %sp, 0x1A0
    mtlr    %r0
    blr
#endif
/*********************************************************************/
#if SYS8_PERMH4
.org 0x2A8
perm0_routine:
    MEM_BASE(%r9)
    LOADI_LABEL2(%r9, perm_mode)
    lwz        %r0, 0(%r9)
    cmplwi  %r0, 1
    beq        return1_h4
    cmplwi  %r0, 2
    beq        return0_h4
    ld      %r9, -0x7B48(%rtoc)
    mflr    %r0
    b        ABS(perm_patch_func + 8)

perm_routine:
    MEM_BASE(%r9)
    LOADI_LABEL2(%r9, perm_mode)
    lwz        %r0, 0(%r9)
    cmplwi  %r0, 0
    bne        return1_h4
    ld      %r9, -0x7B48(%rtoc)
    mflr    %r0
    b        ABS(perm_patch_func + 8)
#endif
/*********************************************************************/

.org 0x320
syscall8_table:
    .long 0x80000000
    .long BASE + sys8_disable       //0 - not work yet
    .long 0x80000000
    .long BASE + sys8_enable        //1 - not work yet
    .long 0x80000000
    .long BASE + sys8_memcpy        //2
    .long 0x80000000
    .long BASE + sys8_memset        //3
    .long 0x80000000
    .long BASE + sys8_call          //4
    .long 0x80000000
    .long BASE + sys8_alloc         //5
    .long 0x80000000
    .long BASE + sys8_free          //6
    .long 0x80000000
    .long BASE + sys8_stealth       //7 - sys8_panic
    .long 0x80000000
    .long BASE + sys8_perm_mode   //8 - sys8_perm_mode
    .long 0x80000000
    .long BASE + sys8_open_table    //9 
    .long 0x80000000
    .long BASE + sys8_config   //10 - sys8_configure ?
    .long 0x80000000
    .long BASE + sys8_lv1call   //11 - sys8_lv1call
str_open_cmp:
    .quad 0

.org 0x398
syscall_8_desc:
    .long 0x80000000
    .long BASE + syscall8

.org 0x3A0
syscall8:
    mflr    %r0
    stdu    %sp, -0xA0(%sp)
    std     %r27, 0x78(%sp)
    std     %r28, 0x80(%sp)
    std     %r29, 0x88(%sp)
    std     %r30, 0x90(%sp)
    std     %r31, 0x98(%sp)
    std     %r0, 0xB0(%sp)

// hermes stealth code no supported yet
// syscall switch
    andi.   %r3, %r3, 0xff
    cmpldi  %r3, 12
    bge     syscall8_stealth //org unsupported 

    MEM_BASE(%r31)
    LOAD_LABEL2(%r27, %r31, syscall8_table)
    sldi     %r28, %r3, 3
    add     %r27, %r27, %r28

    ld        %r29, 0(%r27)

// move param registers
    mr      %r3, %r4
    mr      %r4, %r5
    mr      %r5, %r6

// jump to register address with link
    bl        syscall8_bl

syscall8_end:
    ld      %r27, 0x78(%sp)
    ld      %r28, 0x80(%sp)
    ld      %r29, 0x88(%sp)
    ld      %r30, 0x90(%sp)
    ld      %r31, 0x98(%sp)
    ld      %r0, 0xB0(%sp)
    addi    %sp, %sp, 0xA0
    mtlr    %r0
    blr

syscall8_stealth:
    lis     %r3, -0x7FFF
    ori     %r3, %r3, 3
    b       syscall8_end

syscall8_unsupported:
    li     %r3, -1
    b      syscall8_end

syscall8_bl:
    mtctr   %r29
    bctr

sys8_disable: // system_call_2(8, 0, key)
    // hermes stealth code no ported yet
    b syscall8_stealth

sys8_enable: // system_call_2(8, 1, key) when syscalls are enabled
    // hermes stealth code no supported yet
    li        %r3, SYS8_VERSION
    blr

sys8_memcpy: // system_call_4(8, 2, dest, src, len)
    b      ABS(memcpy)

sys8_memset: // system_call_4(8, 3, dest, dat, len)
    b      ABS(memset)

sys8_call: // system_call_4(8, 4, addr, param1, param2)
    mtctr   %r3
    mr        %r3, %r4
    mr        %r4, %r5
    bctr

sys8_alloc: // system_call_3(8, 5, size, pool)
    b      ABS(alloc)

sys8_free: // system_call_3(8, 6, ptr, pool)
    b      ABS(free)

#if SYS8_PERMH4
sys8_perm_mode: // system_call_2(8, 8, perm)
    LOAD_LABEL2(%r27, %r31, perm_mode)
    stw        %r3, 0(%r27)
    li        %r3, 0
    blr
#else
sys8_perm_mode:
    b syscall8_unsupported
#endif

sys8_open_table:  // system_call_2(8, 9, ptr)
    mr        %r4, %r3
    LOAD_LABEL2(%r27, %r31, str_open_cmp)
    ld        %r3, 0(%r27)
    std       %r4, 0(%r27)
    blr

#if SYS8_CONFIG
sys8_config: //not finished yet
    li      %r4, 1 // set retail xml
    cmplwi  %r3, 0
    beq     b_configure
    li      %r4, 2 // set debug xml
    cmplwi  %r3, 1
    beq     b_configure
 b_configure:
    LOAD_LABEL2(%r27, %r31,  memory_patch_table_3)
    stw        %r4, 0(%r27)
    LOAD_LABEL2(%r27, %r31,  memory_patch_table_4)
    stw        %r4, 0(%r27)
    li        %r3, 0
    blr
#else
sys8_config:
    b syscall8_unsupported
#endif

sys8_lv1call:
    b syscall8_unsupported

#if SYS8_CONFIG
memory_patch_table:
    // 0x8c0a948c000d99b1 3.41
    .quad    0x0
    .long 0x80000000
    .long BASE + memory_patch_table_3
    // 0xa2bc1a5600052adc 3.41
    .quad    0x0
    .long 0x80000000
    .long BASE + memory_patch_table_4
    .quad    0

// we need revert this changes (already in 3.55 CFW)
memory_patch_table_3:
    PATCH_DATA(2, 4*8) // conditional patch: if first 4 bytes == 1 skip patch if == 2 patch successfull
    //0x0022b888 on 3.41 (we need find them on 3.55)
    PATCH_DATA(0x0, 0x5f746f6f) # "_tool2.xml#root"
    PATCH_DATA(0x0 + 4, 0x6c322e78)
    PATCH_DATA(0x0 + 8, 0x6d6c2372)
    PATCH_DATA(0x0 + 12, 0x6f6f7400)

 skip_end_table_3: // conditional patch: if first 4 bytes == 1 skip patch if == 2 patch successfull
    PATCH_DATA(1, 3*8) // skip 3 patchs
    //0x2367e8 on 3.41 (we need find them on 3.55)
    PATCH_DATA(0x0, 0x2f646576) // "/dev"    // replace "/home_apps/PS3_GAME" by "/dev_bdvd/PS3_GAME"
    PATCH_DATA(0x0 + 4, 0x5f626476) // "_bdv"
    PATCH_DATA(0x0 + 8, 0x642f5053) // "d/PS"
    .long 0

// we need revert this changes (already in 3.55 CFW)
memory_patch_table_4:
    PATCH_DATA(2, 4*8) // conditional patch: if first 4 bytes == 1 skip patch if == 2 patch successfull
    //0x000d68b8 on 3.41 (we need find them on 3.55)
    PATCH_DATA(0x0, 0x5f746f6f) # "_tool2.xml#root"
    PATCH_DATA(0x0 + 4, 0x6c322e78)
    PATCH_DATA(0x0 + 8, 0x6d6c2372)
    PATCH_DATA(0x0 + 12, 0x6f6f7400)
    .long 0

patch_flag: 
    .long 0
#endif
#if SYS8_PERMH4
perm_mode:
    .long PERM_MODE
#endif
.org 0x5B0
/* next lv2 code
    stdu    %sp, -0x80(%sp)
*/

