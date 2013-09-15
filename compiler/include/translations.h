/*
translations.h

	This file contains the constant strings required for translating eeskir to new platforms.

Copyright 2013 Theron Rabe
This file is part of Eesk.

    Eesk is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Eesk is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Eesk.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _translations.h_
#define _translations.h_
	const unsigned char c_halt[] = {0x48,0xC7,0xC7,0x00,0x00,0x00,0x00,
				0x48,0x8B,0x45,0xF0,
				0xFF,0xD0,
				0xC3, 0xC3};

	const unsigned char c_jmp[] = {0x58, 0xFF,0xE0, 0xC3, 0xC3};

	const unsigned char c_hop[] = {0x58, 0xE8,0x00,0x00,0x00,0x00,
				0x5B, 0x48,0x01,0xD8,
				0xFF,0xE0, 0xC3, 0xC3};

	const unsigned char c_brn[] = {0x58, 0x5B, 0x48,0x85,0xC0,
				0x74,0x02,
				0xFF,0xE3,
				0xC3, 0xC3};

	const unsigned char c_bne[] = {0x58, 0x5B,
				0x48,0x85,0xC0,
				0x75,0x02,
				0xFF,0xE3,
				0xC3, 0xC3};

	const unsigned char c_ntv[] = {0x5E, 0x5F,
				0x48,0x8B,0x55,0x10,
				0x48,0x8B,0x45,0x00,
				0xFF,0x10,
				0xC3, 0xC3};

	const unsigned char c_loc[] = {0x48,0x8B,0x45,0x00,
				0x5B,
				0x48,0x8D,0x04,0x00,
				0x50,
				0xC3, 0xC3};

	unsigned char c_push[] = {0x48,0xB8,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x50,
				0xC3, 0xC3};

	const unsigned char c_rpush[] = {0x48,0xD8,0x05,
				0x00,0x00,0x00,0x00,
				0x50,0xC3, 0xC3};

	const unsigned char c_grab[] = {0xE8,0x00,0x00,0x00,0x00, 0xC3, 0xC3};

	const unsigned char c_pop[] = {0x5B, 0x58,
				0x48,0x89,0x18,
				0xC3, 0xC3};

	const unsigned char c_bpop[] = {0x5B, 0x58,
				0x88,0x18,
				0xC3, 0xC3};

	const unsigned char c_cont[] = {0x58, 0xFF,0x30, 0xC3, 0xC3};

	const unsigned char c_clr[] = {0x58, 0xC3, 0xC3};

	const unsigned char c_jsr[] = {0x49,0x89,0xE0,
				0x48,0x8B,0x65,0x18,
				0x48,0x8B,0x4D,0x10,
				0x48,0x8B,0x54,0x24,0x08,
				0x51,
				0x48,0x29,0xD1,
				0x48,0x89,0x65,0x18,
				0x4C,0x89,0xC4,
				0x48,0x8B,0x44,0x24,0x10,
				0x48,0x8B,0x18,
				0x48,0x29,0xCB,
				0x48,0x01,0x5D,0x10,
				0x48,0x8B,0x55,0x18,
				0x48,0x01,0x5A,0x08,
				0xFF,0xE0,
				0xC3, 0xC3};

	const unsigned char c_rsr[] = {0x58, 0x5B, 0x59, 0x50,
				0x48,0x83,0x6D,0x18,0x08,
				0x48,0x8B,0x45,0x18,
				0x48,0x8B,0x40,0x08,
				0x48,0x89,0x45,0x10,
				0xFF,0xE1,
				0xC3, 0xC3};

	const unsigned char c_apush[] = {0x58,
				0x48,0x89,0xE0,
				0x48,0x8B,0x65,0x10,
				0x50,
				0x48,0x89,0x65,0x10,
				0x4C,0x89,0xC4,
				0xC3, 0xC3};

	const unsigned char c_aget[] = {0x48,0xB8,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x48,0x8B,0x55,0x18,
				0x48,0x2B,0x42,0x10,
				0xFF,0x30,
				0xC3, 0xC3};

	const unsigned char c_add[] = {0x58,
				0x48,0x01,0x44,0x24,0x00,
				0xC3, 0xC3};

	const unsigned char c_sub[] = {0x58,
				0x48,0x29,0x44,0x24,0x00,
				0xC3, 0xC3};

	const unsigned char c_mul[] = {0x5B,
				0x48,0xF7,0xE3,
				0x50,
				0xC3, 0xC3};

	const unsigned char c_div[] = {0x5B, 0x58,
				0x48,0xF7,0xFB,
				0x50,
				0xC3, 0xC3};

	const unsigned char c_mod[] = {0x5B, 0x58,
				0x48,0xF7,0xF8,
				0x52,
				0xC3, 0xC3};

	const unsigned char c_and[] = {0x58,
				0x48,0x21,0x44,0x24,0x08,
				0xC3, 0xC3};

	const unsigned char c_or[] = {0x58,
				0x48,0x09,0x44,0x24,0x08,
				0xC3, 0xC3};

	const unsigned char c_not[] = {0x48,0xF7,0x54,0x24,0x08,
				0xC3, 0xC3};

	const unsigned char c_gt[] = {0x58, 0x5B,
				0x48,0x39,0xC3,
				0x7f,0x04,
				0x6a,0x00,
				0xEB,0x02,
				0x6A,0x01,
				0xC3, 0xC3};

	const unsigned char c_lt[] = {0x58, 0x5B,
				0x48,0x39,0xC3,
				0x7C,0x04,
				0x6A,0x00,
				0xEB,0x02,
				0x6A,0x01,
				0xC3, 0xC3};

	const unsigned char c_eq[] = {0x58, 0x5B,
				0x48,0x39,0xC3,
				0x74,0x04,
				0x6A,0x00,
				0xEB,0x02,
				0x6A,0x01,
				0xC3, 0xC3};
	const unsigned char c_data[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0xC3, 0xC3};

	const unsigned char c_nop[] = {0x90, 0xC3, 0xC3};
#endif