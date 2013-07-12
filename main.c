#define UART_BAUDRATE								115200
#define CONFIG_SYS_NS16550_REG_SIZE	(-4)

#include <kirkwood.h>
#include <ns16550.h>
#include <nand.h>

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

typedef unsigned long uint_least32_t;

/*
  Name  : CRC-32
  Poly  : 0x04C11DB7    x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 
                       + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
  Init  : 0xFFFFFFFF
  Revert: true
  XorOut: 0xFFFFFFFF
  Check : 0xCBF43926 ("123456789")
  MaxLen: 268 435 455 
*/
 

const uint_least32_t Crc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

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

uint_least32_t crc32(const unsigned char * buf, unsigned int len)
{
    uint_least32_t crc = 0xFFFFFFFF;
    dump_int(len);
    while (len--) {
        crc = (crc >> 8) ^ Crc32Table[(crc ^ *buf++) & 0xFF];
	puts(crc);
    }
    return crc ^ 0xFFFFFFFF;
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
	puts((char *)LOAD_ADDRESS + 32); putc('\n');
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

	/* Reading data into memory  */
#define PAGE_SIZE 2 * 1024

	u32 ptr = 0;
	for (int page_num = IMG_START_PAGE; page_num < 1342 + IMG_START_PAGE; page_num++) {
		nand_command(NAND_CMD_READ0, 0, page_num);
				
		int i = 0;
		if (page_num == IMG_START_PAGE) {
			for (; i < 64; i++)
				nand_readb();
		}
		for (; i < PAGE_SIZE; i++)
			((char *)load_addr)[ptr++] = nand_readb();
				
//		WATCHDOG_RESET();
	}

	u32 data_crc = ((u32 *)LOAD_ADDRESS)[6];
	puts("Image CRC: "); dump_int(data_crc); puts("\n");
	u32 calc_data_crc = crc32(load_addr, ptr-load_addr);
	puts("Calculated CRC: "); dump_int(calc_data_crc); puts("\n");

	void (*kernel_entry)(int, int, void *);
	kernel_entry = (void (*)(int, int, void *))ep_addr;

#define MACH_ID 0xd54

	/* Starting kernel */
	kernel_entry(0, MACH_ID, &atags);

	/* ... */
}

