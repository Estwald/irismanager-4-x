/*
 * payload.S -- Adds system call 36 code on custom firmware
 * 
 * It contains pl3 and psgroove parts of code
 */

#include "firmware_symbols.h"

#define BASE			0x2BE4A0
#define ABS(target) ((target) - (BASE) - .)

.org 0
aDS016lx016llx0:.string	"%d%s%016lx%016llx%016llx%s%s%08x%d%1d%1d%1dAAA\n"

.org 0x30
syscall36_map_dvd:
		stdu	%sp, -0xD0(%sp)
		mflr	%r0
		std	%r0, 0xE0(%sp)
		std	%r31, 0xC8(%sp)
		addi	%r4, %sp, 0x70
		bl	ABS(pathdup_from_user) // strdup %r3 from userspace and store the new pointer into %r1[0x70]
		li	%r31, 1
		rldicr	%r31, %r31, 63,0
		oris	%r31, %r31, (BASE + buffer)@h
		ori	%r31, %r31, (BASE + buffer)@l
		ld	%r3, 0(%r31)
		cmpdi	%r3, 0 // if game_path != NULL: free(game_path)
		beq	l_game_path_null
		li	%r4, 0x27
		bl	ABS(free)

l_game_path_null:
		li	%r4, 0x27
		li	%r3, 0x800
		bl	ABS(alloc)	// alloc (2048)
		std	%r3, 0(%r31)
		ld	%r4, 0x70(%sp)
		bl	ABS(strcpy)
		ld	%r3, 0x70(%sp)
		li	%r4, 0x27
		bl	ABS(free)
		ld	%r3, 0(%r31)
		bl	ABS(strlen)
		ld	%r4, 0(%r31)
		add	%r3, %r4, %r3
		std	%r3, 8(%r31)
		li	%r3, 0		// return 0
		ld	%r31, 0xC8(%sp)
		ld	%r0, 0xE0(%sp)
		addi	%sp, %sp, 0xD0
		mtlr	%r0
		blr

buffer:
.org 0xd0
		.long 0x80000000
		.long BASE + syscall36_map_dvd

.org 0x100	# If you change that you also need to change the jump in patch.txt
open_hook:
		stdu	%sp, -0xA0(%sp)
		mflr	%r0
		std	%r28, 0x80(%sp)
		std	%r29, 0x88(%sp)
		std	%r31, 0x98(%sp)
		std	%r26, 0x70(%sp)
		std	%r27, 0x78(%sp)
		std	%r0, 0xB0(%sp)
		mr	%r28, %r4
		mr	%r29, %r3
		li	%r31, 1
		rldicr	%r31, %r31, 63,0
		oris	%r4, %r31, (BASE + aDev_bdvd)@h
		ori	%r4, %r4, (BASE + aDev_bdvd)@l
		li	%r5, 9
		bl	ABS(strncmp)
		cmpldi	%r3, 0
		bne	loc_2BE614
		oris	%r31, %r31, (BASE + buffer)@h
		ori	%r31, %r31, (BASE + buffer)@l
		ld	%r3, 0(%r31)
		cmpldi	%r3, 0
		beq	loc_2BE60C
		ld	%r3, 8(%r31)
		addi	%r4, %r29, 9
		bl	ABS(strcpy)
		ld	%r29, 0(%r31)

loc_2BE60C:
		mr	%r3, %r29
		b	ABS(memory_patch_func)

loc_2BE614:
		mr	%r3, %r29
		li	%r31, 1
		rldicr	%r31, %r31, 63,0
		oris	%r4, %r31, (BASE + aApp_home)@h
		ori	%r4, %r4, (BASE + aApp_home)@l
		li	%r5, 9
		bl	ABS(strncmp)
		cmpldi	%r3, 0
		bne	loc_2BE65C
		oris	%r31, %r31, (BASE + buffer)@h
		ori	%r31, %r31, (BASE + buffer)@l
		ld	%r3, 0(%r31)
		cmpldi	%r3, 0
		beq	loc_2BE65C
		ld	%r3, 8(%r31)
		addi	%r4, %r29, 9
		bl	ABS(strcpy)
		ld	%r29, 0(%r31)

loc_2BE65C:
		mr	%r3, %r29
		b	ABS(memory_patch_func)

aDev_bdvd:	.string	"/dev_bdvd"
aApp_home:	.string	"/app_home"
