/*
 * genmaptab.c - helper program to generate character mapping tables
 *
 * Copyright (c) 2018 Thorsten Otto of ARAnyM dev team (see AUTHORS)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

static unsigned short const atari_to_utf16[256] = {
/* 00 */	0x0000, 0x21e7, 0x21e9, 0x21e8, 0x21e6, 0x2610, 0x2611, 0x2612,
/* 08 */	0x2713, 0x231a, 0x237e, 0x266a, 0x240c, 0x240d, 0x26f0, 0x26f1,
/* 10 */	0x24ea, 0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0x2466,
/* 18 */	0x2467, 0x2468, 0x018f, 0x241b, 0x26f2, 0x26f3, 0x26f4, 0x26f5,
/* 20 */	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
/* 28 */	0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
/* 30 */	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
/* 38 */	0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
/* 40 */	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
/* 48 */	0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
/* 50 */	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
/* 58 */	0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
/* 60 */	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
/* 68 */	0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
/* 70 */	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
/* 78 */	0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2206,
/* 80 */	0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
/* 88 */	0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
/* 90 */	0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
/* 98 */	0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x00df, 0x0192,
/* a0 */	0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
/* a8 */	0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
/* b0 */	0x00e3, 0x00f5, 0x00d8, 0x00f8, 0x0153, 0x0152, 0x00c0, 0x00c3,
/* b8 */	0x00d5, 0x00a8, 0x00b4, 0x2020, 0x00b6, 0x00a9, 0x00ae, 0x2122,
/* c0 */	0x0133, 0x0132, 0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5,
/* c8 */	0x05d6, 0x05d7, 0x05d8, 0x05d9, 0x05db, 0x05dc, 0x05de, 0x05e0,
/* d0 */	0x05e1, 0x05e2, 0x05e4, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea,
/* d8 */	0x05df, 0x05da, 0x05dd, 0x05e3, 0x05e5, 0x00a7, 0x2227, 0x221e,
/* e0 */	0x03b1, 0x03b2, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
/* e8 */	0x03a6, 0x0398, 0x03a9, 0x03b4, 0x222e, 0x03c6, 0x2208, 0x2229,
/* f0 */	0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
/* f8 */	0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x00b3, 0x00af
};

static unsigned short maptab[0x10000];


int main(void)
{
	int i, j;
	unsigned short uc;
	
	/*
	 * initialize tables
	 */
	for (i = 0; i < 0x10000; i++)
		maptab[i] = 0xffff;
	
	/*
	 * build lookup table
	 */
	for (i = 0; i < 256; i++)
	{
		uc = atari_to_utf16[i];
		maptab[uc] = i;
	}
	
	/*
	 * the control characters in the table above
	 * are mapped to font encodings. For reverse
	 * lookup, map them to themselves
	 */
	for (i = 0; i < 0x20; i++)
		maptab[i] = i;
	maptab[0x7f] = 0x7f;

	/*
	 * mapping of our special chars
	 */
	maptab[0x600] = 0x0600; /* left half of normal radio button */
	maptab[0x601] = 0x0601; /* right half of normal radio button */
	maptab[0x602] = 0x0602; /* left half of selected radio button */
	maptab[0x603] = 0x0603; /* right half of selected radio button */
	maptab[0x604] = 0x0604; /* left half of normal checkbox */
	maptab[0x605] = 0x0605; /* right half of normal checkbox */
	maptab[0x606] = 0x0606; /* left half of selected checkbox */
	maptab[0x607] = 0x0607; /* right half of selected checkbox */

	/*
	 * mapping of characters missing from atari standard encoding
	 */
	maptab[0x00c1] = 0x00c1; /* LATIN CAPITAL LETTER A WITH ACUTE */
	maptab[0x00c2] = 0x00c2; /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
	maptab[0x00c8] = 0x00c8; /* LATIN CAPITAL LETTER E WITH GRAVE */
	maptab[0x00ca] = 0x00ca; /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
	maptab[0x00cb] = 0x00cb; /* LATIN CAPITAL LETTER E WITH DIAERESIS */
	maptab[0x00cc] = 0x00cc; /* LATIN CAPITAL LETTER I WITH GRAVE */
	maptab[0x00cd] = 0x00cd; /* LATIN CAPITAL LETTER I WITH ACUTE */
	maptab[0x00ce] = 0x00ce; /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
	maptab[0x00cf] = 0x00cf; /* LATIN CAPITAL LETTER I WITH DIAERESIS */
	maptab[0x00d0] = 0x00d0; /* LATIN CAPITAL LETTER ETH */
	maptab[0x00d2] = 0x00d2; /* LATIN CAPITAL LETTER O WITH GRAVE */
	maptab[0x00d3] = 0x00d3; /* LATIN CAPITAL LETTER O WITH ACUTE */
	maptab[0x00d4] = 0x00d4; /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
	maptab[0x00d7] = 0x00d7; /* MULTIPLICATION SIGN */
	maptab[0x00d9] = 0x00d9; /* LATIN CAPITAL LETTER U WITH GRAVE */
	maptab[0x00da] = 0x00da; /* LATIN CAPITAL LETTER U WITH ACUTE */
	maptab[0x00db] = 0x00db; /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
	maptab[0x00dd] = 0x00dd; /* LATIN CAPITAL LETTER Y WITH ACUTE */
	maptab[0x00de] = 0x00de; /* LATIN CAPITAL LETTER THORN */
	maptab[0x00f0] = 0x00f0; /* LATIN SMALL LETTER ETH */
	maptab[0x00fd] = 0x00fd; /* LATIN SMALL LETTER Y WITH ACUTE */
	maptab[0x00fe] = 0x00fe; /* LATIN SMALL LETTER THORN */
	maptab[0x00b8] = 0x00b8; /* CEDILLA */
	maptab[0x00be] = 0x00be; /* VULGAR FRACTION THREE QUARTERS */
	maptab[0x00a6] = 0x00a6; /* BROKEN BAR */
	
	maptab[0x0100] = 0x0100; /* LATIN CAPITAL LETTER A WITH MACRON */
	maptab[0x0101] = 0x0101; /* LATIN SMALL LETTER A WITH MACRON */
	maptab[0x0102] = 0x0102; /* LATIN CAPITAL LETTER A WITH BREVE */
	maptab[0x0103] = 0x0103; /* LATIN SMALL LETTER A WITH BREVE */
	maptab[0x0104] = 0x0104; /* LATIN CAPITAL LETTER A WITH OGONEK */
	maptab[0x0105] = 0x0105; /* LATIN SMALL LETTER A WITH OGONEK */
	maptab[0x0106] = 0x0106; /* LATIN CAPITAL LETTER C WITH ACUTE */
	maptab[0x0107] = 0x0107; /* LATIN SMALL LETTER C WITH ACUTE */
	maptab[0x0108] = 0x0108; /* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
	maptab[0x0109] = 0x0109; /* LATIN SMALL LETTER C WITH CIRCUMFLEX */
	maptab[0x010a] = 0x010a; /* LATIN CAPITAL LETTER C WITH DOT ABOVE */
	maptab[0x010b] = 0x010b; /* LATIN SMALL LETTER C WITH DOT ABOVE */
	maptab[0x010c] = 0x010c; /* LATIN CAPITAL LETTER C WITH CARON */
	maptab[0x010d] = 0x010d; /* LATIN SMALL LETTER C WITH CARON */
	maptab[0x010e] = 0x010e; /* LATIN CAPITAL LETTER D WITH CARON */
	maptab[0x010f] = 0x010f; /* LATIN SMALL LETTER D WITH CARON */
	maptab[0x0110] = 0x0110; /* LATIN CAPITAL LETTER D WITH STROKE */
	maptab[0x0111] = 0x0111; /* LATIN SMALL LETTER D WITH STROKE */
	maptab[0x0112] = 0x0112; /* LATIN CAPITAL LETTER E WITH MACRON */
	maptab[0x0113] = 0x0113; /* LATIN SMALL LETTER E WITH MACRON */
	maptab[0x0114] = 0x0114; /* LATIN CAPITAL LETTER E WITH BREVE */
	maptab[0x0115] = 0x0115; /* LATIN SMALL LETTER E WITH BREVE */
	maptab[0x0116] = 0x0116; /* LATIN CAPITAL LETTER E WITH DOT ABOVE */
	maptab[0x0117] = 0x0117; /* LATIN SMALL LETTER E WITH DOT ABOVE */
	maptab[0x0118] = 0x0118; /* LATIN CAPITAL LETTER E WITH OGONEK */
	maptab[0x0119] = 0x0119; /* LATIN SMALL LETTER E WITH OGONEK */
	maptab[0x011a] = 0x011a; /* LATIN CAPITAL LETTER E WITH CARON */
	maptab[0x011b] = 0x011b; /* LATIN SMALL LETTER E WITH CARON */
	maptab[0x011c] = 0x011c; /* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
	maptab[0x011d] = 0x011d; /* LATIN SMALL LETTER G WITH CIRCUMFLEX */
	maptab[0x011e] = 0x011e; /* LATIN CAPITAL LETTER G WITH BREVE */
	maptab[0x011f] = 0x011f; /* LATIN SMALL LETTER G WITH BREVE */
	maptab[0x0120] = 0x0120; /* LATIN CAPITAL LETTER G WITH DOT ABOVE */
	maptab[0x0121] = 0x0121; /* LATIN SMALL LETTER G WITH DOT ABOVE */
	maptab[0x0122] = 0x0122; /* LATIN CAPITAL LETTER G WITH CEDILLA */
	maptab[0x0123] = 0x0123; /* LATIN SMALL LETTER G WITH CEDILLA */
	maptab[0x0124] = 0x0124; /* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
	maptab[0x0125] = 0x0125; /* LATIN SMALL LETTER H WITH CIRCUMFLEX */
	maptab[0x0126] = 0x0126; /* LATIN CAPITAL LETTER H WITH STROKE */
	maptab[0x0127] = 0x0127; /* LATIN SMALL LETTER H WITH STROKE */
	maptab[0x0128] = 0x0128; /* LATIN CAPITAL LETTER I WITH TILDE */
	maptab[0x0129] = 0x0129; /* LATIN SMALL LETTER I WITH TILDE */
	maptab[0x012a] = 0x012a; /* LATIN CAPITAL LETTER I WITH MACRON */
	maptab[0x012b] = 0x012b; /* LATIN SMALL LETTER I WITH MACRON */
	maptab[0x012c] = 0x012c; /* LATIN CAPITAL LETTER I WITH BREVE */
	maptab[0x012d] = 0x012d; /* LATIN SMALL LETTER I WITH BREVE */
	maptab[0x012e] = 0x012e; /* LATIN CAPITAL LETTER I WITH OGONEK */
	maptab[0x012f] = 0x012f; /* LATIN SMALL LETTER I WITH OGONEK */
	maptab[0x0130] = 0x0130; /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
	maptab[0x0131] = 0x0131; /* LATIN SMALL LETTER DOTLESS I */
	maptab[0x0132] = 0x00c1; /* LATIN CAPITAL LIGATURE IJ */
	maptab[0x0133] = 0x00c0; /* LATIN SMALL LIGATURE IJ */
	maptab[0x0134] = 0x0134; /* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
	maptab[0x0135] = 0x0135; /* LATIN SMALL LETTER J WITH CIRCUMFLEX */
	maptab[0x0136] = 0x0136; /* LATIN CAPITAL LETTER K WITH CEDILLA */
	maptab[0x0137] = 0x0137; /* LATIN SMALL LETTER K WITH CEDILLA */
	maptab[0x0138] = 0x0138; /* LATIN SMALL LETTER KRA */
	maptab[0x0139] = 0x0139; /* LATIN CAPITAL LETTER L WITH ACUTE */
	maptab[0x013a] = 0x013a; /* LATIN SMALL LETTER L WITH ACUTE */
	maptab[0x013b] = 0x013b; /* LATIN CAPITAL LETTER L WITH CEDILLA */
	maptab[0x013c] = 0x013c; /* LATIN SMALL LETTER L WITH CEDILLA */
	maptab[0x013d] = 0x013d; /* LATIN CAPITAL LETTER L WITH CARON */
	maptab[0x013e] = 0x013e; /* LATIN SMALL LETTER L WITH CARON */
	maptab[0x013f] = 0x013f; /* LATIN CAPITAL LETTER L WITH MIDDLE DOT */
	maptab[0x0140] = 0x0140; /* LATIN SMALL LETTER L WITH MIDDLE DOT */
	maptab[0x0141] = 0x0141; /* LATIN CAPITAL LETTER L WITH STROKE */
	maptab[0x0142] = 0x0142; /* LATIN SMALL LETTER L WITH STROKE */
	maptab[0x0143] = 0x0143; /* LATIN CAPITAL LETTER N WITH ACUTE */
	maptab[0x0144] = 0x0144; /* LATIN SMALL LETTER N WITH ACUTE */
	maptab[0x0145] = 0x0145; /* LATIN CAPITAL LETTER N WITH CEDILLA */
	maptab[0x0146] = 0x0146; /* LATIN SMALL LETTER N WITH CEDILLA */
	maptab[0x0147] = 0x0147; /* LATIN CAPITAL LETTER N WITH CARON */
	maptab[0x0148] = 0x0148; /* LATIN SMALL LETTER N WITH CARON */
	maptab[0x0149] = 0x0149; /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
	maptab[0x014a] = 0x014a; /* LATIN CAPITAL LETTER ENG */
	maptab[0x014b] = 0x014b; /* LATIN SMALL LETTER ENG */
	maptab[0x014c] = 0x014c; /* LATIN CAPITAL LETTER O WITH MACRON */
	maptab[0x014d] = 0x014d; /* LATIN SMALL LETTER O WITH MACRON */
	maptab[0x014e] = 0x014e; /* LATIN CAPITAL LETTER O WITH BREVE */
	maptab[0x014f] = 0x014f; /* LATIN SMALL LETTER O WITH BREVE */
	maptab[0x0150] = 0x0150; /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
	maptab[0x0151] = 0x0151; /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
	maptab[0x0154] = 0x0154; /* LATIN CAPITAL LETTER R WITH ACUTE */
	maptab[0x0155] = 0x0155; /* LATIN SMALL LETTER R WITH ACUTE */
	maptab[0x0156] = 0x0156; /* LATIN CAPITAL LETTER R WITH CEDILLA */
	maptab[0x0157] = 0x0157; /* LATIN SMALL LETTER R WITH CEDILLA */
	maptab[0x0158] = 0x0158; /* LATIN CAPITAL LETTER R WITH CARON */
	maptab[0x0159] = 0x0159; /* LATIN SMALL LETTER R WITH CARON */
	maptab[0x015a] = 0x015a; /* LATIN CAPITAL LETTER S WITH ACUTE */
	maptab[0x015b] = 0x015b; /* LATIN SMALL LETTER S WITH ACUTE */
	maptab[0x015c] = 0x015c; /* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
	maptab[0x015d] = 0x015d; /* LATIN SMALL LETTER S WITH CIRCUMFLEX */
	maptab[0x015e] = 0x015e; /* LATIN CAPITAL LETTER S WITH CEDILLA */
	maptab[0x015f] = 0x015f; /* LATIN SMALL LETTER S WITH CEDILLA */
	maptab[0x0160] = 0x0160; /* LATIN CAPITAL LETTER S WITH CARON */
	maptab[0x0161] = 0x0161; /* LATIN SMALL LETTER S WITH CARON */
	maptab[0x0162] = 0x0162; /* LATIN CAPITAL LETTER T WITH CEDILLA */
	maptab[0x0163] = 0x0163; /* LATIN SMALL LETTER T WITH CEDILLA */
	maptab[0x0164] = 0x0164; /* LATIN CAPITAL LETTER T WITH CARON */
	maptab[0x0165] = 0x0165; /* LATIN SMALL LETTER T WITH CARON */
	maptab[0x0166] = 0x0166; /* LATIN CAPITAL LETTER T WITH STROKE */
	maptab[0x0167] = 0x0167; /* LATIN SMALL LETTER T WITH STROKE */
	maptab[0x0168] = 0x0168; /* LATIN CAPITAL LETTER U WITH TILDE */
	maptab[0x0169] = 0x0169; /* LATIN SMALL LETTER U WITH TILDE */
	maptab[0x016a] = 0x016a; /* LATIN CAPITAL LETTER U WITH MACRON */
	maptab[0x016b] = 0x016b; /* LATIN SMALL LETTER U WITH MACRON */
	maptab[0x016c] = 0x016c; /* LATIN CAPITAL LETTER U WITH BREVE */
	maptab[0x016d] = 0x016d; /* LATIN SMALL LETTER U WITH BREVE */
	maptab[0x016e] = 0x016e; /* LATIN CAPITAL LETTER U WITH RING ABOVE */
	maptab[0x016f] = 0x016f; /* LATIN SMALL LETTER U WITH RING ABOVE */
	maptab[0x0170] = 0x0170; /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
	maptab[0x0171] = 0x0171; /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
	maptab[0x0172] = 0x0172; /* LATIN CAPITAL LETTER U WITH OGONEK */
	maptab[0x0173] = 0x0173; /* LATIN SMALL LETTER U WITH OGONEK */
	maptab[0x0174] = 0x0174; /* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
	maptab[0x0175] = 0x0175; /* LATIN SMALL LETTER W WITH CIRCUMFLEX */
	maptab[0x0176] = 0x0176; /* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
	maptab[0x0177] = 0x0177; /* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
	maptab[0x0178] = 0x0178; /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
	maptab[0x0179] = 0x0179; /* LATIN CAPITAL LETTER Z WITH ACUTE */
	maptab[0x017a] = 0x017a; /* LATIN SMALL LETTER Z WITH ACUTE */
	maptab[0x017b] = 0x017b; /* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
	maptab[0x017c] = 0x017c; /* LATIN SMALL LETTER Z WITH DOT ABOVE */
	maptab[0x017d] = 0x017d; /* LATIN CAPITAL LETTER Z WITH CARON */
	maptab[0x017e] = 0x017e; /* LATIN SMALL LETTER Z WITH CARON */
	maptab[0x017f] = 0x017f; /* LATIN SMALL LETTER LONG S */
	maptab[0x02c7] = 0x02c7; /* CARON */
	maptab[0x02d8] = 0x02d8; /* BREVE */
	maptab[0x02d9] = 0x02d9; /* DOT ABOVE */
	maptab[0x02db] = 0x02db; /* OGONEK */
	maptab[0x02dd] = 0x02dd; /* DOUBLE ACUTE ACCENT */

	maptab[0x0384] = 0x0384; /* GREEK TONOS */
	maptab[0x0385] = 0x0385; /* GREEK DIALYTIKA TONOS */
	maptab[0x0386] = 0x0386; /* GREEK CAPITAL LETTER ALPHA WITH TONOS */
	maptab[0x0387] = 0x0387; /* GREEK ANO TELEIA */
	maptab[0x0388] = 0x0388; /* GREEK CAPITAL LETTER EPSILON WITH TONOS */
	maptab[0x0389] = 0x0389; /* GREEK CAPITAL LETTER ETA WITH TONOS */
	maptab[0x038a] = 0x038a; /* GREEK CAPITAL LETTER IOTA WITH TONOS */
	maptab[0x038c] = 0x038c; /* GREEK CAPITAL LETTER OMICRON WITH TONOS */
	maptab[0x038e] = 0x038e; /* GREEK CAPITAL LETTER UPSILON WITH TONOS */
	maptab[0x038f] = 0x038f; /* GREEK CAPITAL LETTER OMEGA WITH TONOS */
	maptab[0x0390] = 0x0390; /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
	maptab[0x0391] = 0x0391; /* GREEK CAPITAL LETTER ALPHA */
	maptab[0x0392] = 0x0392; /* GREEK CAPITAL LETTER BETA */
	maptab[0x0393] = 0x00e2; /* GREEK CAPITAL LETTER GAMMA */
	maptab[0x0394] = 0x0394; /* GREEK CAPITAL LETTER DELTA */
	maptab[0x0395] = 0x0395; /* GREEK CAPITAL LETTER EPSILON */
	maptab[0x0396] = 0x0396; /* GREEK CAPITAL LETTER ZETA */
	maptab[0x0397] = 0x0397; /* GREEK CAPITAL LETTER ETA */
	maptab[0x0398] = 0x00e9; /* GREEK CAPITAL LETTER THETA */
	maptab[0x0399] = 0x0399; /* GREEK CAPITAL LETTER IOTA */
	maptab[0x039a] = 0x039a; /* GREEK CAPITAL LETTER KAPPA */
	maptab[0x039b] = 0x039b; /* GREEK CAPITAL LETTER LAMDA */
	maptab[0x039c] = 0x039c; /* GREEK CAPITAL LETTER MU */
	maptab[0x039d] = 0x039d; /* GREEK CAPITAL LETTER NU */
	maptab[0x039e] = 0x039e; /* GREEK CAPITAL LETTER XI */
	maptab[0x039f] = 0x039f; /* GREEK CAPITAL LETTER OMICRON */
	maptab[0x03a0] = 0x03a0; /* GREEK CAPITAL LETTER PI */
	maptab[0x03a1] = 0x03a1; /* GREEK CAPITAL LETTER RHO */
	maptab[0x03a3] = 0x00e4; /* GREEK CAPITAL LETTER SIGMA */
	maptab[0x03a4] = 0x03a4; /* GREEK CAPITAL LETTER TAU */
	maptab[0x03a5] = 0x03a5; /* GREEK CAPITAL LETTER UPSILON */
	maptab[0x03a6] = 0x00e8; /* GREEK CAPITAL LETTER PHI */
	maptab[0x03a7] = 0x03a7; /* GREEK CAPITAL LETTER CHI */
	maptab[0x03a8] = 0x03a8; /* GREEK CAPITAL LETTER PSI */
	maptab[0x03a9] = 0x00ea; /* GREEK CAPITAL LETTER OMEGA */
	maptab[0x03aa] = 0x03aa; /* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
	maptab[0x03ab] = 0x03ab; /* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
	maptab[0x03ac] = 0x03ac; /* GREEK SMALL LETTER ALPHA WITH TONOS */
	maptab[0x03ad] = 0x03ad; /* GREEK SMALL LETTER EPSILON WITH TONOS */
	maptab[0x03ae] = 0x03ae; /* GREEK SMALL LETTER ETA WITH TONOS */
	maptab[0x03af] = 0x03af; /* GREEK SMALL LETTER IOTA WITH TONOS */
	maptab[0x03b0] = 0x03b0; /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
	maptab[0x03b1] = 0x00e0; /* GREEK SMALL LETTER ALPHA */
	maptab[0x03b2] = 0x00e1; /* GREEK SMALL LETTER BETA */
	maptab[0x03b3] = 0x03b3; /* GREEK SMALL LETTER GAMMA */
	maptab[0x03b4] = 0x00eb; /* GREEK SMALL LETTER DELTA */
	maptab[0x03b5] = 0x03b5; /* GREEK SMALL LETTER EPSILON */
	maptab[0x03b6] = 0x03b6; /* GREEK SMALL LETTER ZETA */
	maptab[0x03b7] = 0x03b7; /* GREEK SMALL LETTER ETA */
	maptab[0x03b8] = 0x03b8; /* GREEK SMALL LETTER THETA */
	maptab[0x03b9] = 0x03b9; /* GREEK SMALL LETTER IOTA */
	maptab[0x03ba] = 0x03ba; /* GREEK SMALL LETTER KAPPA */
	maptab[0x03bb] = 0x03bb; /* GREEK SMALL LETTER LAMDA */
	maptab[0x03bc] = 0x03bc; /* GREEK SMALL LETTER MU */
	maptab[0x03bd] = 0x03bd; /* GREEK SMALL LETTER NU */
	maptab[0x03be] = 0x03be; /* GREEK SMALL LETTER XI */
	maptab[0x03bf] = 0x03bf; /* GREEK SMALL LETTER OMICRON */
	maptab[0x03c0] = 0x00e3; /* GREEK SMALL LETTER PI */
	maptab[0x03c1] = 0x03c1; /* GREEK SMALL LETTER RHO */
	maptab[0x03c2] = 0x03c2; /* GREEK SMALL LETTER FINAL SIGMA */
	maptab[0x03c3] = 0x00e5; /* GREEK SMALL LETTER SIGMA */
	maptab[0x03c4] = 0x00e7; /* GREEK SMALL LETTER TAU */
	maptab[0x03c5] = 0x03c5; /* GREEK SMALL LETTER UPSILON */
	maptab[0x03c6] = 0x00ed; /* GREEK SMALL LETTER PHI */
	maptab[0x03c7] = 0x03c7; /* GREEK SMALL LETTER CHI */
	maptab[0x03c8] = 0x03c8; /* GREEK SMALL LETTER PSI */
	maptab[0x03c9] = 0x03c9; /* GREEK SMALL LETTER OMEGA */
	maptab[0x03ca] = 0x03ca; /* GREEK SMALL LETTER IOTA WITH DIALYTIKA */
	maptab[0x03cb] = 0x03cb; /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
	maptab[0x03cc] = 0x03cc; /* GREEK SMALL LETTER OMICRON WITH TONOS */
	maptab[0x03cd] = 0x03cd; /* GREEK SMALL LETTER UPSILON WITH TONOS */
	maptab[0x03ce] = 0x03ce; /* GREEK SMALL LETTER OMEGA WITH TONOS */
	maptab[0x03d0] = 0x03d0; /* GREEK BETA SYMBOL */
	maptab[0x03d1] = 0x03d1; /* GREEK THETA SYMBOL */
	
	maptab[0x0400] = 0x0400; /* CYRILLIC CAPITAL LETTER IE WITH GRAVE */
	maptab[0x0401] = 0x0401; /* CYRILLIC CAPITAL LETTER IO */
	maptab[0x0402] = 0x0402; /* CYRILLIC CAPITAL LETTER DJE */
	maptab[0x0403] = 0x0403; /* CYRILLIC CAPITAL LETTER GJE */
	maptab[0x0404] = 0x0404; /* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
	maptab[0x0405] = 0x0405; /* CYRILLIC CAPITAL LETTER DZE */
	maptab[0x0406] = 0x0406; /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
	maptab[0x0407] = 0x0407; /* CYRILLIC CAPITAL LETTER YI */
	maptab[0x0408] = 0x0408; /* CYRILLIC CAPITAL LETTER JE */
	maptab[0x0409] = 0x0409; /* CYRILLIC CAPITAL LETTER LJE */
	maptab[0x040a] = 0x040a; /* CYRILLIC CAPITAL LETTER NJE */
	maptab[0x040b] = 0x040b; /* CYRILLIC CAPITAL LETTER TSHE */
	maptab[0x040c] = 0x040c; /* CYRILLIC CAPITAL LETTER KJE */
	maptab[0x040d] = 0x040d; /* CYRILLIC CAPITAL LETTER I WITH GRAVE */
	maptab[0x040e] = 0x040e; /* CYRILLIC CAPITAL LETTER SHORT U */
	maptab[0x040f] = 0x040f; /* CYRILLIC CAPITAL LETTER DZHE */
	maptab[0x0410] = 0x0410; /* CYRILLIC CAPITAL LETTER A */
	maptab[0x0411] = 0x0411; /* CYRILLIC CAPITAL LETTER BE */
	maptab[0x0412] = 0x0412; /* CYRILLIC CAPITAL LETTER VE */
	maptab[0x0413] = 0x0413; /* CYRILLIC CAPITAL LETTER GHE */
	maptab[0x0414] = 0x0414; /* CYRILLIC CAPITAL LETTER DE */
	maptab[0x0415] = 0x0415; /* CYRILLIC CAPITAL LETTER IE */
	maptab[0x0416] = 0x0416; /* CYRILLIC CAPITAL LETTER ZHE */
	maptab[0x0417] = 0x0417; /* CYRILLIC CAPITAL LETTER ZE */
	maptab[0x0418] = 0x0418; /* CYRILLIC CAPITAL LETTER I */
	maptab[0x0419] = 0x0419; /* CYRILLIC CAPITAL LETTER SHORT I */
	maptab[0x041a] = 0x041a; /* CYRILLIC CAPITAL LETTER KA */
	maptab[0x041b] = 0x041b; /* CYRILLIC CAPITAL LETTER EL */
	maptab[0x041c] = 0x041c; /* CYRILLIC CAPITAL LETTER EM */
	maptab[0x041d] = 0x041d; /* CYRILLIC CAPITAL LETTER EN */
	maptab[0x041e] = 0x041e; /* CYRILLIC CAPITAL LETTER O */
	maptab[0x041f] = 0x041f; /* CYRILLIC CAPITAL LETTER PE */
	maptab[0x0420] = 0x0420; /* CYRILLIC CAPITAL LETTER ER */
	maptab[0x0421] = 0x0421; /* CYRILLIC CAPITAL LETTER ES */
	maptab[0x0422] = 0x0422; /* CYRILLIC CAPITAL LETTER TE */
	maptab[0x0423] = 0x0423; /* CYRILLIC CAPITAL LETTER U */
	maptab[0x0424] = 0x0424; /* CYRILLIC CAPITAL LETTER EF */
	maptab[0x0425] = 0x0425; /* CYRILLIC CAPITAL LETTER HA */
	maptab[0x0426] = 0x0426; /* CYRILLIC CAPITAL LETTER TSE */
	maptab[0x0427] = 0x0427; /* CYRILLIC CAPITAL LETTER CHE */
	maptab[0x0428] = 0x0428; /* CYRILLIC CAPITAL LETTER SHA */
	maptab[0x0429] = 0x0429; /* CYRILLIC CAPITAL LETTER SHCHA */
	maptab[0x042a] = 0x042a; /* CYRILLIC CAPITAL LETTER HARD SIGN */
	maptab[0x042b] = 0x042b; /* CYRILLIC CAPITAL LETTER YERU */
	maptab[0x042c] = 0x042c; /* CYRILLIC CAPITAL LETTER SOFT SIGN */
	maptab[0x042d] = 0x042d; /* CYRILLIC CAPITAL LETTER E */
	maptab[0x042e] = 0x042e; /* CYRILLIC CAPITAL LETTER YU */
	maptab[0x042f] = 0x042f; /* CYRILLIC CAPITAL LETTER YA */
	maptab[0x0430] = 0x0430; /* CYRILLIC SMALL LETTER A */
	maptab[0x0431] = 0x0431; /* CYRILLIC SMALL LETTER BE */
	maptab[0x0432] = 0x0432; /* CYRILLIC SMALL LETTER VE */
	maptab[0x0433] = 0x0433; /* CYRILLIC SMALL LETTER GHE */
	maptab[0x0434] = 0x0434; /* CYRILLIC SMALL LETTER DE */
	maptab[0x0435] = 0x0435; /* CYRILLIC SMALL LETTER IE */
	maptab[0x0436] = 0x0436; /* CYRILLIC SMALL LETTER ZHE */
	maptab[0x0437] = 0x0437; /* CYRILLIC SMALL LETTER ZE */
	maptab[0x0438] = 0x0438; /* CYRILLIC SMALL LETTER I */
	maptab[0x0439] = 0x0439; /* CYRILLIC SMALL LETTER SHORT I */
	maptab[0x043a] = 0x043a; /* CYRILLIC SMALL LETTER KA */
	maptab[0x043b] = 0x043b; /* CYRILLIC SMALL LETTER EL */
	maptab[0x043c] = 0x043c; /* CYRILLIC SMALL LETTER EM */
	maptab[0x043d] = 0x043d; /* CYRILLIC SMALL LETTER EN */
	maptab[0x043e] = 0x043e; /* CYRILLIC SMALL LETTER O */
	maptab[0x043f] = 0x043f; /* CYRILLIC SMALL LETTER PE */
	maptab[0x0440] = 0x0440; /* CYRILLIC SMALL LETTER ER */
	maptab[0x0441] = 0x0441; /* CYRILLIC SMALL LETTER ES */
	maptab[0x0442] = 0x0442; /* CYRILLIC SMALL LETTER TE */
	maptab[0x0443] = 0x0443; /* CYRILLIC SMALL LETTER U */
	maptab[0x0444] = 0x0444; /* CYRILLIC SMALL LETTER EF */
	maptab[0x0445] = 0x0445; /* CYRILLIC SMALL LETTER HA */
	maptab[0x0446] = 0x0446; /* CYRILLIC SMALL LETTER TSE */
	maptab[0x0447] = 0x0447; /* CYRILLIC SMALL LETTER CHE */
	maptab[0x0448] = 0x0448; /* CYRILLIC SMALL LETTER SHA */
	maptab[0x0449] = 0x0449; /* CYRILLIC SMALL LETTER SHCHA */
	maptab[0x044a] = 0x044a; /* CYRILLIC SMALL LETTER HARD SIGN */
	maptab[0x044b] = 0x044b; /* CYRILLIC SMALL LETTER YERU */
	maptab[0x044c] = 0x044c; /* CYRILLIC SMALL LETTER SOFT SIGN */
	maptab[0x044d] = 0x044d; /* CYRILLIC SMALL LETTER E */
	maptab[0x044e] = 0x044e; /* CYRILLIC SMALL LETTER YU */
	maptab[0x044f] = 0x044f; /* CYRILLIC SMALL LETTER YA */

	maptab[0x2025] = 0x2025; /* TWO DOT LEADER */
	maptab[0x2070] = 0x2070; /* SUPERSCRIPT ZERO */
	maptab[0x2116] = 0x2116; /* NUMERO SIGN */
	maptab[0x2153] = 0x2153; /* VULGAR FRACTION ONE THIRD */
	maptab[0x25a0] = 0x25a0; /* BLACK SQUARE */
	
	/*
	 * 0xde in the table above is mapped to the
	 * original definition (0x2227 = logical and)
	 * Some prefer to map it to euro sign instead.
	 * Do so at least in reverse lookup.
	 */
	maptab[0x00a4] = 0xde; /* currency sign */
	maptab[0x20ac] = 0xde; /* euro sign */
	maptab[0x2038] = 0xde; /* caret; some atari programs use this for the missing euro sign */

	maptab[0x2022] = 0xf9; /* bullet */
	maptab[0x03bc] = 0xe6; /* micro sign */
	maptab[0x00ad] = '-';  /* soft hyphen */
	maptab[0x00a0] = ' ';  /* no-break space */
	
	printf("/* generated by genmaptab.c -- DO NOT EDIT */\n");
	printf("\n");
	printf("#include \"maptab.h\"\n");
	printf("\n");

	printf("unsigned short const atari_to_utf16[256] = {\n");
	for (j = 0; j < 256; j++)
	{
		if (((j + 0) % 8) == 0)
			printf("/* %02x */\t", j);
		else
			printf(", ");
		if (j >= 0x20 && j != 0x7f)
			i = atari_to_utf16[j];
		else
			i = j;
		printf("0x%04x", i);
		if (j == 255)
			printf("\n");
		else if (((j + 1) % 8) == 0)
			printf(",\n");
	}
	printf("};\n");
	printf("\n");

	printf("/* same, except for the controls char which map to themselves in the table above */\n");
	printf("\n");
	printf("unsigned short const atarifont_to_utf16[256] = {\n");
	for (j = 0; j < 256; j++)
	{
		if (((j + 0) % 8) == 0)
			printf("/* %02x */\t", j);
		else
			printf(", ");
		printf("0x%04x", atari_to_utf16[j]);
		if (j == 255)
			printf("\n");
		else if (((j + 1) % 8) == 0)
			printf(",\n");
	}
	printf("};\n");
	printf("\n");

	printf("unsigned short const utf16_to_atari[] = {\n");
	for (i = 0; i < 0x10000; )
	{
		printf("/* %04x */\t0x%04x", i, maptab[i]);
		if (++i != 0x10000)
			printf(",");
		printf("\n");
	}
	printf("};\n");

	return 0;
}	
