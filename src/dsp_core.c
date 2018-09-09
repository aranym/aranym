/*
	DSP M56001 emulation
	Host/Emulator <-> DSP glue

	(C) 2003-2009 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include <string.h>

#include "math.h"
#include "dsp_core.h"
#include "dsp_cpu.h"
#include "SDL_compat.h"

/*--- Defines ---*/

#ifndef M_PI
#define M_PI	3.141592653589793238462643383279502
#endif

#define DEBUG 0

#define UNUSED(x)	((void)(x))


/* More disasm infos, if wanted */
#define DSP_DISASM_HOSTREAD 0	/* Dsp->Host transfer */
#define DSP_DISASM_HOSTWRITE 0	/* Host->Dsp transfer */
#define DSP_DISASM_STATE 0	/* State changes */

/*--- Functions prototypes ---*/

static void lockMutexNull(dsp_core_t *dsp_core);
static void lockMutexThread(dsp_core_t *dsp_core);
static void unlockMutexNull(dsp_core_t *dsp_core);
static void unlockMutexThread(dsp_core_t *dsp_core);

static void pauseThreadNull(dsp_core_t *dsp_core);
static void pauseThreadThread(dsp_core_t *dsp_core);
static void resumeThreadNull(dsp_core_t *dsp_core);
static void resumeThreadThread(dsp_core_t *dsp_core);

static Uint32 const x_rom[0x100] = {
	/* mulaw table */
	/* M_00 */ 0x7D7C00, /* 8031 */
	/* M_01 */ 0x797C00, /* 7775 */
	/* M_02 */ 0x757C00, /* 7519 */
	/* M_03 */ 0x717C00, /* 7263 */
	/* M_04 */ 0x6D7C00, /* 7007 */
	/* M_05 */ 0x697C00, /* 6751 */
	/* M_06 */ 0x657C00, /* 6495 */
	/* M_07 */ 0x617C00, /* 6239 */
	/* M_08 */ 0x5D7C00, /* 5983 */
	/* M_09 */ 0x597C00, /* 5727 */
	/* M_0A */ 0x557C00, /* 5471 */
	/* M_0B */ 0x517C00, /* 5215 */
	/* M_0C */ 0x4D7C00, /* 4959 */
	/* M_0D */ 0x497C00, /* 4703 */
	/* M_0E */ 0x457C00, /* 4447 */
	/* M_0F */ 0x417C00, /* 4191 */
	/* M_10 */ 0x3E7C00, /* 3999 */
	/* M_11 */ 0x3C7C00, /* 3871 */
	/* M_12 */ 0x3A7C00, /* 3743 */
	/* M_13 */ 0x387C00, /* 3615 */
	/* M_14 */ 0x367C00, /* 3487 */
	/* M_15 */ 0x347C00, /* 3359 */
	/* M_16 */ 0x327C00, /* 3231 */
	/* M_17 */ 0x307C00, /* 3103 */
	/* M_18 */ 0x2E7C00, /* 2975 */
	/* M_19 */ 0x2C7C00, /* 2847 */
	/* M_1A */ 0x2A7C00, /* 2719 */
	/* M_1B */ 0x287C00, /* 2591 */
	/* M_1C */ 0x267C00, /* 2463 */
	/* M_1D */ 0x247C00, /* 2335 */
	/* M_1E */ 0x227C00, /* 2207 */
	/* M_1F */ 0x207C00, /* 2079 */
	/* M_20 */ 0x1EFC00, /* 1983 */
	/* M_21 */ 0x1DFC00, /* 1919 */
	/* M_22 */ 0x1CFC00, /* 1855 */
	/* M_23 */ 0x1BFC00, /* 1791 */
	/* M_24 */ 0x1AFC00, /* 1727 */
	/* M_25 */ 0x19FC00, /* 1663 */
	/* M_26 */ 0x18FC00, /* 1599 */
	/* M_27 */ 0x17FC00, /* 1535 */
	/* M_28 */ 0x16FC00, /* 1471 */
	/* M_29 */ 0x15FC00, /* 1407 */
	/* M_2A */ 0x14FC00, /* 1343 */
	/* M_2B */ 0x13FC00, /* 1279 */
	/* M_2C */ 0x12FC00, /* 1215 */
	/* M_2D */ 0x11FC00, /* 1151 */
	/* M_2E */ 0x10FC00, /* 1087 */
	/* M_2F */ 0x0FFC00, /* 1023 */
	/* M_30 */ 0x0F3C00, /* 975 */
	/* M_31 */ 0x0EBC00, /* 943 */
	/* M_32 */ 0x0E3C00, /* 911 */
	/* M_33 */ 0x0DBC00, /* 879 */
	/* M_34 */ 0x0D3C00, /* 847 */
	/* M_35 */ 0x0CBC00, /* 815 */
	/* M_36 */ 0x0C3C00, /* 783 */
	/* M_37 */ 0x0BBC00, /* 751 */
	/* M_38 */ 0x0B3C00, /* 719 */
	/* M_39 */ 0x0ABC00, /* 687 */
	/* M_3A */ 0x0A3C00, /* 655 */
	/* M_3B */ 0x09BC00, /* 623 */
	/* M_3C */ 0x093C00, /* 591 */
	/* M_3D */ 0x08BC00, /* 559 */
	/* M_3E */ 0x083C00, /* 527 */
	/* M_3F */ 0x07BC00, /* 495 */
	/* M_40 */ 0x075C00, /* 471 */
	/* M_41 */ 0x071C00, /* 455 */
	/* M_42 */ 0x06DC00, /* 439 */
	/* M_43 */ 0x069C00, /* 423 */
	/* M_44 */ 0x065C00, /* 407 */
	/* M_45 */ 0x061C00, /* 391 */
	/* M_46 */ 0x05DC00, /* 375 */
	/* M_47 */ 0x059C00, /* 359 */
	/* M_48 */ 0x055C00, /* 343 */
	/* M_49 */ 0x051C00, /* 327 */
	/* M_4A */ 0x04DC00, /* 311 */
	/* M_4B */ 0x049C00, /* 295 */
	/* M_4C */ 0x045C00, /* 279 */
	/* M_4D */ 0x041C00, /* 263 */
	/* M_4E */ 0x03DC00, /* 247 */
	/* M_4F */ 0x039C00, /* 231 */
	/* M_50 */ 0x036C00, /* 219 */
	/* M_51 */ 0x034C00, /* 211 */
	/* M_52 */ 0x032C00, /* 203 */
	/* M_53 */ 0x030C00, /* 195 */
	/* M_54 */ 0x02EC00, /* 187 */
	/* M_55 */ 0x02CC00, /* 179 */
	/* M_56 */ 0x02AC00, /* 171 */
	/* M_57 */ 0x028C00, /* 163 */
	/* M_58 */ 0x026C00, /* 155 */
	/* M_59 */ 0x024C00, /* 147 */
	/* M_5A */ 0x022C00, /* 139 */
	/* M_5B */ 0x020C00, /* 131 */
	/* M_5C */ 0x01EC00, /* 123 */
	/* M_5D */ 0x01CC00, /* 115 */
	/* M_5E */ 0x01AC00, /* 107 */
	/* M_5F */ 0x018C00, /* 99 */
	/* M_60 */ 0x017400, /* 93 */
	/* M_61 */ 0x016400, /* 89 */
	/* M_62 */ 0x015400, /* 85 */
	/* M_63 */ 0x014400, /* 81 */
	/* M_64 */ 0x013400, /* 77 */
	/* M_65 */ 0x012400, /* 73 */
	/* M_66 */ 0x011400, /* 69 */
	/* M_67 */ 0x010400, /* 65 */
	/* M_68 */ 0x00F400, /* 61 */
	/* M_69 */ 0x00E400, /* 57 */
	/* M_6A */ 0x00D400, /* 53 */
	/* M_6B */ 0x00C400, /* 49 */
	/* M_6C */ 0x00B400, /* 45 */
	/* M_6D */ 0x00A400, /* 41 */
	/* M_6E */ 0x009400, /* 37 */
	/* M_6F */ 0x008400, /* 33 */
	/* M_70 */ 0x007800, /* 30 */
	/* M_71 */ 0x007000, /* 28 */
	/* M_72 */ 0x006800, /* 26 */
	/* M_73 */ 0x006000, /* 24 */
	/* M_74 */ 0x005800, /* 22 */
	/* M_75 */ 0x005000, /* 20 */
	/* M_76 */ 0x004800, /* 18 */
	/* M_77 */ 0x004000, /* 16 */
	/* M_78 */ 0x003800, /* 14 */
	/* M_79 */ 0x003000, /* 12 */
	/* M_7A */ 0x002800, /* 10 */
	/* M_7B */ 0x002000, /* 8 */
	/* M_7C */ 0x001800, /* 6 */
	/* M_7D */ 0x001000, /* 4 */
	/* M_7E */ 0x000800, /* 2 */
	/* M_7F */ 0x000000, /* 0 */

	/* a-law table */
	/* A_80 */ 0x158000, /* 688 */
	/* A_81 */ 0x148000, /* 656 */
	/* A_82 */ 0x178000, /* 752 */
	/* A_83 */ 0x168000, /* 720 */
	/* A_84 */ 0x118000, /* 560 */
	/* A_85 */ 0x108000, /* 528 */
	/* A_86 */ 0x138000, /* 624 */
	/* A_87 */ 0x128000, /* 592 */
	/* A_88 */ 0x1D8000, /* 944 */
	/* A_89 */ 0x1C8000, /* 912 */
	/* A_8A */ 0x1F8000, /* 1008 */
	/* A_8B */ 0x1E8000, /* 976 */
	/* A_8C */ 0x198000, /* 816 */
	/* A_8D */ 0x188000, /* 784 */
	/* A_8E */ 0x1B8000, /* 880 */
	/* A_8F */ 0x1A8000, /* 848 */
	/* A_90 */ 0x0AC000, /* 344 */
	/* A_91 */ 0x0A4000, /* 328 */
	/* A_92 */ 0x0BC000, /* 376 */
	/* A_93 */ 0x0B4000, /* 360 */
	/* A_94 */ 0x08C000, /* 280 */
	/* A_95 */ 0x084000, /* 264 */
	/* A_96 */ 0x09C000, /* 312 */
	/* A_97 */ 0x094000, /* 296 */
	/* A_98 */ 0x0EC000, /* 472 */
	/* A_99 */ 0x0E4000, /* 456 */
	/* A_9A */ 0x0FC000, /* 504 */
	/* A_9B */ 0x0F4000, /* 488 */
	/* A_9C */ 0x0CC000, /* 408 */
	/* A_9D */ 0x0C4000, /* 392 */
	/* A_9E */ 0x0DC000, /* 440 */
	/* A_9F */ 0x0D4000, /* 424 */
	/* A_A0 */ 0x560000, /* 2752 */
	/* A_A1 */ 0x520000, /* 2624 */
	/* A_A2 */ 0x5E0000, /* 3008 */
	/* A_A3 */ 0x5A0000, /* 2880 */
	/* A_A4 */ 0x460000, /* 2240 */
	/* A_A5 */ 0x420000, /* 2112 */
	/* A_A6 */ 0x4E0000, /* 2496 */
	/* A_A7 */ 0x4A0000, /* 2368 */
	/* A_A8 */ 0x760000, /* 3776 */
	/* A_A9 */ 0x720000, /* 3648 */
	/* A_AA */ 0x7E0000, /* 4032 */
	/* A_AB */ 0x7A0000, /* 3904 */
	/* A_AC */ 0x660000, /* 3264 */
	/* A_AD */ 0x620000, /* 3136 */
	/* A_AE */ 0x6E0000, /* 3520 */
	/* A_AF */ 0x6A0000, /* 3392 */
	/* A_B0 */ 0x2B0000, /* 1376 */
	/* A_B1 */ 0x290000, /* 1312 */
	/* A_B2 */ 0x2F0000, /* 1504 */
	/* A_B3 */ 0x2D0000, /* 1440 */
	/* A_B4 */ 0x230000, /* 1120 */
	/* A_B5 */ 0x210000, /* 1056 */
	/* A_B6 */ 0x270000, /* 1248 */
	/* A_B7 */ 0x250000, /* 1184 */
	/* A_B8 */ 0x3B0000, /* 1888 */
	/* A_B9 */ 0x390000, /* 1824 */
	/* A_BA */ 0x3F0000, /* 2016 */
	/* A_BB */ 0x3D0000, /* 1952 */
	/* A_BC */ 0x330000, /* 1632 */
	/* A_BD */ 0x310000, /* 1568 */
	/* A_BE */ 0x370000, /* 1760 */
	/* A_BF */ 0x350000, /* 1696 */
	/* A_C0 */ 0x015800, /* 43 */
	/* A_C1 */ 0x014800, /* 41 */
	/* A_C2 */ 0x017800, /* 47 */
	/* A_C3 */ 0x016800, /* 45 */
	/* A_C4 */ 0x011800, /* 35 */
	/* A_C5 */ 0x010800, /* 33 */
	/* A_C6 */ 0x013800, /* 39 */
	/* A_C7 */ 0x012800, /* 37 */
	/* A_C8 */ 0x01D800, /* 59 */
	/* A_C9 */ 0x01C800, /* 57 */
	/* A_CA */ 0x01F800, /* 63 */
	/* A_CB */ 0x01E800, /* 61 */
	/* A_CC */ 0x019800, /* 51 */
	/* A_CD */ 0x018800, /* 49 */
	/* A_CE */ 0x01B800, /* 55 */
	/* A_CF */ 0x01A800, /* 53 */
	/* A_D0 */ 0x005800, /* 11 */
	/* A_D1 */ 0x004800, /* 9 */
	/* A_D2 */ 0x007800, /* 15 */
	/* A_D3 */ 0x006800, /* 13 */
	/* A_D4 */ 0x001800, /* 3 */
	/* A_D5 */ 0x000800, /* 1 */
	/* A_D6 */ 0x003800, /* 7 */
	/* A_D7 */ 0x002800, /* 5 */
	/* A_D8 */ 0x00D800, /* 27 */
	/* A_D9 */ 0x00C800, /* 25 */
	/* A_DA */ 0x00F800, /* 31 */
	/* A_DB */ 0x00E800, /* 29 */
	/* A_DC */ 0x009800, /* 19 */
	/* A_DD */ 0x008800, /* 17 */
	/* A_DE */ 0x00B800, /* 23 */
	/* A_DF */ 0x00A800, /* 21 */
	/* A_E0 */ 0x056000, /* 172 */
	/* A_E1 */ 0x052000, /* 164 */
	/* A_E2 */ 0x05E000, /* 188 */
	/* A_E3 */ 0x05A000, /* 180 */
	/* A_E4 */ 0x046000, /* 140 */
	/* A_E5 */ 0x042000, /* 132 */
	/* A_E6 */ 0x04E000, /* 156 */
	/* A_E7 */ 0x04A000, /* 148 */
	/* A_E8 */ 0x076000, /* 236 */
	/* A_E9 */ 0x072000, /* 228 */
	/* A_EA */ 0x07E000, /* 252 */
	/* A_EB */ 0x07A000, /* 244 */
	/* A_EC */ 0x066000, /* 204 */
	/* A_ED */ 0x062000, /* 196 */
	/* A_EE */ 0x06E000, /* 220 */
	/* A_EF */ 0x06A000, /* 212 */
	/* A_F0 */ 0x02B000, /* 86 */
	/* A_F1 */ 0x029000, /* 82 */
	/* A_F2 */ 0x02F000, /* 94 */
	/* A_F3 */ 0x02D000, /* 90 */
	/* A_F4 */ 0x023000, /* 70 */
	/* A_F5 */ 0x021000, /* 66 */
	/* A_F6 */ 0x027000, /* 78 */
	/* A_F7 */ 0x025000, /* 74 */
	/* A_F8 */ 0x03B000, /* 118 */
	/* A_F9 */ 0x039000, /* 114 */
	/* A_FA */ 0x03F000, /* 126 */
	/* A_FB */ 0x03D000, /* 122 */
	/* A_FC */ 0x033000, /* 102 */
	/* A_FD */ 0x031000, /* 98 */
	/* A_FE */ 0x037000, /* 110 */
	/* A_FF */ 0x035000  /* 106 */
};

/* sin table */
static Uint32 const y_rom[0x100] = {
	/* S_00 */ 0x000000, /* +0.0000000000 */
	/* S_01 */ 0x03242b, /* +0.0245412588 */
	/* S_02 */ 0x0647d9, /* +0.0490676165 */
	/* S_03 */ 0x096a90, /* +0.0735645294 */
	/* S_04 */ 0x0c8bd3, /* +0.0980170965 */
	/* S_05 */ 0x0fab27, /* +0.1224106550 */
	/* S_06 */ 0x12c810, /* +0.1467304230 */
	/* S_07 */ 0x15e214, /* +0.1709618568 */
	/* S_08 */ 0x18f8b8, /* +0.1950902939 */
	/* S_09 */ 0x1c0b82, /* +0.2191011906 */
	/* S_0A */ 0x1f19f9, /* +0.2429801226 */
	/* S_0B */ 0x2223a5, /* +0.2667127848 */
	/* S_0C */ 0x25280c, /* +0.2902846336 */
	/* S_0D */ 0x2826b9, /* +0.3136817217 */
	/* S_0E */ 0x2b1f35, /* +0.3368898630 */
	/* S_0F */ 0x2e110a, /* +0.3598949909 */
	/* S_10 */ 0x30fbc5, /* +0.3826833963 */
	/* S_11 */ 0x33def3, /* +0.4052413702 */
	/* S_12 */ 0x36ba20, /* +0.4275550842 */
	/* S_13 */ 0x398cdd, /* +0.4496113062 */
	/* S_14 */ 0x3c56ba, /* +0.4713966846 */
	/* S_15 */ 0x3f174a, /* +0.4928982258 */
	/* S_16 */ 0x41ce1e, /* +0.5141026974 */
	/* S_17 */ 0x447acd, /* +0.5349975824 */
	/* S_18 */ 0x471ced, /* +0.5555702448 */
	/* S_19 */ 0x49b415, /* +0.5758081675 */
	/* S_1A */ 0x4c3fe0, /* +0.5956993103 */
	/* S_1B */ 0x4ebfe9, /* +0.6152316332 */
	/* S_1C */ 0x5133cd, /* +0.6343933344 */
	/* S_1D */ 0x539b2b, /* +0.6531728506 */
	/* S_1E */ 0x55f5a5, /* +0.6715589762 */
	/* S_1F */ 0x5842dd, /* +0.6895405054 */
	/* S_20 */ 0x5a827a, /* +0.7071068287 */
	/* S_21 */ 0x5cb421, /* +0.7242470980 */
	/* S_22 */ 0x5ed77d, /* +0.7409511805 */
	/* S_23 */ 0x60ec38, /* +0.7572088242 */
	/* S_24 */ 0x62f202, /* +0.7730104923 */
	/* S_25 */ 0x64e889, /* +0.7883464098 */
	/* S_26 */ 0x66cf81, /* +0.8032075167 */
	/* S_27 */ 0x68a69f, /* +0.8175848722 */
	/* S_28 */ 0x6a6d99, /* +0.8314696550 */
	/* S_29 */ 0x6c2429, /* +0.8448535204 */
	/* S_2A */ 0x6dca0d, /* +0.8577286005 */
	/* S_2B */ 0x6f5f03, /* +0.8700870275 */
	/* S_2C */ 0x70e2cc, /* +0.8819212914 */
	/* S_2D */ 0x72552d, /* +0.8932243586 */
	/* S_2E */ 0x73b5ec, /* +0.9039893150 */
	/* S_2F */ 0x7504d3, /* +0.9142097235 */
	/* S_30 */ 0x7641af, /* +0.9238795042 */
	/* S_31 */ 0x776c4f, /* +0.9329928160 */
	/* S_32 */ 0x788484, /* +0.9415440559 */
	/* S_33 */ 0x798a24, /* +0.9495282173 */
	/* S_34 */ 0x7a7d05, /* +0.9569402933 */
	/* S_35 */ 0x7b5d04, /* +0.9637761116 */
	/* S_36 */ 0x7c29fc, /* +0.9700312614 */
	/* S_37 */ 0x7ce3cf, /* +0.9757021666 */
	/* S_38 */ 0x7d8a5f, /* +0.9807852507 */
	/* S_39 */ 0x7e1d94, /* +0.9852776527 */
	/* S_3A */ 0x7e9d56, /* +0.9891765118 */
	/* S_3B */ 0x7f0992, /* +0.9924795628 */
	/* S_3C */ 0x7f6237, /* +0.9951847792 */
	/* S_3D */ 0x7fa737, /* +0.9972904921 */
	/* S_3E */ 0x7fd888, /* +0.9987955093 */
	/* S_3F */ 0x7ff622, /* +0.9996988773 */
	/* S_40 */ 0x7fffff, /* +1.0000000000 */
	/* S_41 */ 0x7ff622, /* +0.9996988773 */
	/* S_42 */ 0x7fd888, /* +0.9987955093 */
	/* S_43 */ 0x7fa737, /* +0.9972904921 */
	/* S_44 */ 0x7f6237, /* +0.9951847792 */
	/* S_45 */ 0x7f0992, /* +0.9924795628 */
	/* S_46 */ 0x7e9d56, /* +0.9891765118 */
	/* S_47 */ 0x7e1d94, /* +0.9852776527 */
	/* S_48 */ 0x7d8a5f, /* +0.9807852507 */
	/* S_49 */ 0x7ce3cf, /* +0.9757021666 */
	/* S_4A */ 0x7c29fc, /* +0.9700312614 */
	/* S_4B */ 0x7b5d04, /* +0.9637761116 */
	/* S_4C */ 0x7a7d05, /* +0.9569402933 */
	/* S_4D */ 0x798a24, /* +0.9495282173 */
	/* S_4E */ 0x788484, /* +0.9415440559 */
	/* S_4F */ 0x776c4f, /* +0.9329928160 */
	/* S_50 */ 0x7641af, /* +0.9238795042 */
	/* S_51 */ 0x7504d3, /* +0.9142097235 */
	/* S_52 */ 0x73b5ec, /* +0.9039893150 */
	/* S_53 */ 0x72552d, /* +0.8932243586 */
	/* S_54 */ 0x70e2cc, /* +0.8819212914 */
	/* S_55 */ 0x6f5f03, /* +0.8700870275 */
	/* S_56 */ 0x6dca0d, /* +0.8577286005 */
	/* S_57 */ 0x6c2429, /* +0.8448535204 */
	/* S_58 */ 0x6a6d99, /* +0.8314696550 */
	/* S_59 */ 0x68a69f, /* +0.8175848722 */
	/* S_5A */ 0x66cf81, /* +0.8032075167 */
	/* S_5B */ 0x64e889, /* +0.7883464098 */
	/* S_5C */ 0x62f202, /* +0.7730104923 */
	/* S_5D */ 0x60ec38, /* +0.7572088242 */
	/* S_5E */ 0x5ed77d, /* +0.7409511805 */
	/* S_5F */ 0x5cb421, /* +0.7242470980 */
	/* S_60 */ 0x5a827a, /* +0.7071068287 */
	/* S_61 */ 0x5842dd, /* +0.6895405054 */
	/* S_62 */ 0x55f5a5, /* +0.6715589762 */
	/* S_63 */ 0x539b2b, /* +0.6531728506 */
	/* S_64 */ 0x5133cd, /* +0.6343933344 */
	/* S_65 */ 0x4ebfe9, /* +0.6152316332 */
	/* S_66 */ 0x4c3fe0, /* +0.5956993103 */
	/* S_67 */ 0x49b415, /* +0.5758081675 */
	/* S_68 */ 0x471ced, /* +0.5555702448 */
	/* S_69 */ 0x447acd, /* +0.5349975824 */
	/* S_6A */ 0x41ce1e, /* +0.5141026974 */
	/* S_6B */ 0x3f174a, /* +0.4928982258 */
	/* S_6C */ 0x3c56ba, /* +0.4713966846 */
	/* S_6D */ 0x398cdd, /* +0.4496113062 */
	/* S_6E */ 0x36ba20, /* +0.4275550842 */
	/* S_6F */ 0x33def3, /* +0.4052413702 */
	/* S_70 */ 0x30fbc5, /* +0.3826833963 */
	/* S_71 */ 0x2e110a, /* +0.3598949909 */
	/* S_72 */ 0x2b1f35, /* +0.3368898630 */
	/* S_73 */ 0x2826b9, /* +0.3136817217 */
	/* S_74 */ 0x25280c, /* +0.2902846336 */
	/* S_75 */ 0x2223a5, /* +0.2667127848 */
	/* S_76 */ 0x1f19f9, /* +0.2429801226 */
	/* S_77 */ 0x1c0b82, /* +0.2191011906 */
	/* S_78 */ 0x18f8b8, /* +0.1950902939 */
	/* S_79 */ 0x15e214, /* +0.1709618568 */
	/* S_7A */ 0x12c810, /* +0.1467304230 */
	/* S_7B */ 0x0fab27, /* +0.1224106550 */
	/* S_7C */ 0x0c8bd3, /* +0.0980170965 */
	/* S_7D */ 0x096a90, /* +0.0735645294 */
	/* S_7E */ 0x0647d9, /* +0.0490676165 */
	/* S_7F */ 0x03242b, /* +0.0245412588 */
	/* S_80 */ 0x000000, /* +0.0000000000 */
	/* S_81 */ 0xfcdbd5, /* -0.0245412588 */
	/* S_82 */ 0xf9b827, /* -0.0490676165 */
	/* S_83 */ 0xf69570, /* -0.0735645294 */
	/* S_84 */ 0xf3742d, /* -0.0980170965 */
	/* S_85 */ 0xf054d9, /* -0.1224106550 */
	/* S_86 */ 0xed37f0, /* -0.1467304230 */
	/* S_87 */ 0xea1dec, /* -0.1709618568 */
	/* S_88 */ 0xe70748, /* -0.1950902939 */
	/* S_89 */ 0xe3f47e, /* -0.2191011906 */
	/* S_8A */ 0xe0e607, /* -0.2429801226 */
	/* S_8B */ 0xdddc5b, /* -0.2667127848 */
	/* S_8C */ 0xdad7f4, /* -0.2902846336 */
	/* S_8D */ 0xd7d947, /* -0.3136817217 */
	/* S_8E */ 0xd4e0cb, /* -0.3368898630 */
	/* S_8F */ 0xd1eef6, /* -0.3598949909 */
	/* S_90 */ 0xcf043b, /* -0.3826833963 */
	/* S_91 */ 0xcc210d, /* -0.4052413702 */
	/* S_92 */ 0xc945e0, /* -0.4275550842 */
	/* S_93 */ 0xc67323, /* -0.4496113062 */
	/* S_94 */ 0xc3a946, /* -0.4713966846 */
	/* S_95 */ 0xc0e8b6, /* -0.4928982258 */
	/* S_96 */ 0xbe31e2, /* -0.5141026974 */
	/* S_97 */ 0xbb8533, /* -0.5349975824 */
	/* S_98 */ 0xb8e313, /* -0.5555702448 */
	/* S_99 */ 0xb64beb, /* -0.5758081675 */
	/* S_9A */ 0xb3c020, /* -0.5956993103 */
	/* S_9B */ 0xb14017, /* -0.6152316332 */
	/* S_9C */ 0xaecc33, /* -0.6343933344 */
	/* S_9D */ 0xac64d5, /* -0.6531728506 */
	/* S_9E */ 0xaa0a5b, /* -0.6715589762 */
	/* S_9F */ 0xa7bd23, /* -0.6895405054 */
	/* S_A0 */ 0xa57d86, /* -0.7071068287 */
	/* S_A1 */ 0xa34bdf, /* -0.7242470980 */
	/* S_A2 */ 0xa12883, /* -0.7409511805 */
	/* S_A3 */ 0x9f13c8, /* -0.7572088242 */
	/* S_A4 */ 0x9d0dfe, /* -0.7730104923 */
	/* S_A5 */ 0x9b1777, /* -0.7883464098 */
	/* S_A6 */ 0x99307f, /* -0.8032075167 */
	/* S_A7 */ 0x975961, /* -0.8175848722 */
	/* S_A8 */ 0x959267, /* -0.8314696550 */
	/* S_A9 */ 0x93dbd7, /* -0.8448535204 */
	/* S_AA */ 0x9235f3, /* -0.8577286005 */
	/* S_AB */ 0x90a0fd, /* -0.8700870275 */
	/* S_AC */ 0x8f1d34, /* -0.8819212914 */
	/* S_AD */ 0x8daad3, /* -0.8932243586 */
	/* S_AE */ 0x8c4a14, /* -0.9039893150 */
	/* S_AF */ 0x8afb2d, /* -0.9142097235 */
	/* S_B0 */ 0x89be51, /* -0.9238795042 */
	/* S_B1 */ 0x8893b1, /* -0.9329928160 */
	/* S_B2 */ 0x877b7c, /* -0.9415440559 */
	/* S_B3 */ 0x8675dc, /* -0.9495282173 */
	/* S_B4 */ 0x8582fb, /* -0.9569402933 */
	/* S_B5 */ 0x84a2fc, /* -0.9637761116 */
	/* S_B6 */ 0x83d604, /* -0.9700312614 */
	/* S_B7 */ 0x831c31, /* -0.9757021666 */
	/* S_B8 */ 0x8275a1, /* -0.9807852507 */
	/* S_B9 */ 0x81e26c, /* -0.9852776527 */
	/* S_BA */ 0x8162aa, /* -0.9891765118 */
	/* S_BB */ 0x80f66e, /* -0.9924795628 */
	/* S_BC */ 0x809dc9, /* -0.9951847792 */
	/* S_BD */ 0x8058c9, /* -0.9972904921 */
	/* S_BE */ 0x802778, /* -0.9987955093 */
	/* S_BF */ 0x8009de, /* -0.9996988773 */
	/* S_C0 */ 0x800000, /* -1.0000000000 */
	/* S_C1 */ 0x8009de, /* -0.9996988773 */
	/* S_C2 */ 0x802778, /* -0.9987955093 */
	/* S_C3 */ 0x8058c9, /* -0.9972904921 */
	/* S_C4 */ 0x809dc9, /* -0.9951847792 */
	/* S_C5 */ 0x80f66e, /* -0.9924795628 */
	/* S_C6 */ 0x8162aa, /* -0.9891765118 */
	/* S_C7 */ 0x81e26c, /* -0.9852776527 */
	/* S_C8 */ 0x8275a1, /* -0.9807852507 */
	/* S_C9 */ 0x831c31, /* -0.9757021666 */
	/* S_CA */ 0x83d604, /* -0.9700312614 */
	/* S_CB */ 0x84a2fc, /* -0.9637761116 */
	/* S_CC */ 0x8582fb, /* -0.9569402933 */
	/* S_CD */ 0x8675dc, /* -0.9495282173 */
	/* S_CE */ 0x877b7c, /* -0.9415440559 */
	/* S_CF */ 0x8893b1, /* -0.9329928160 */
	/* S_D0 */ 0x89be51, /* -0.9238795042 */
	/* S_D1 */ 0x8afb2d, /* -0.9142097235 */
	/* S_D2 */ 0x8c4a14, /* -0.9039893150 */
	/* S_D3 */ 0x8daad3, /* -0.8932243586 */
	/* S_D4 */ 0x8f1d34, /* -0.8819212914 */
	/* S_D5 */ 0x90a0fd, /* -0.8700870275 */
	/* S_D6 */ 0x9235f3, /* -0.8577286005 */
	/* S_D7 */ 0x93dbd7, /* -0.8448535204 */
	/* S_D8 */ 0x959267, /* -0.8314696550 */
	/* S_D9 */ 0x975961, /* -0.8175848722 */
	/* S_DA */ 0x99307f, /* -0.8032075167 */
	/* S_DB */ 0x9b1777, /* -0.7883464098 */
	/* S_DC */ 0x9d0dfe, /* -0.7730104923 */
	/* S_DD */ 0x9f13c8, /* -0.7572088242 */
	/* S_DE */ 0xa12883, /* -0.7409511805 */
	/* S_DF */ 0xa34bdf, /* -0.7242470980 */
	/* S_E0 */ 0xa57d86, /* -0.7071068287 */
	/* S_E1 */ 0xa7bd23, /* -0.6895405054 */
	/* S_E2 */ 0xaa0a5b, /* -0.6715589762 */
	/* S_E3 */ 0xac64d5, /* -0.6531728506 */
	/* S_E4 */ 0xaecc33, /* -0.6343933344 */
	/* S_E5 */ 0xb14017, /* -0.6152316332 */
	/* S_E6 */ 0xb3c020, /* -0.5956993103 */
	/* S_E7 */ 0xb64beb, /* -0.5758081675 */
	/* S_E8 */ 0xb8e313, /* -0.5555702448 */
	/* S_E9 */ 0xbb8533, /* -0.5349975824 */
	/* S_EA */ 0xbe31e2, /* -0.5141026974 */
	/* S_EB */ 0xc0e8b6, /* -0.4928982258 */
	/* S_EC */ 0xc3a946, /* -0.4713966846 */
	/* S_ED */ 0xc67323, /* -0.4496113062 */
	/* S_EE */ 0xc945e0, /* -0.4275550842 */
	/* S_EF */ 0xcc210d, /* -0.4052413702 */
	/* S_F0 */ 0xcf043b, /* -0.3826833963 */
	/* S_F1 */ 0xd1eef6, /* -0.3598949909 */
	/* S_F2 */ 0xd4e0cb, /* -0.3368898630 */
	/* S_F3 */ 0xd7d947, /* -0.3136817217 */
	/* S_F4 */ 0xdad7f4, /* -0.2902846336 */
	/* S_F5 */ 0xdddc5b, /* -0.2667127848 */
	/* S_F6 */ 0xe0e607, /* -0.2429801226 */
	/* S_F7 */ 0xe3f47e, /* -0.2191011906 */
	/* S_F8 */ 0xe70748, /* -0.1950902939 */
	/* S_F9 */ 0xea1dec, /* -0.1709618568 */
	/* S_FA */ 0xed37f0, /* -0.1467304230 */
	/* S_FB */ 0xf054d9, /* -0.1224106550 */
	/* S_FC */ 0xf3742d, /* -0.0980170965 */
	/* S_FD */ 0xf69570, /* -0.0735645294 */
	/* S_FE */ 0xf9b827, /* -0.0490676165 */
	/* S_FF */ 0xfcdbd5  /* -0.0245412588 */
};

/*--- Functions ---*/

/* Init DSP emulation */
void dsp_core_init(dsp_core_t *dsp_core, int use_thread)
{
	/* int i; */

#if DEBUG
	fprintf(stderr, "Dsp: core init\n");
#endif

	memset(dsp_core, 0, sizeof(dsp_core_t));

#if 0
	/* Initialize Y:rom[0x0100-0x01ff] with a sin table */
	for (i=0;i<256;i++) {
		float src = (((float) i)*M_PI)/128.0;
		Sint32 dest = (Sint32) round(sin(src) * 8388608.0); /* 1<<23 */
		if (dest>8388607) {
			dest = 8388607;
		} else if (dest<-8388608) {
			dest = -8388608;
		}
		dsp_core->rom[DSP_SPACE_Y][0x100+i]=dest & 0x00ffffff;
	}

	/* Initialize X:rom[0x0100-0x017f] with a mu-law table */
	{
		const Uint16 mulaw_base[8]={
			0x7d7c, 0x3e7c, 0x1efc, 0x0f3c, 0x075c, 0x036c, 0x0174, 0x0078
		};

		Uint32 position = 0x0100;
		Uint32 offset = 0x040000;

		for(i=0;i<8;i++) {
			int j;
			Uint32 value = mulaw_base[i]<<8;

			for (j=0;j<16;j++) {
				dsp_core->rom[DSP_SPACE_X][position++]=value;
				value -= offset;
			}

			offset >>= 1;
		}
	}

	/* Initialize X:rom[0x0180-0x01ff] with a a-law table */
	{
		const Sint32 multiply_base[8]={
			0x1580, 0x0ac0, 0x5600, 0x2b00,
			0x1580, 0x0058, 0x0560, 0x02b0
		};
		const Sint32 multiply_col[4]={0x10, 0x01, 0x04, 0x02};
		const Sint32 multiply_line[4]={0x40, 0x04, 0x10, 0x08};
		const Sint32 base_values[4]={0, -1, 2, 1};
		Uint32 pos=0x0180;
		
		for (i=0;i<8;i++) {
			Sint32 alawbase, j;

			alawbase = multiply_base[i]<<8;
			for (j=0;j<4;j++) {
				Sint32 alawbase1, k;
				
				alawbase1 = alawbase + ((base_values[j]*multiply_line[i & 3])<<12);

				for (k=0;k<4;k++) {
					Sint32 alawbase2;

					alawbase2 = alawbase1 + ((base_values[k]*multiply_col[i & 3])<<12);

					dsp_core->rom[DSP_SPACE_X][pos++]=alawbase2;
				}
			}
		}
	}
#else
	memcpy(&dsp_core->rom[DSP_SPACE_X][0x100], x_rom, sizeof(x_rom));
	memcpy(&dsp_core->rom[DSP_SPACE_Y][0x100], y_rom, sizeof(y_rom));
#endif
	
	dsp_core->use_thread = use_thread;
	if (use_thread) {
		dsp_core->unlockMutex = unlockMutexThread;
		dsp_core->lockMutex = lockMutexThread;
		dsp_core->pauseThread = pauseThreadThread;
		dsp_core->resumeThread = resumeThreadThread;
	} else {
		dsp_core->unlockMutex = unlockMutexNull;
		dsp_core->lockMutex = lockMutexNull;
		dsp_core->pauseThread = pauseThreadNull;
		dsp_core->resumeThread = resumeThreadNull;
	}
}

/* Shutdown DSP emulation */
void dsp_core_shutdown(dsp_core_t *dsp_core)
{
#if DEBUG
	fprintf(stderr, "Dsp: core shutdown\n");
#endif

	dsp_core->running = 0;

	if (dsp_core->thread) {
		dsp_core->resumeThread(dsp_core);
		SDL_WaitThread(dsp_core->thread, NULL);
		dsp_core->thread = NULL;
	}

	/* Destroy the semaphore */
	if (dsp_core->semaphore) {
		SDL_DestroySemaphore(dsp_core->semaphore);
		dsp_core->semaphore = NULL;
	}

	/* Destroy mutex */
	if (dsp_core->mutex) {
		SDL_DestroyMutex(dsp_core->mutex);
		dsp_core->mutex = NULL;
	}
}

/* Reset */
void dsp_core_reset(dsp_core_t *dsp_core)
{
	int i;

#if DEBUG
	fprintf(stderr, "Dsp: core reset\n");
#endif

	/* Kill existing thread and semaphore */
	dsp_core_shutdown(dsp_core);

	/* Memory */
	memset((void*)dsp_core->periph, 0,sizeof(dsp_core->periph));
	memset(dsp_core->stack, 0,sizeof(dsp_core->stack));
	memset(dsp_core->registers, 0,sizeof(dsp_core->registers));

	dsp_core->bootstrap_pos = 0;
	
	/* Registers */
	dsp_core->pc = 0x0000;
	dsp_core->registers[DSP_REG_OMR]=0x02;
	for (i=0;i<8;i++) {
		dsp_core->registers[DSP_REG_M0+i]=0x00ffff;
	}

	/* Interruptions */
	dsp_core->interrupt_state = DSP_INTERRUPT_NONE;
	dsp_core->interrupt_instr_fetch = -1;
	dsp_core->interrupt_save_pc = -1;
	dsp_core->swi_inter = 0;

	/* host port init, dsp side */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR]=(1<<DSP_HOST_HSR_HTDE);

	/* host port init, cpu side */
	dsp_core->hostport[CPU_HOST_CVR]=0x12;
	dsp_core->hostport[CPU_HOST_ISR]=(1<<CPU_HOST_ISR_TRDY)|(1<<CPU_HOST_ISR_TXDE);
	dsp_core->hostport[CPU_HOST_IVR]=0x0f;

	/* Other hardware registers */
	dsp_core->periph[DSP_SPACE_X][DSP_IPR]=0;
	dsp_core->periph[DSP_SPACE_X][DSP_BCR]=0xffff;

	/* Misc */
	dsp_core->loop_rep = 0;

#if DEBUG
	fprintf(stderr, "Dsp: reset done\n");
#endif

	/* Create thread, semaphore, mutex if needed */
	if (dsp_core->use_thread) {
		if (dsp_core->semaphore == NULL) {
			dsp_core->semaphore = SDL_CreateSemaphore(0);
		}
		if (dsp_core->mutex == NULL) {
			dsp_core->mutex = SDL_CreateMutex();
		}
		if (dsp_core->thread == NULL) {
			dsp_core->thread = SDL_CreateNamedThread(dsp56k_exec_thread, "DSP", dsp_core);
		}
	} else {
		dsp56k_init_cpu(dsp_core);
	}
}

/* Lock/unlock mutex functions */
static void lockMutexNull(dsp_core_t *dsp_core)
{
	UNUSED(dsp_core);
}

static void lockMutexThread(dsp_core_t *dsp_core)
{
	SDL_LockMutex(dsp_core->mutex);
}

static void unlockMutexNull(dsp_core_t *dsp_core)
{
	UNUSED(dsp_core);
}

static void unlockMutexThread(dsp_core_t *dsp_core)
{
	SDL_UnlockMutex(dsp_core->mutex);
}

static void pauseThreadNull(dsp_core_t *dsp_core)
{
	UNUSED(dsp_core);
}

static void pauseThreadThread(dsp_core_t *dsp_core)
{
	SDL_SemWait(dsp_core->semaphore);
}

static void resumeThreadNull(dsp_core_t *dsp_core)
{
	UNUSED(dsp_core);
}

static void resumeThreadThread(dsp_core_t *dsp_core)
{
	/* TODO: maybe use a condition variable, to avoid the IF */
	if (SDL_SemValue(dsp_core->semaphore)==0) {
		SDL_SemPost(dsp_core->semaphore);
	}
}

/* CPU<->DSP Host interface transfers */

static void dsp_core_hostport_update_trdy(dsp_core_t *dsp_core)
{
	int trdy;

	/* Clear/set TRDY bit */
	dsp_core->hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_TRDY);
	trdy = (dsp_core->hostport[CPU_HOST_ISR]>>CPU_HOST_ISR_TXDE)
		& ~(dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR]>>DSP_HOST_HSR_HRDF);
	dsp_core->hostport[CPU_HOST_ISR] |= (trdy & 1)<< CPU_HOST_ISR_TRDY;
}

/* Host port transfer ? (dsp->host) */
void dsp_core_dsp2host(dsp_core_t *dsp_core)
{
	/* RXDF = 1 ==> host hasn't read the last value yet */
	if (dsp_core->hostport[CPU_HOST_ISR] & (1<<CPU_HOST_ISR_RXDF)) {
		return;
	}
	/* HTDE = 1 ==> nothing to transfer from DSP port */
	if (dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HTDE)) {
		return;
	}

	dsp_core->hostport[CPU_HOST_RXL] = dsp_core->periph[DSP_SPACE_X][DSP_HOST_HTX];
	dsp_core->hostport[CPU_HOST_RXM] = dsp_core->periph[DSP_SPACE_X][DSP_HOST_HTX]>>8;
	dsp_core->hostport[CPU_HOST_RXH] = dsp_core->periph[DSP_SPACE_X][DSP_HOST_HTX]>>16;

	/* Set HTDE bit to say that DSP can write */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HTDE;

	/* Set RXDF bit to say that host can read */
	dsp_core->hostport[CPU_HOST_ISR] |= 1<<CPU_HOST_ISR_RXDF;

#if DSP_DISASM_HOSTWRITE
	fprintf(stderr, "Dsp: (D->H): Transfer 0x%06x, Dsp HTDE=1, Host RXDF=1\n", dsp_core->periph[DSP_SPACE_X][DSP_HOST_HTX]);
#endif
}

/* Host port transfer ? (host->dsp) */
void dsp_core_host2dsp(dsp_core_t *dsp_core)
{
	/* TXDE = 1 ==> nothing to transfer from host port */
	if (dsp_core->hostport[CPU_HOST_ISR] & (1<<CPU_HOST_ISR_TXDE)) {
		return;
	}
	
	/* HRDF = 1 ==> DSP hasn't read the last value yet */
	if (dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HRDF)) {
		return;
	}

	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] = dsp_core->hostport[CPU_HOST_TXL];
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] |= dsp_core->hostport[CPU_HOST_TXM]<<8;
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] |= dsp_core->hostport[CPU_HOST_TXH]<<16;

	/* Set HRDF bit to say that DSP can read */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HRDF;

	/* Set TXDE bit to say that host can write */
	dsp_core->hostport[CPU_HOST_ISR] |= 1<<CPU_HOST_ISR_TXDE;

#if DSP_DISASM_HOSTREAD
	fprintf(stderr, "Dsp: (H->D): Transfer 0x%06x, Dsp HRDF=1, Host TXDE=1\n", dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX]);
#endif

	dsp_core_hostport_update_trdy(dsp_core);
}

void dsp_core_hostport_dspread(dsp_core_t *dsp_core)
{
	/* Clear HRDF bit to say that DSP has read */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] &= 0xff-(1<<DSP_HOST_HSR_HRDF);
#if DSP_DISASM_HOSTREAD
	fprintf(stderr, "Dsp: (H->D): Dsp HRDF=0\n");
#endif
	dsp_core_hostport_update_trdy(dsp_core);
	dsp_core_host2dsp(dsp_core);
}

void dsp_core_hostport_dspwrite(dsp_core_t *dsp_core)
{
	/* Clear HTDE bit to say that DSP has written */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] &= 0xff-(1<<DSP_HOST_HSR_HTDE);
#if DSP_DISASM_HOSTWRITE
	fprintf(stderr, "Dsp: (D->H): Dsp HTDE=0\n");
#endif
	dsp_core_dsp2host(dsp_core);
}

static void dsp_core_hostport_cpuread(dsp_core_t *dsp_core)
{
	/* Clear RXDF bit to say that CPU has read */
	dsp_core->hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_RXDF);
#if DSP_DISASM_HOSTWRITE
	fprintf(stderr, "Dsp: (D->H): Host RXDF=0\n");
#endif
	dsp_core_dsp2host(dsp_core);
}

static void dsp_core_hostport_cpuwrite(dsp_core_t *dsp_core)
{
	/* Clear TXDE to say that CPU has written */
	dsp_core->hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_TXDE);
#if DSP_DISASM_HOSTREAD
	fprintf(stderr, "Dsp: (H->D): Host TXDE=0\n");
#endif
	dsp_core_hostport_update_trdy(dsp_core);
	dsp_core_host2dsp(dsp_core);
}

/* Read/writes on host port */

Uint8 dsp_core_read_host(dsp_core_t *dsp_core, int addr)
{
	Uint8 value;

	dsp_core->lockMutex(dsp_core);
	value = dsp_core->hostport[addr];
	if (addr == CPU_HOST_RXL) {
		dsp_core_hostport_cpuread(dsp_core);

#if DSP_DISASM_STATE
		fprintf(stderr, "Dsp: WAIT_HOSTREAD done\n");
#endif
		/* Wake up DSP if it was waiting our read */
		dsp_core->resumeThread(dsp_core);
	}
	dsp_core->unlockMutex(dsp_core);

	return value;
}

void dsp_core_write_host(dsp_core_t *dsp_core, int addr, Uint8 value)
{
	dsp_core->lockMutex(dsp_core);
	switch(addr) {
		case CPU_HOST_ICR:
			dsp_core->hostport[CPU_HOST_ICR]=value & 0xfb;
			/* Set HF1 and HF0 accordingly on the host side */
			dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] &=
					0xff-((1<<DSP_HOST_HSR_HF1)|(1<<DSP_HOST_HSR_HF0));
			dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |=
					dsp_core->hostport[CPU_HOST_ICR] & ((1<<DSP_HOST_HSR_HF1)|(1<<DSP_HOST_HSR_HF0));
			break;
		case CPU_HOST_CVR:
			dsp_core->hostport[CPU_HOST_CVR]=value & 0x9f;
			/* if bit 7=1, host command */
			if (value & (1<<7)) {
				dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HCP;
				/* Wake up DSP if needed */
				dsp_core->resumeThread(dsp_core);
			}
			break;
		case CPU_HOST_ISR:
		case CPU_HOST_TX0:
			/* Read only */
			break;
		case CPU_HOST_IVR:
		case CPU_HOST_TXH:
		case CPU_HOST_TXM:
			dsp_core->hostport[addr]=value;
			break;
		case CPU_HOST_TXL:
			dsp_core->hostport[CPU_HOST_TXL]=value;

			if (!dsp_core->running) {
				dsp_core->ramint[DSP_SPACE_P][dsp_core->bootstrap_pos] =
					(dsp_core->hostport[CPU_HOST_TXH]<<16) |
					(dsp_core->hostport[CPU_HOST_TXM]<<8) |
					dsp_core->hostport[CPU_HOST_TXL];
#if DEBUG
				fprintf(stderr, "Dsp: bootstrap p:0x%04x = 0x%06x\n",
					dsp_core->bootstrap_pos,
					dsp_core->ramint[DSP_SPACE_P][dsp_core->bootstrap_pos]);
#endif
				if (++dsp_core->bootstrap_pos == 0x200) {
#if DSP_DISASM_STATE
					fprintf(stderr, "Dsp: WAIT_BOOTSTRAP done\n");
#endif
					dsp_core->running = 1;
					dsp_core->resumeThread(dsp_core);
				}		
			} else {
				dsp_core_hostport_cpuwrite(dsp_core);

#if DSP_DISASM_STATE
				fprintf(stderr, "Dsp: WAIT_HOSTWRITE done\n");
#endif
				/* Wake up DSP if it was waiting our write */
				dsp_core->resumeThread(dsp_core);
			}
			break;
	}
	dsp_core->unlockMutex(dsp_core);
}

/*
vim:ts=4:sw=4:
*/
