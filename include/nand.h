#ifndef _NAND_H_
#define _NAND_H_

/*
 * This constant declares the max. oobsize / page, which
 * is supported now. If you add a chip with bigger oobsize/page
 * adjust this accordingly.
 */
#define NAND_MAX_OOBSIZE	576
#define NAND_MAX_PAGESIZE	8192

/*
 * Constants for hardware specific CLE/ALE/NCE function
 *
 * These are bits which can be or'ed to set/clear multiple
 * bits in one go.
 */
/* Select the chip by setting nCE to low */
#define NAND_NCE		0x01
/* Select the command latch by setting CLE to high */
#define NAND_CLE		0x02
/* Select the address latch by setting ALE to high */
#define NAND_ALE		0x04

#define NAND_CTRL_CLE		(NAND_NCE | NAND_CLE)
#define NAND_CTRL_ALE		(NAND_NCE | NAND_ALE)
#define NAND_CTRL_CHANGE	0x80

/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_RNDOUT		5
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_RNDIN		0x85
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_PARAM		0xec
#define NAND_CMD_RESET		0xff

#define NAND_CMD_LOCK		0x2a
#define NAND_CMD_LOCK_TIGHT	0x2c
#define NAND_CMD_UNLOCK1	0x23
#define NAND_CMD_UNLOCK2	0x24
#define NAND_CMD_LOCK_STATUS	0x7a

/* Extended commands for large page devices */
#define NAND_CMD_READSTART	0x30
#define NAND_CMD_RNDOUTSTART	0xE0
#define NAND_CMD_CACHEDPROG	0x15

/* Extended commands for AG-AND device */
/*
 * Note: the command for NAND_CMD_DEPLETE1 is really 0x00 but
 *       there is no way to distinguish that from NAND_CMD_READ0
 *       until the remaining sequence of commands has been completed
 *       so add a high order bit and mask it off in the command.
 */
#define NAND_CMD_DEPLETE1	0x100
#define NAND_CMD_DEPLETE2	0x38
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_STATUS_ERROR	0x72
/* multi-bank error status (banks 0-3) */
#define NAND_CMD_STATUS_ERROR0	0x73
#define NAND_CMD_STATUS_ERROR1	0x74
#define NAND_CMD_STATUS_ERROR2	0x75
#define NAND_CMD_STATUS_ERROR3	0x76
#define NAND_CMD_STATUS_RESET	0x7f
#define NAND_CMD_STATUS_CLEAR	0xff

#define NAND_CMD_NONE		-1

/* Status bits */
#define NAND_STATUS_FAIL	0x01
#define NAND_STATUS_FAIL_N1	0x02
#define NAND_STATUS_TRUE_READY	0x20
#define NAND_STATUS_READY	0x40
#define NAND_STATUS_WP		0x80

/*
 * Constants for ECC_MODES
 */
typedef enum {
	NAND_ECC_NONE,
	NAND_ECC_SOFT,
	NAND_ECC_HW,
	NAND_ECC_HW_SYNDROME,
	NAND_ECC_HW_OOB_FIRST,
	NAND_ECC_SOFT_BCH,
} nand_ecc_modes_t;

/*
 * Constants for Hardware ECC
 */
/* Reset Hardware ECC for read */
#define NAND_ECC_READ		0
/* Reset Hardware ECC for write */
#define NAND_ECC_WRITE		1
/* Enable Hardware ECC before syndrom is read back from flash */
#define NAND_ECC_READSYN	2

/* Bit mask for flags passed to do_nand_read_ecc */
#define NAND_GET_DEVICE		0x80


/*
 * Option constants for bizarre disfunctionality and real
 * features.
 */
/* Chip can not auto increment pages */
#define NAND_NO_AUTOINCR	0x00000001
/* Buswitdh is 16 bit */
#define NAND_BUSWIDTH_16	0x00000002
/* Device supports partial programming without padding */
#define NAND_NO_PADDING		0x00000004
/* Chip has cache program function */
#define NAND_CACHEPRG		0x00000008
/* Chip has copy back function */
#define NAND_COPYBACK		0x00000010
/*
 * AND Chip which has 4 banks and a confusing page / block
 * assignment. See Renesas datasheet for further information.
 */
#define NAND_IS_AND		0x00000020
/*
 * Chip has a array of 4 pages which can be read without
 * additional ready /busy waits.
 */
#define NAND_4PAGE_ARRAY	0x00000040
/*
 * Chip requires that BBT is periodically rewritten to prevent
 * bits from adjacent blocks from 'leaking' in altering data.
 * This happens with the Renesas AG-AND chips, possibly others.
 */
#define BBT_AUTO_REFRESH	0x00000080
/*
 * Chip does not require ready check on read. True
 * for all large page devices, as they do not support
 * autoincrement.
 */
#define NAND_NO_READRDY		0x00000100
/* Chip does not allow subpage writes */
#define NAND_NO_SUBPAGE_WRITE	0x00000200

/* Device is one of 'new' xD cards that expose fake nand command set */
#define NAND_BROKEN_XD		0x00000400

/* Device behaves just like nand, but is readonly */
#define NAND_ROM		0x00000800

/* Options valid for Samsung large page devices */
#define NAND_SAMSUNG_LP_OPTIONS \
	(NAND_NO_PADDING | NAND_CACHEPRG | NAND_COPYBACK)

/* Non chip related options */
/*
 * Use a flash based bad block table. OOB identifier is saved in OOB area.
 * This option is passed to the default bad block table function.
 */
#define NAND_USE_FLASH_BBT	0x00010000
/* This option skips the bbt scan during initialization. */
#define NAND_SKIP_BBTSCAN	0x00020000
/*
 * This option is defined if the board driver allocates its own buffers
 * (e.g. because it needs them DMA-coherent).
 */
#define NAND_OWN_BUFFERS	0x00040000
/* Chip may not exist, so silence any errors in scan */
#define NAND_SCAN_SILENT_NODEV	0x00080000
/*
 * If passed additionally to NAND_USE_FLASH_BBT then BBT code will not touch
 * the OOB area.
 */
#define NAND_USE_FLASH_BBT_NO_OOB	0x00800000
/* Create an empty BBT with no vendor information if the BBT is available */
#define NAND_CREATE_EMPTY_BBT		0x01000000

/* Options set by nand scan */
/* bbt has already been read */
#define NAND_BBT_SCANNED	0x40000000
/* Nand scan has allocated controller struct */
#define NAND_CONTROLLER_ALLOC	0x80000000

/* Cell info constants */
#define NAND_CI_CHIPNR_MSK	0x03
#define NAND_CI_CELLTYPE_MSK	0x0C

#endif /* _NAND_H */
