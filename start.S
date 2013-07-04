#define SYS_INIT_SP_ADDR   0xC8012000

.globl _start
_start:
	b	reset
	
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq

_undefined_instruction:
	.word undefined_instruction
_software_interrupt:
	.word software_interrupt
_prefetch_abort:
	.word prefetch_abort
_data_abort:
	.word data_abort
_not_used:
	.word not_used
_irq:
	.word irq
_fiq:
	.word fiq

	.balignl 16,0xdeadbeef

/*
 * Startup Code (reset vector)
 */

.globl _TEXT_BASE
_TEXT_BASE:
	.word	SYS_TEXT_BASE

reset:
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0,cpsr
	bic	r0,r0,#0x1f
	orr	r0,r0,#0xd3
	msr	cpsr,r0

/* Set stackpointer in internal RAM to call board_init_f */
call_board_init_f:
	ldr	sp, =(SYS_INIT_SP_ADDR)
	bic	sp, sp, #7 /* 8-byte alignment for ABI compliance */
	ldr	r0,=0x00000000
	bl	main

/*
 * exception handlers
 */
	.align  5
undefined_instruction:
	/* dummy */
	bl	undefined_instruction

	.align	5
software_interrupt:
	/* dummy */
	bl	software_interrupt

	.align	5
prefetch_abort:
	/* dummy */
	bl	prefetch_abort

	.align	5
data_abort:
	/* dummy */
	bl	data_abort

	.align	5
not_used:
	/* dummy */
	bl	not_used

	.align	5
irq:
	/* dummy */
	bl	irq

	.align	5
fiq:
	/* dummy */
	bl	fiq
