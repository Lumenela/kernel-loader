#define UART_BAUDRATE								115200
#define CONFIG_SYS_NS16550_REG_SIZE	(-4)

#include <kirkwood.h>
#include <ns16550.h>
#include <nand.h>
#include "ecc.h"

#define NS16550_CLK									CONFIG_SYS_TCLK
static NS16550_t com_port = (NS16550_t)KW_UART0_BASE;

int timer_init(void);
char nand_readb(void);
void nand_command(int, int, int);
void __udelay(unsigned long);

#define ATAG_NONE       0x00000000
#define ATAG_CORE       0x54410001
#define ATAG_MEM        0x54410002
#define ATAG_CMDLINE    0x54410009

static u32 atags[]= {
	/* ATAG_CORE */
	5,
	ATAG_CORE,
	0,
	0,
	0,

	/* ATAG_MEM */
	4,
	ATAG_MEM,
	0x20000000,
	0,

	/* ATAG_CMDLINE */
	19,
	ATAG_CMDLINE,
	0x736e6f63,	// console=ttyS0,115200
	0x3d656c6f,
	0x53797474,
	0x31312c30,
	0x30303235,
	0x69627520,	//  ubi.mtd=3 root=ubi0:rootfs rootfstype=ubifs
	0x64746D2E,
	0x7220333D,
	0x3D746F6F,
	0x30696275,
	0x6F6F723A,
	0x20736674,
	0x746F6F72,
	0x79747366,
	0x753D6570,
	0x73666962,
	0,

	/* ATAG_NONE */
	0,
	ATAG_NONE,
};

void
putc(char c)
{
	if (c == '\n')
		putc('\r');

  while ((readb(&com_port->lsr) & UART_LSR_THRE) == 0);
  writeb(c, &com_port->thr);
}

void
puts(const char *s)
{
	while (*s)
		putc(*s++);
}

void
putc_hex(char c)
{
	char lo, hi;

	lo = c & 0x0f;
	hi = c  >> 4;

	if (lo > 9)
		lo += 'A' - 10;
	else
		lo += '0';

	if (hi > 9)
		hi += 'A' - 10;
	else
		hi += '0';

	putc(hi);
	putc(lo);
}

void
dump_int(u32 value);

void
dump(int ptr, int len)
{
	if (!len || len < 0)
		return;
	
	unsigned long i;
	for (i = 0; i < len; i++) {
		putc('\t');
		unsigned char *pc= (unsigned char *)ptr;
		if (i == 0) {
			dump_int(pc[i]);
			putc('\r');
		}
		putc_hex(pc[i]);
		putc(' ');
		if (!((i + 1) % 16) && i)
			putc('\n');
	}
	putc('\n');
}

void
dump_int(u32 value)
{
	int i;
	for (i = 0; i < 4; i++) {
		putc_hex((value >> ((3 - i) * 8)) & 0xff);
	}
}

int
ec(int value)
{
	return ((((0xff << 0) & value) << 24)
					| (((0xff <<  8) & value) <<  8)
					| (((0xff << 16) & value) >>  8)
					| (((0xff << 24) & value) >> 24));
}

void
hang(void)
{
	puts("### Failure ###\n");
	for (;;);
}

void
main(void)
{
	/* CPU setup */
	struct kwcpu_registers *cpureg =
		(struct kwcpu_registers *)KW_CPU_REG_BASE;

	/* Linux expects` the internal registers to be at 0xf1000000 */
	writel(KW_REGS_PHY_BASE, KW_OFFSET_REG);

	/* GPIO setup */
	kw_config_gpio(WTPLUG_OE_VAL_LOW,
			WTPLUG_OE_VAL_HIGH,
			WTPLUG_OE_LOW, WTPLUG_OE_HIGH);

	u32 kwmpp_config[] = {
		MPP6_SYSRST_OUTn,
		MPP10_UART0_TXD,
		MPP11_UART0_RXD,
		0
	};
	kirkwood_mpp_conf(kwmpp_config);

	/* UART setup */
	int div = (NS16550_CLK + (UART_BAUDRATE * (MODE_X_DIV / 2))) / 
						(MODE_X_DIV * UART_BAUDRATE);

	writeb(0, &com_port->ier);
  writeb(UART_LCR_BKSE | UART_LCRVAL, (u32)&com_port->lcr);
  writeb(0, &com_port->dll);
  writeb(0, &com_port->dlm);
  writeb(UART_LCRVAL, &com_port->lcr);
  writeb(UART_MCRVAL, &com_port->mcr);
  writeb(UART_FCRVAL, &com_port->fcr);
  writeb(UART_LCR_BKSE | UART_LCRVAL, &com_port->lcr);
  writeb(div & 0xff, &com_port->dll);
  writeb((div >> 8) & 0xff, &com_port->dlm);
  writeb(UART_LCRVAL, &com_port->lcr);

	/* Timer init */
	timer_init();
	
	/* NAND init */
	nand_command(NAND_CMD_RESET, 		-1, -1);
	nand_command(NAND_CMD_READID, 0x00, -1);
	puts("Found NAND: ");
	for (int i = 0; i < 6; i++) {
		putc('['); putc('0'+i);	puts("]: ");
		putc_hex(nand_readb());
	}
	putc('\n');

#define TIMEOUT 5
	/* Display counter */
	for (int i = 0; i < TIMEOUT; i++) {
		putc('\r'); puts("Start after: "); putc('0' + TIMEOUT - i);
		__udelay(1000000);
	}
	putc('\r');

#define LOAD_ADDRESS 0x800000

#if 0
	/* Clearing memory */
	for (int i = 0; i < 64; i++)
		((u32 *)LOAD_ADDRESS)[i] = 0xffffffff;
#endif

	/* Reading image header */
#define HEADER_MAGIC 0x56190527
#define IMG_START_PAGE 0x200
#define IMG_START_BLOCK 0

	nand_command(NAND_CMD_READ0, 0, IMG_START_PAGE);
	for (int i = 0; i < 64; i++)
		((char *)LOAD_ADDRESS)[i] = nand_readb();

	puts("Checking header magic...");
	if (((u32 *)LOAD_ADDRESS)[0] != HEADER_MAGIC) {
		puts("\n");
		dump_int(((u32 *)LOAD_ADDRESS)[0]);
		puts("\nFailed!\n");
		dump(LOAD_ADDRESS, 64);
		hang();
	}
	puts("done\n\n");
	dump(LOAD_ADDRESS, 64);
	puts((char *)LOAD_ADDRESS + 32); puts("\n\n");
	u32 load_size = ((u32 *)LOAD_ADDRESS)[3];
	load_size = ec(load_size);
	puts("\tImage size:\t\t");
	dump_int((u32)load_size);
	putc('\n');
	u32 load_addr = ((u32 *)LOAD_ADDRESS)[4];
	ec(load_addr);
	puts("\tLoading address:\t");
	dump_int((u32)load_addr);
	putc('\n');
	u32 ep_addr = ((u32 *)LOAD_ADDRESS)[5];
	ec(ep_addr);
	puts("\tEntry point:\t\t");
	dump_int((u32)ep_addr);
	putc('\n');

#define PAGE_SIZE 2 * 1024
#define PAGES_COUNT 1342
#define OOB_OFFSET 0x2000000
#define OOB_SIZE 64

	/* Reading data into memory  */

	u32 ptr = 0;

	u32 oob = load_addr + OOB_OFFSET;
	u32 oob_ptr = 0;

	/* Reading one page from NAND at a time */
	for (int page_num = IMG_START_PAGE; page_num < IMG_START_PAGE + PAGES_COUNT; page_num++) {
		nand_command(NAND_CMD_READ0, 0, page_num);

		int i = 0;

		/* Skipping read image header */
		if (page_num == IMG_START_PAGE) {
			for (; i < 64; i++)
				nand_readb();
		}
		
		/* Reading image page data */
		for (; i < PAGE_SIZE; i++)
			((char *)load_addr)[ptr++] = nand_readb();

		/* Reading image page OOB block */
		for (; i < PAGE_SIZE + OOB_SIZE; i++)
			((char *)oob)[oob_ptr++] = nand_readb();

		dump(oob, 64);
		return;

		/* Verifying ECC code */
		if (!verify_ecc(((char *)load_addr)[ptr - PAGE_SIZE], oob)) {
			print_ecc_failure(page_num, oob);
			for(;;);
			return;
		}
	}

	void (*kernel_entry)(int, int, void *);
	kernel_entry = (void (*)(int, int, void *))ep_addr;

#define MACH_ID 0xd54

	/* Starting kernel */
	kernel_entry(0, MACH_ID, &atags);

	/* ... */
}

