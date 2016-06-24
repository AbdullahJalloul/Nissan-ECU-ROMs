/* collection of funcs for working with Nissan ROMs
 * (c) fenugrec 2014-2015
 * GPLv3
 */

#include <stdint.h>
#include <stdio.h>	//for printf(); probably can go away someday
#include <string.h>
#include <stdbool.h>
#include "nislib.h"

uint32_t reconst_32(const uint8_t *buf) {
	// ret 4 bytes at *buf with SH endianness
	// " reconst_4B"
	return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
}

uint16_t reconst_16(const uint8_t *buf) {
	return buf[0] << 8 | buf[1];
}

void write_32b(uint32_t val, uint8_t *buf) {
	//write 4 bytes at *buf with SH endianness
	// "decomp_4B"
	buf += 3;	//start at end
	*(buf--) = val & 0xFF;
	val >>= 8;
	*(buf--) = val & 0xFF;
	val >>= 8;
	*(buf--) = val & 0xFF;
	val >>= 8;
	*(buf--) = val & 0xFF;

	return;
}


// used only in enc1(), dec1()
static inline uint16_t mess1(const uint16_t a, const uint16_t b, const uint16_t x) {
	//orig : a = r4= u16 *, b=r5 = u16 *, x = [ffff8416]
	//sub 15FB2 in 8U92A
	uint16_t var2,var2b, var6;
	uint32_t var3;

	var2 = (x + b);	// & 0xFFFF;
	var3 = var2 << 2;
	var6 = (var3 >>16);

	var2b = var6 + var2 + var3 -1;

	return var2b ^ a;

}
// used only in enc1(), dec1()
static inline uint16_t mess2(const uint16_t a, const uint16_t b, const uint16_t x) {
	//orig : sub 15FDE
	uint16_t var0,var2, var3;
	uint32_t var1, var2b;

	var0 = (x + b) ;
	var1 = var0 << 1;

	var2 = (var1 >>16) + var0 + var1 -1;
	var2b = var2 << 4;
	var3 = var2b + (var2b >>16);
	return a ^ var3 ^ var2;

}

// "encode" u32 data with key 'scode'
uint32_t enc1(uint32_t data, uint32_t scode) {
	//m: scrambling code (hardcoded in ECU firmware)
	uint16_t mH,mL, sH, sL;
	uint16_t kL, kH;	//temp words

	mL = scode & 0xFFFF;
	mH= scode >> 16;
	sL = data & 0xFFFF;
	sH = data >> 16;


	kL = mess1(sH, sL, mH);

	kH = mess2(sL, kL, mL);

	return (kH << 16) | kL;
}

//decrypt 4 bytes in <data> with firmware's key <scode>
uint32_t dec1(uint32_t data, uint32_t scode) {
	//based on sub 15F18, ugly rewrite
	uint16_t scH, scL;
	uint16_t dH, dL;
	uint16_t t0, t1, tx; //temp vars

	//split in 16b words
	scH = scode >> 16;
	scL = scode;
	dH = data >> 16;
	dL = data;

	t0 = dH;
	t1 = dL;
	t0 = mess2(t0, t1, scL);
	//printf("mess1 returns %0#x\n", t0);
	tx=t0;
	t0=t1;	//swap t0,t1
	t1=tx;

	t0 = mess1(t0, t1, scH);
	//printf("mess2 returns %0#x\n", t0);
	//printf("local_0: %0#X\tlocal_1: %0#X\n", t0, t1);
	return (t0 << 16) | t1;
}


//Painfully unoptimized, because it's easy to get it wrong
const uint8_t *u8memstr(const uint8_t *buf, long buflen, const uint8_t *needle, long nlen) {
	long hcur;
	if (!buf || !needle || (nlen > buflen)) return NULL;

	for (hcur=0; hcur < (buflen - nlen); hcur++) {
		if (memcmp(buf + hcur, needle, nlen)==0) {
			return &buf[hcur];
		}
	}

	return NULL;
}

/* thin wrapper around u8memstr and write_32b */
const uint8_t *u32memstr(const uint8_t *buf, long buflen, const uint32_t needle) {
	uint8_t u8val[4];
	write_32b(needle, u8val);

	return u8memstr(buf, buflen, u8val, 4);
}


const struct keyset_t known_keys[] = {
	{0x0E7D44A9, 0x7A4D91C3, 0x0},
	{0x1701C82E, 0xFED01648, 0x0},
	{0x3621E84E, 0x1FF03568, 0x0},
	{0x3722E94F, 0x1FF13669, 0x0},
	{0x3823EA50, 0x20F2376A, 0x0},
	{0x3B26EC52, 0x23F53A6C, 0x0},
	{0x3C24EB50, 0x21F3386B, 0x0},
	{0x412CF359, 0x2AFB4073, 0x1F1DDD9A},
	{0x422DF45A, 0x2AFC4174, 0x0},
	{0x4D38FE64, 0x35084C7E, 0x0},
	{0x5414CDA6, 0xE303BF23, 0x0},
	{0x634D157A, 0x4B1D6294, 0x403EFEBB},
	{0x6BD9175D, 0x02581327, 0x0},
	{0x705A2287, 0x582A6FA1, 0x4D4B0DC8},
	{0x7B472BD1, 0x8F7577FC, 0x0},
	{0x7C672F93, 0x64377BAE, 0x0},
	{0x7F6A3297, 0x673A7EB1, 0x0},
	{0x8CF9387E, 0x23793448, 0x0},
	{0x8FFD3C82, 0x277C374B, 0x0},
	{0x917B43A8, 0x794C90C2, 0x0},
	{0x93B645A1, 0x46A7E9FD, 0x5E353F7D},
	{0x9851EB85, 0xB4395810, 0x32F1A9FB},
	{0x9E8950B5, 0x86599DCF, 0x0},
	{0x9F8A52B7, 0x875A9ED1, 0x0},
	{0xAD12D93F, 0x10E12659, 0x0},
	{0xAE9961C5, 0x9669ADE0, 0x0},
	{0xBAA56CD1, 0xA275B9EB, 0x97955714},
	{0xBE8298AC, 0x560F4925, 0x0},
	{0xC02E6CB2, 0x57AC677B, 0x0},
	{0xC6E19CF0, 0x685BFBBA, 0x124DCA1B},
	{0xCAB57DE1, 0xB285C9FC, 0x0},
	{0xD3BE86EA, 0xBB8ED206, 0x0},
	{0xE091912E, 0xF04F0066, 0x817EDC9C},
	{0xE5D097FC, 0xCDA0E417, 0xC2C0823F},
	{0xEC5B98DE, 0x83D994A8, 0x0},
	{0xED19DF45, 0x16E82D5F, 0x0},
	{0xEED9A107, 0xD6A9ED21, 0xCBCA8B48},
	{0xF0DBA309, 0xD9ABEF23, 0x0},
	{0,0,0}
	};

/* sum and xor all u32 values in *buf, read with SH endianness */
void sum32(const uint8_t *buf, long siz, uint32_t *sum, uint32_t *xor) {
	long bufcur;
	uint32_t sumt, xort;

	if (!buf || !sum || !xor) return;
	sumt=0;
	xort=0;
	for (bufcur=0; bufcur < siz; bufcur += 4) {
		//loop each uint32, but with good endianness (need to reconstruct)
		uint32_t lw;
		lw = reconst_32(&buf[bufcur]);
		sumt += lw;
		xort ^= lw;
	}
	*sum = sumt;
	*xor = xort;
	return;
}

//calculate checksum & locations
// theory : real ck_sum= sum of all u32 (except ck_sum and ck_xor);
//	real ck_xor = xor of all u32 (except ck_sum et ck_xor)
//	A) we can XOR everything (including cks and ckx since they are unknown yet),
//		this gives xort=ckx ^ ckx ^ cks = cks (so we found the real ck_sum)
//	B) we can SUM everything (including cks and ckx), we get sumt=cks(real sum) + cks + ckx = 2*cks + ckx.
//	so we get ckx= sumt - 2*ckx, and then we try to locate cks and ckx in the ROM.
int checksum_alt2(const uint8_t *buf, long siz, long *p_ack_s, long *p_ack_x,
				long p_skip1, long p_skip2) {
	int ckscount, ckxcount;	//count number of found instances
	long bufcur;
	uint32_t sumt,xort, cks, ckx;

	if (!buf || (siz & 0x3) || !p_ack_s || !p_ack_x || (siz <= 0)) {
		return -1;
	}

	sumt = xort = 0;
	sum32(buf, siz, &sumt, &xort);
	/* Optionally skip 2 extra locations by compensating sumt and xort */
	if (p_skip1 >= 0) {
		sumt -= reconst_32(buf + p_skip1);
		xort ^= reconst_32(buf + p_skip1);
	}
	if (p_skip2 >= 0) {
		sumt -= reconst_32(buf + p_skip2);
		xort ^= reconst_32(buf + p_skip2);
	}


	cks=xort;
	ckx= sumt - 2*xort;	//cheat !
	//printf("sumt=0x%0X; xort=cks=0x%0X; ckx=0x%0X\n",sumt, cks, ckx);
	//try to find cks et ckx in there
	ckscount=0;
	ckxcount=0;
	*p_ack_s = 0;
	*p_ack_x = 0;

	for (bufcur=0; bufcur < siz; bufcur += 4) {
		uint32_t lw;
		lw = reconst_32(&buf[bufcur]);
		if (lw==cks) {
			//printf("cks found @ 0x%0X\n", bufcur);
			*p_ack_s = bufcur;
			ckscount += 1;
			continue;
		}
		if (lw==ckx) {
			//printf("ckx found @ 0x%0X\n", bufcur);
			*p_ack_x = bufcur;
			ckxcount += 1;
			continue;
		}
	}

	if (ckxcount>1 || ckscount>1)
		fprintf(dbg_stream, "warning : more than one set of checksums found ! the real checksums should be close to each other.\n");

	if (ckxcount==0 && ckscount==0) {
//		printf("warning : no checksum found !\n");
		return -1;
	}

	return 0;
}

//Thin wrapper around more generic checksum_alt2
int checksum_std(const uint8_t *buf, long siz, long *p_cks, long *p_ckx) {
	return checksum_alt2(buf, siz, p_cks, p_ckx, -1, -1);
}


// Steps to fix checksums :
// 1) set a,b,c to 0
// 2) calculate actual sum and xor (skipping locs p_cks and p_ckx)
// 3) determine correction values to bring actual sum and xor to the desired cks and ckx
void checksum_fix(uint8_t *buf, long siz, long p_cks, long p_ckx, long p_a, long p_b, long p_c) {
	uint32_t cks, ckx;	//desired sum and xor
	uint32_t ds, dx;	//actual/delta vals
	uint32_t a, b, c;	//correction vals

	//abort if siz not a multiple of 4, and other problems
	if (!buf || (siz <= 0) || (siz & 3) ||
		(p_cks >= siz) || (p_ckx >= siz) ||
		(p_a >= siz) || (p_b >= siz)) return;

	cks = reconst_32(&buf[p_cks]);
	ckx = reconst_32(&buf[p_ckx]);
	printf("desired cks=%X, ckx=%X\n", cks, ckx);
	if ((cks & 1) != (ckx &1)) {
		//Major problem, since both those bits should *always* match
		// (bit 0 of an addition is the XOR of all "bit 0"s !! )
		printf("Warning : unlikely original checksums; unmatched LSBs\n");
	}

	// 1) set correction vals to 0
	write_32b(0, &buf[p_a]);
	write_32b(0, &buf[p_b]);
	write_32b(0, &buf[p_c]);

	// 2) calc actual sum & xor
	ds=0;
	dx=0;
	sum32(buf, siz, &ds, &dx);
	// do not count orig cks and ckx
	ds = ds - (cks + ckx);
	dx = dx ^ cks ^ ckx;
	printf("actual s=%X, x=%X\n", ds, dx);

	//required corrections :
	ds = cks - ds;
	dx = ckx ^ dx;
	printf("corrections ds=%X, dx=%X\n", ds, dx);
	// 3) solve thus :
	//	- find 'c' such that c ^ dx == 0; easy : c = dx.
	//	- the new sum correction is now (ds - c)
	//	- divide the sum correction by 2 : hence,
	//		a + b == ds
	//		a ^ b == 0

	c = dx;
	ds -= c;
	a = b = ds / 2;
	//aaaand... that's it !?

	printf("Correction vals a=%X, b=%X, c=%X\n", a,b,c);
	//write correction vals
	write_32b(a, &buf[p_a]);
	write_32b(b, &buf[p_b]);
	write_32b(c, &buf[p_c]);
	//and verify, just for shits
	sum32(buf, siz, &ds, &dx);
	// do not count orig cks and ckx
	ds = ds - (cks + ckx);
	dx = dx ^ cks ^ ckx;
	if ((ds == cks) && (dx == ckx)) {
		printf("checksum fixed !\n");
	} else {
		printf("could not fix checksum !!\n");
	}

	return;
}


 /* Example of a valid IVT : 0000 0104, ffff 7ffc, 0000 0104, ffff 7ffc
 */
bool check_ivt(const uint8_t *buf) {
	uint32_t por_pc, por_sp;
	uint32_t mr_pc, mr_sp;

	por_pc = reconst_32(buf);
	por_sp = reconst_32(buf + 4);
	mr_pc = reconst_32(buf + 8);
	mr_sp = reconst_32(buf + 12);

	if ((por_pc & 1) || (mr_pc & 1)) return 0;
	if ((por_sp & 3) || (mr_sp & 3)) return 0;
	if (por_pc != mr_pc) return 0;
	if (por_sp != mr_sp) return 0;
	if (por_pc > 0x00ffffff) return 0;
	if (por_sp < 0xffff0000) return 0;

	return 1;
}


long find_ivt(const uint8_t *buf, long siz) {
	long offs;

	if (!buf) return -1;

	siz &= ~3;
	for (offs = 0; siz > 0; siz -= 4, offs += 4) {
		if (check_ivt(buf + offs)) return offs;
	}
	return -1;
}


/* check if the code at offset "func" inside the buffer looks like an eeprom read function.
 * ret 1 if good.
 *
 * assumes "func" is 16-bit aligned. Note : the bounds may be only valid for 7055, 7058, 7059
 */
#define EEPREAD_GETIOREG 30	//check only the first N opcodes
//these are too specific maybe ? covers 7055, 7058, 7058
#define EEPREAD_IOBOUND_L 0xF720
#define EEPREAD_IOBOUND_H 0xF789	//IO reg will be within [L,H] bounds

static bool analyze_eepread(const uint8_t *buf, long siz, uint32_t func) {
	/* algo : look for a mov.w (), Rn that loads an IO register address. This should cover both
	 * bit-bang SPI  and SCI-based code.
	 */
	uint32_t fcur;
	
	for (fcur = 0;fcur < EEPREAD_GETIOREG; fcur += 1) {
		uint16_t opc;
		uint32_t litofs;
		uint16_t literal;
		if ((func + fcur * 2) >= siz) return 0;

		opc = reconst_16(&buf[func + fcur * 2]);
		if ((opc & 0xF000) != 0x9000) continue;
		// so we do have a mov.w (@x, PC), Rn opcode.
		litofs = (func + fcur * 2) + 4;	//PC;
		litofs += (opc & 0xFF) * 2;	//PC + disp*2
		literal = reconst_16(&buf[litofs]);
		if (literal > EEPREAD_IOBOUND_H) continue;
		if (literal < EEPREAD_IOBOUND_L) continue;
		return 1;
	}
	return 0;
}


#define EEPREAD_POSTJSR	6	//search for "jsr" within [-1, +POSTJSR] instructions of "mov 7B"
#define EEPREAD_MAXBT 25	//max backtrack to locate the mov that loads the function address
#define EEPREAD_MINJ 1		//min # of identical, nearby calls to eepread()
#define EEPREAD_JSRWINDOW 10	//search within a radius of _JSRWINDOW for identical jsr opcodes
/* XXX todo : bounds check vs "siz" for the iterations within */

uint32_t find_eepread(const uint8_t *buf, long siz) {
	int occurences = 0;
	uint32_t cur = 0;
	uint32_t jackpot = 9;
	
	siz &= ~1;

	for (cur = 0; cur < siz; cur += 2) {
		/* find E4 7B opcode, for every occurence check if the pattern is credible */
		bool addr_long = 0;	//if addr is loaded with "mov.l"
		uint16_t opc;
		int jumpreg;
		int window;
		bool found_seq;
		uint32_t jsr_loc;	//offset of jsr instr
		uint32_t jsr_opcode;	//copy of opcode

		opc = reconst_16(&buf[cur]);
		if (opc != 0xE47B) continue;
		/* We found a "mov 0x7B, r4"  :
		 * see if there's a jsr just before, or within [POSTJSR] instructions
		*/
		found_seq = 0;
		for (window = -1; window <= EEPREAD_POSTJSR; window ++) {
			opc = reconst_16(&buf[cur + window * 2]);
			if ((opc & 0xF0FF) != 0x400B) continue;
			//printf("found a mov + jsr sequence;");
			jsr_loc = (cur + window * 2);
			found_seq = 1;
			break;
		}
		if (!found_seq) continue;
		/* here, we found a new mov + jsr sequence. Get jsr register # :*/
		jumpreg = (opc & 0x0F00) >> 8;
		/* backtrack to  find a "mov.x ..., Rn" */
		found_seq = 0;
		for (window -= 1; (window + EEPREAD_MAXBT) > 0; window--) {
			opc = reconst_16(&buf[cur + window * 2]);
			if (opc == 0x4F22) break;	// "sts.l pr, @-r15"  :function entry; abort.

			//2 possible opcodes : -  mov.w @(i, pc), Rn  : (0x9n 0xii) , or
			//  mov.l @(i, pc), Rn : (0xDn 0xii)
			jsr_opcode = opc;
			uint8_t jc_top = (opc & 0xFF00) >> 8;
			if (jc_top == (0xD0 | jumpreg)) {
				addr_long = 1;
				found_seq = 1;
				break;
			}
			if (jc_top == (0x90 | jumpreg)) {
				found_seq = 1;
				break;
			}
		}
		if (!found_seq) {
			//unusual; algo should be tweaked if this happens
			fprintf(dbg_stream, "Occurence %d : jumpreg setting not found near 0x%x \n",
				occurences, cur);
			continue;
		}
		/* looking good : found the instruction that loads the function address.
		 * Compute PC offset, retrieve addr
		*/
		jackpot = (cur + window * 2);	//location of "mov.x" instr
		if (addr_long) {
			jackpot += ((opc & 0xFF) * 4) + 4;
			/* essential : align 4 !!! */
			jackpot &= ~0x03;
			//printf("retrieve &er() from 0x%0X\n", jackpot);
			jackpot = reconst_32(&buf[jackpot]);
		} else {
			jackpot += ((opc & 0xFF) * 2) + 4;
			//printf("retrieve &er() from 0x%0X\n", jackpot);
			jackpot = reconst_16(&buf[jackpot]);
		}
		/* discard out-of-ROM addresses */
		if (jackpot > (1024 * 1024 * 1024UL)) {
			//printf("Occurence %d @ 0x%0X: bad; &eep_read() out of bounds (0x%0X)\n",
			//	occurences, cur + window * 2, jackpot);
			continue;
		}
		
		/* improve confidence level : there should really be 2-3 identical "jsr" opcodes nearby */
		int other_jsrs = 0;
		int sign = 1;
		window = 0;
		while (window != EEPREAD_JSRWINDOW) {
			opc = reconst_16(&buf[jsr_loc + window * 2 * sign]);
			if (opc == jsr_opcode) {
				other_jsrs += 1;
			}
			sign = -sign;
			if (sign == 1) window += 1;
		}
		if (other_jsrs < EEPREAD_MINJ) {
			//printf("Occurence %d @ 0x%0X : Unlikely, not enough identical 'jsr's\n", occurences, cur + window * 2);
			continue;
		}
		found_seq = 0;
		/* improve moar : there should be another call with "mov 0x7C, r4" */
		for (window = -10; window <= 10; window ++ ) {
			opc = reconst_16(&buf[jsr_loc + window * 2]);
			if (opc == 0xE47C) {
				found_seq = 1;
				break;
			}
		}
		if (!found_seq) {
			continue;
			//printf("Occurence %d @ 0x%0X : no 7C nearby\n", occurences, cur + window * 2);
		}
		
		/* last test : follow inside eep_read() to see if we access IO registers pretty soon */
		if (analyze_eepread(buf, siz, jackpot)) {
			occurences += 1;
			//printf("Occurence %d @ 0x%0X : &eep_read() = 0x%0X\n", occurences, cur + window * 2, jackpot);
		} else {
			fprintf(dbg_stream, "didn't recognize &eep_read()\n");
		}
		

	}	//for
	//return last occurence.
	switch (occurences) {
	case 0:
		fprintf(dbg_stream, "eep_read() not found ! Needs better heuristics\n");
		jackpot = 0;
		break;
	case 1:
		//normal result
		break;
	default:
		fprintf(dbg_stream, "more than one likely eep_read() found ! Needs better heuristics\n");
		jackpot = 0;
		break;
	}
	return jackpot;
}

