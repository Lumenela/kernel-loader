#include <mpp.h>

#ifndef _KIRKWOOD_H_
#define _KIRKWOOD_H_

typedef signed char 		s8;
typedef unsigned char 	u8;

typedef signed short 		s16;
typedef unsigned short 	u16;

typedef signed int 			s32;
typedef unsigned int 		u32;

typedef unsigned int uint;
typedef unsigned long ulong;

#define MODE_X_DIV	 						 	16

#define INTREG_BASE     					0xd0000000
#define KW_REGISTER(x)      			(KW_REGS_PHY_BASE + x)
#define KW_OFFSET_REG     				(INTREG_BASE + 0x20080)

#define KW_UART0_BASE     				(KW_REGISTER(0x12000))
#define KW_MPP_BASE     					(KW_REGISTER(0x10000))
#define KW_GPIO0_BASE     				(KW_REGISTER(0x10100))
#define KW_GPIO1_BASE     				(KW_REGISTER(0x10140))
#define KW_CPU_REG_BASE     			(KW_REGISTER(0x20100))
#define KW_NANDF_BASE							(KW_REGISTER(0x10418))
#define KW_TIMER_BASE							(KW_REGISTER(0x20300))

#define KW_REGS_PHY_BASE    			0xf1000000

#define CONFIG_SYS_TCLK						200000000

#define WTPLUG_OE_LOW   					(~(0))
#define WTPLUG_OE_HIGH    				(~(0x3 << 15))
#define WTPLUG_OE_VAL_LOW 				0
#define WTPLUG_OE_VAL_HIGH  			(0x3 << 15) /* RST_GE and RST_USB Pins high */

#define UART_LCRVAL 							UART_LCR_8N1    /* 8 data, 1 stop, no parity */
#define UART_MCRVAL (UART_MCR_DTR | UART_MCR_RTS)    /* RTS/DTR */
#define UART_FCRVAL (UART_FCR_FIFO_EN | UART_FCR_RXSR | UART_FCR_TXSR)   /* Clear & enable FIFOs */

#define isb() __asm__ __volatile__ ("" : : : "memory")

/*
 * Generic virtual read/write.  Note that we don't support half-word
 * read/writes.  We define __arch_*[bl] here, and leave __arch_*w
 * to the architecture specific code.
 */
#define __arch_getb(a)      (*(volatile unsigned char *)(a))
#define __arch_getw(a)      (*(volatile unsigned short *)(a))
#define __arch_getl(a)      (*(volatile unsigned int *)(a))

#define __arch_putb(v,a)    (*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v,a)    (*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v,a)    (*(volatile unsigned int *)(a) = (v))

#define dmb()   __asm__ __volatile__ ("" : : : "memory")
#define __iormb() dmb()
#define __iowmb() dmb()

#define writeb(v,c) ({ u8  __v = v; __iowmb(); __arch_putb(__v,c); __v; })
#define writew(v,c) ({ u16 __v = v; __iowmb(); __arch_putw(__v,c); __v; })
#define writel(v,c) ({ u32 __v = v; __iowmb(); __arch_putl(__v,c); __v; })

#define readb(c)  ({ u8  __v = __arch_getb(c); __iormb(); __v; })
#define readw(c)  ({ u16 __v = __arch_getw(c); __iormb(); __v; })
#define readl(c)  ({ u32 __v = __arch_getl(c); __iormb(); __v; })

/*
 * GPIO Registers
 * Ref: Datasheet sec:A.19
 */
struct kwgpio_registers {
  u32 dout;
  u32 oe;
  u32 blink_en;
  u32 din_pol;
  u32 din;
  u32 irq_cause;
  u32 irq_mask;
  u32 irq_level;
};

/*
 * CPU control and status Registers
 * Ref: Datasheet sec:A.3.2
 */
struct kwcpu_registers {
  u32 config; /*0x20100 */
  u32 ctrl_stat;  /*0x20104 */
  u32 rstoutn_mask; /* 0x20108 */
  u32 sys_soft_rst; /* 0x2010C */
  u32 ahb_mbus_cause_irq; /* 0x20110 */
  u32 ahb_mbus_mask_irq; /* 0x20114 */
  u32 pad1[2];
  u32 ftdll_config; /* 0x20120 */
  u32 pad2;
  u32 l2_cfg; /* 0x20128 */
};

#if 0
/*
 * Invalidate L2 Cache using co-proc instruction
 */
static inline void invalidate_l2_cache(void)
{
  unsigned int val=0;

  asm volatile("mcr p15, 1, %0, c15, c11, 0 @ invl l2 cache"
    : : "r" (val) : "cc");
  isb();
}
#endif

/*
 * kw_config_gpio - GPIO configuration
 */
void kw_config_gpio(u32 gpp0_oe_val, u32 gpp1_oe_val, u32 gpp0_oe, u32 gpp1_oe);

#define MPP_CTRL(i) (KW_MPP_BASE + (i* 4))
#define MPP_NR_REGS (1 + MPP_MAX/8)

//void kirkwood_mpp_conf(u32 *mpp_list);

#define udelay __udelay

#endif // _KIRKWOOD_H_
