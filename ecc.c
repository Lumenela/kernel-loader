#include "ecc.h"

/**
 * Calculates byte parity.
 */
u32 parity(char byte) {

	u32 parity = 0;

	int i;
	for (i = 7; i >= 0; i--) {
		parity ^= (byte >> i) & 0x01;
	}

	return parity;
}

/**
 * Ones in the value.
 */
int count_ones(u32 val) {

	int count = 0;

	int i;
	for (i = 0; i < 22; i++) {
		count += (val >> (10 + i)) & 0x01;
	}

	return count;
}

/**
 * Calculates and verifies ECC code.
 */
int verify_ecc(u32 page, u32 oob, int start_page) {

	/* Old ECC, U-Boot ECC and result. */
	u32 old_ecc, uboot_ecc, result;

	int i, j, p;
	char byte;

	char ecc0, ecc1, ecc2;
	char idx, reg1, reg2, reg3, tmp1, tmp2;

	/* For each 256-bytes sector. */
	for (i = 0; i < 8; i++) {

		/* Initialize variables */
		reg1 = reg2 = reg3 = 0;

		/* Build up column parity */
		for(p = 0; p < 256; p++) {
			/* Get CP0 - CP5 from table */
			if (start_page && i == 0 && p < 64) { /* Start page, first sector, header. */
				idx = nand_ecc_precalc_table[((char*)(oob - 64))[p]];
			} else {
				if (!start_page) {
					/* Usual sector number defining and byte shift within. */
					idx = nand_ecc_precalc_table[((char*)page)[i * SECTOR_SIZE + p]];
				} else {
					if (i == 0) {
						/*
						 * First sector of the start page. Continue reading
						 * from the page start dealing with value of p > 63.
						 */
						idx = nand_ecc_precalc_table[((char*)page)[p - 64]];
					} else {
						/*
						 * Not first sector of the start page. We must assume
						 * that simple formula is not the case here, because
						 * the first sector from the start page is only 192
						 * bytes (minus 64 bytes of header), and so, formula
						 * deals with value i - 1 and separate 192 bytes.
						 */
						idx = nand_ecc_precalc_table[((char*)page)
							[(i - 1) * SECTOR_SIZE + 192 + p]];
					}
				}
			}
			reg1 ^= (idx & 0x3f);

			/* All bit XOR = 1 ? */
			if (idx & 0x40) {
				reg3 ^=   (char) p;
				reg2 ^= ~((char) p);
			}
		}

		/* Create non-inverted ECC code from line parity */
		tmp1  = (reg3 & 0x80) >> 0; /* B7 -> B7 */
		tmp1 |= (reg2 & 0x80) >> 1; /* B7 -> B6 */
		tmp1 |= (reg3 & 0x40) >> 1; /* B6 -> B5 */
		tmp1 |= (reg2 & 0x40) >> 2; /* B6 -> B4 */
		tmp1 |= (reg3 & 0x20) >> 2; /* B5 -> B3 */
		tmp1 |= (reg2 & 0x20) >> 3; /* B5 -> B2 */
		tmp1 |= (reg3 & 0x10) >> 3; /* B4 -> B1 */
		tmp1 |= (reg2 & 0x10) >> 4; /* B4 -> B0 */

		tmp2  = (reg3 & 0x08) << 4; /* B3 -> B7 */
		tmp2 |= (reg2 & 0x08) << 3; /* B3 -> B6 */
		tmp2 |= (reg3 & 0x04) << 3; /* B2 -> B5 */
		tmp2 |= (reg2 & 0x04) << 2; /* B2 -> B4 */
		tmp2 |= (reg3 & 0x02) << 2; /* B1 -> B3 */
		tmp2 |= (reg2 & 0x02) << 1; /* B1 -> B2 */
		tmp2 |= (reg3 & 0x01) << 1; /* B0 -> B1 */
		tmp2 |= (reg2 & 0x01) << 0; /* B7 -> B0 */

		/* Calculate final ECC code */
		ecc0 = ~tmp1;
		ecc1 = ~tmp2;
		ecc2 = ((~reg1) << 2) | 0x03;
		uboot_ecc = (ecc0 << 24) | (ecc1 << 16) | (ecc2 << 8);

		/* Read the old ECC value from OOB. */
		old_ecc = 0;
		for (j = 3; j > 0; j--) {
			old_ecc |= ((char*)oob)[40 + i * 3 + (3 - j)] << (j * 8);
		}

		/* Calculate the final checking result. */
		result = old_ecc ^ uboot_ecc;

		/* No errors, OK. */
		if (result == 0x0) {
			continue;
		} else /* Correction. */
		if (count_ones(result) == 11) {

			char byte_addr, bit_addr;

			/* Calculate byte address. */
			byte_addr = (result >> 25) & 0x01 |
				    (result >> 26) & 0x02 |
				    (result >> 27) & 0x04 |
				    (result >> 28) & 0x08 |
				    (result >> 13) & 0x10 |
				    (result >> 14) & 0x20 |
				    (result >> 15) & 0x40 |
				    (result >> 16) & 0x80 ;

			/* Calculate bit address. */
			bit_addr = (result >> 11) & 0x01 |
				   (result >> 12) & 0x02 |
				   (result >> 13) & 0x04 ;

			/* Correct the wrong bit. */
			((char*)page)[i * SECTOR_SIZE + byte_addr] =
				((char*)page)[i * SECTOR_SIZE + byte_addr] ^ (1 << bit_addr);

		} else { /* Fail. */
			return 0;
		}
	}
	return 1;
}

/**
 * Prints ECC verifying failure information.
 */
void print_ecc_failure(u32 number, u32 oob) {

	puts("\n\nECC checking failed!\n");
	puts("2 or more bits wrong.\n\n");

	puts("Page number: ");
	dump_int(number);

	puts("\nOOB block:\n\n");
	dump(oob, 64);
}
