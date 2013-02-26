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

#define BASE        0xF590
#define PATHSTR_LEN    0x9
#define PATHUSB_LEN    0xB //11

#define DEBUG_HOOK 0
#define SYS8_PERMH4 1 // Added on 1.35 to iris

#define SYS8_VERSION 0x207
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
#if SYS8_PERMH4
.org 0x2A8
perm_routine:
    MEM_BASE(%r9)
    LOADI_LABEL2(%r9, perm_mode)
    lwz        %r0, 0(%r9)
    cmplwi  %r0, 1
    beq        return1_h4
    ld      %r9, -0x7B30(%rtoc)
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
    .long BASE + syscall8_unsupported   //10 - sys8_configure ?
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

#if 0
sys8_config:
    b syscall8_unsupported

sys8_lv1call:

    b syscall8_unsupported
#else

sys8_lv1call:
    // system_call_2(8, 11, input_reg_table, output_reg_table) 

    mflr    %r0
	stdu    %sp, -0xA0(%sp)
	std     %r0, 0xB0(%sp)

	std     %r4, 0x60(%sp)

	ld	    %r4, 0x08(%r3)
	ld	    %r5, 0x10(%r3)
	ld	    %r6, 0x18(%r3)
	ld	    %r7, 0x20(%r3)
	ld	    %r8, 0x28(%r3)
	ld	    %r9, 0x30(%r3)
	ld	    %r10,0x38(%r3)
	ld	    %r11,0x40(%r3)
	ld	    %r3, 0x00(%r3)

	sc      1
	
	ld	    %r11, 0x60(%sp)

	std	    %r3, 0x00(%r11)
	std	    %r4, 0x08(%r11)
	std	    %r5, 0x10(%r11)
	std	    %r6, 0x18(%r11)
	std	    %r7, 0x20(%r11)
	std	    %r8, 0x28(%r11)
	std	    %r9, 0x30(%r11)
	std	    %r10,0x38(%r11)
	
	ld	    %r0, 0xB0(%sp)
    addi    %sp, %sp, 0xA0
	mtlr    %r0
	blr
#endif

#if SYS8_PERMH4
perm_mode:
    .long PERM_MODE
#endif
//.org 0x4a0
.org 0x500
