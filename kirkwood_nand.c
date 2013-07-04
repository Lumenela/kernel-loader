#include <kirkwood.h>
#include <nand.h>

void putc(char);

/*
 * kw_config_gpio - GPIO configuration
 */
void kw_config_gpio(u32 gpp0_oe_val, u32 gpp1_oe_val, u32 gpp0_oe, u32 gpp1_oe)
{
  struct kwgpio_registers *gpio0reg =
    (struct kwgpio_registers *)KW_GPIO0_BASE;
  struct kwgpio_registers *gpio1reg =
    (struct kwgpio_registers *)KW_GPIO1_BASE;
  
	/* Init GPIOS to default values as per board requirement */
  writel(gpp0_oe_val, &gpio0reg->dout);
  writel(gpp1_oe_val, &gpio1reg->dout);
  writel(gpp0_oe, &gpio0reg->oe);
  writel(gpp1_oe, &gpio1reg->oe);
}

void kirkwood_mpp_conf(u32 *mpp_list)
{
  u32 mpp_ctrl[MPP_NR_REGS];
  unsigned int variant_mask;
  int i;

  variant_mask = MPP_F6281_MASK;

  for (i = 0; i < MPP_NR_REGS; i++) {
    mpp_ctrl[i] = readl(MPP_CTRL(i));
  }

  while (*mpp_list) {
    unsigned int num = MPP_NUM(*mpp_list);
    unsigned int sel = MPP_SEL(*mpp_list);
    int shift;

    if (num > MPP_MAX) {
      continue;
    }
    if (!(*mpp_list & variant_mask)) {
      continue;
    }

    shift = (num & 7) << 2;
    mpp_ctrl[num / 8] &= ~(0xf << shift);
    mpp_ctrl[num / 8] |= sel << shift;

    mpp_list++;
  }

  for (i = 0; i < MPP_NR_REGS; i++) {
    writel(mpp_ctrl[i], MPP_CTRL(i));
  }
}

/* NAND Flash Soc registers */
struct kwnandf_registers {
	u32 rd_params;	/* 0x10418 */
	u32 wr_param;	/* 0x1041c */
	u8  pad[0x10470 - 0x1041c - 4];
	u32 ctrl;	/* 0x10470 */
};

static struct kwnandf_registers *nf_reg =
	(struct kwnandf_registers *)KW_NANDF_BASE;

/*
 * hardware specific access to control-lines/bits
 */
#define NAND_ACTCEBOOT_BIT		0x02
#define NAND_BASE 0xd8000000

#define CHIP_DELAY 300

char
nand_readb(void)
{
	return readb(NAND_BASE);
}

static void kw_nand_hwcontrol(int cmd, unsigned int ctrl)
{
	u32 offs;

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
		offs = (1 << 0);	/* Commands with A[1:0] == 01 */
	else if (ctrl & NAND_ALE)
		offs = (1 << 1);	/* Addresses with A[1:0] == 10 */
	else
		return;

	writeb(cmd, NAND_BASE + offs);
}

void kw_nand_select_chip(void)
{
	u32 data;

	data = readl(&nf_reg->ctrl);
	data |= NAND_ACTCEBOOT_BIT;
	writel(data, &nf_reg->ctrl);
}

void
nand_command(unsigned int command,
							int column, int page_addr)
{
	kw_nand_hwcontrol(command & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	if (column != -1 || page_addr != -1) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

		/* Serially input address */
		if (column != -1) {
//			puts("a0: "); dump_int(column);
			kw_nand_hwcontrol(column, ctrl);
			ctrl &= ~NAND_CTRL_CHANGE;
//			puts(" a1: "); dump_int(column >> 8);
			kw_nand_hwcontrol(column >> 8, ctrl);
		}
		if (page_addr != -1) {
//			puts(" a2: "); dump_int(page_addr);
			kw_nand_hwcontrol(page_addr, ctrl);
//			puts(" a3: "); dump_int(page_addr >> 8);
			kw_nand_hwcontrol(page_addr >> 8,
				       NAND_NCE | NAND_ALE);
			/* One more address cycle for devices > 128MiB */
//			if (chip->chipsize > (128 << 20))
//			puts(" a4: "); dump_int(page_addr >> 16); putc('\n');
				kw_nand_hwcontrol(page_addr >> 16,
					       NAND_NCE | NAND_ALE);
		}
	}
	kw_nand_hwcontrol(NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	switch (command) {

	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_RNDIN:
	case NAND_CMD_STATUS:
	case NAND_CMD_DEPLETE1:
		return;

		/*
		 * read error status commands require only a short delay
		 */
	case NAND_CMD_STATUS_ERROR:
	case NAND_CMD_STATUS_ERROR0:
	case NAND_CMD_STATUS_ERROR1:
	case NAND_CMD_STATUS_ERROR2:
	case NAND_CMD_STATUS_ERROR3:
		udelay(CHIP_DELAY);
		return;

	case NAND_CMD_RESET:
//		if (chip->dev_ready)
//			break;
		udelay(CHIP_DELAY);
		kw_nand_hwcontrol(NAND_CMD_STATUS,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		kw_nand_hwcontrol(NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		uint rst_sts_cnt = 200000;
		while (!(readb(NAND_BASE) & NAND_STATUS_READY) &&
			(rst_sts_cnt--));
//		--	TEST --
		udelay(CHIP_DELAY);
		return;

	case NAND_CMD_RNDOUT:
		/* No ready / busy check necessary */
		kw_nand_hwcontrol(NAND_CMD_RNDOUTSTART,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		kw_nand_hwcontrol(NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		return;

	case NAND_CMD_READ0:
		kw_nand_hwcontrol(NAND_CMD_READSTART,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		kw_nand_hwcontrol(NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);

	
	/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		udelay(CHIP_DELAY);
		return;
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	udelay(1);

	nand_wait_ready();
}
