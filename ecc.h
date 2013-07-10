/**
 * Dump routine definition.
 */
void dump(int ptr, int len);

/**
 * Dump int definition.
 */
void dump_int(u32 value);

/**
 * Prints ECC verifying failure information.
 */
void print_ecc_failure(u32 number, u32 oob) {

	puts("ECC checking failed!\n");
	puts("2 or more bits wrong.\n\n");

	puts("\tCorrupted page number: ");
	dump_int(number);

	puts("\n\tPage OOB block:\n\n");
	dump(oob, 64);
}

/**
 * Partitions the whole byte.
 *
 * ECCo is the most right 3 bits,
 * ECCe is right before the ECCo.
 * 
 * ECCe = result >> 3;
 * ECCo = result & 0x7; 
 */
char partition_byte(char byte) {

	char re = 0, ro = 0;
	char even, odd, val;

	int i, j, k;
	for (i = 4; i >= 1; i >>= 1) {
		even = odd = 0;
		for (j = 0; j < 8 / i; j++) {
			for (k = i * j; k < i * (j + 1); k++) {
				val = (byte << (7 - k)) >> 7;
				if (j % 2 == 0) {
					even ^= val;	
				} else {
					odd ^= val;
				}
			}
		}
		re = (re << 1) | even;
		ro = (ro << 1) | odd;
	}

	return (re << 3) | ro;
}

/**
 * Partitions the whole block (byte parity).
 * Minor two bytes of result: EEEEEEEEOOOOOOOO
 */
u32 partition_block(u32* bytes) {
	/*
	 * The goal is to fill two lowest bytes,
	 * each with 8 bits of 128, 64,... 2, 1
	 * XOR-ed subsequences of block bits.
	 */
	return 0;
}

#define SECTOR_SIZE 256
#define ECC_FAIL 0x7FF
#define ECC_OK 0

/**
 * Calculates and verifies ECC code.
 */
int verify_ecc(u32 page, u32 oob) {
	/*
	 * ECC even and odd 11-bit values are shifted
	 * to the most left position in the u32 vars.
	 */
	u32 old_ecc = 0, new_ecc = 0;

	/*
	 * The final result by XOR-ed ECC values
	 * and the wrong byte and bit addresses.
	 */
	u32 result = 0, address = 0,
	    minors = 0, majors	= 0;

	/*
	 * Byte-wise and bit-wise are containers for
	 * every sector's byte parity and cumulative
	 * bit indexes parities in block accordingly.
	 */
	u32 byte_parity[] = {0, 0, 0, 0, 0, 0, 0, 0};
	char bit_parity	  =  0;

	int i, j, k;
	char byte, b;

	/* For each 256-bytes sector. */
	for (i = 0; i < 8; i++) {

		/* Reset parities. */
		bit_parity = 0;
		for (j = 0; j < 8; j++) {
			byte_parity[j] = 0;
		}

		/* Reading the sector byte-wise. */
		for (j = 0; j < SECTOR_SIZE; j++) {			

			/* Reset current byte parity. */			
			b = 0;

			/* Getting single byte value. */
			byte = ((char*)page)[i * SECTOR_SIZE + j];

			/* Recalculating byte and bit parities. */
			for (k = 7; k >= 0; k--) {
				/* Bit parity. */
				bit_parity ^= byte;

				/* Cumulative byte parity. */
				b += (byte << (7 - k)) >> 7;
			}

			/* Shifting the byte parity left. */
			byte_parity[j / 32] <<= 1;
			byte_parity[j / 32] |= b % 2;
		}

		/* Partitioning and masking all the parities. */
		new_ecc = minors = majors = 0;

		/*
		 * Writing minor parts of the value (bit-wise).
		 * E is ECCe, O is ECCo. Then minors = 00EEEOOO
		 */
		minors |= partition_byte(bit_parity);

		/*
		 * Writing major parts of the value (byte-wise).
		 * The returned u32 is half-complete with data.
		 */
		majors |= partition_block(byte_parity);

		/*
		 * Combining majors and minors into single value.
		 * The new_ecc value will be: 11 bits of ECCe, 11
		 * bits of ECCo, then 10 zeroed bits.
		 */
		new_ecc = ((majors >> 8)   << 24) | ((minors >> 3)  << 21) |
			  ((majors & 0xFF) << 13) | ((minors & 0x7) << 18);

		/* Read the old ECC value from OOB. */
		old_ecc = 0;
		for (j = 3; j > 0; j--) {
			old_ecc |= ((char*)oob)[40 + i * 3 + (3 - j)] << (j * 8);
		}
		
		/* Calculate the final checking result. */
		result = (old_ecc >> 21) ^ ((old_ecc << 11) >> 21) ^
		    	 (new_ecc >> 21) ^ ((new_ecc << 11) >> 21);

		/* No errors, OK. */
		if (result == ECC_OK) {
			continue;
		} else /* Single error. */
		if (result == ECC_FAIL) {

			char byte_addr = 0, bit_addr = 0;

			/* Get wrong byte and bit address. */
			address = ((old_ecc << 11) >> 21) ^
				  ((new_ecc << 11) >> 21);

			/* Flip inverted bit. */
			byte_addr = (char) (address >> 3);
			bit_addr  = (char) (address & 0x7);
			((char*)page)[i * SECTOR_SIZE + byte_addr] =
				((char*)page)[i * SECTOR_SIZE + byte_addr] ^ (1 << bit_addr);

			puts("Flipped inverted bit : ");
			dump_int(address); putc('\n');

		} else { /* 2 or more errors. */
			return 0;
		}
	}
	return 1;
}
