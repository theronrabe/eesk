/*
eeskIR.h

	This file provides an enumeration of the eesk intermediate represetation.


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
#ifndef _eeskIR.h_
#define _eeskIR.h_

#define WRDSZ 8

typedef enum {
	//machine control
	HALT,
	JMP,
	HOP,
	BRN,
	BNE,
	NTV,
	LOC,
	DLOC,
	PRNT,

	//stack control
	PUSH,	//9
	RPUSH,
	GRAB,
	POPTO,
	POP,
	BPOP,
	CONT,
	CLR,

	//activation stack
	JSR, //11
	RSR,
	APUSH,
	AGET,

	//value manipulation
	ADD,	//15
	SUB,
	MUL,
	DIV,
	MOD,
	AND,
	OR,
	NOT,
	SHIFT,

	//float manipulation
	FTOD,	//1e
	DTOF,
	PRTF,
	FADD,
	FSUB,
	FMUL,
	FDIV,

	//string manipulation
	PRTC,	//25
	PRTS,

	//comparison
	GT,	//27
	LT,
	EQ,

	//memory handling
	ALOC,	//2a
	NEW,
	FREE,
	LOAD,

	//continuation handling
	STORE,
	RESTORE,

	//new instructions
	RPOP,	//30
	IMPL,
	CREATE,
	BKSET,
	CHAR,
	NPUSH,

	//typing instructions
	TPUSH,	//36
	TSYM,
	TYPEOF,
	TASGN,
	TPOP,
	TVAL,
	TSET,
	TDUP,

	//experimental stuff
	SWAP,	//3e
	RESWAP,
	ABASE,
	
	//convenience keywords
	DATA,
	TRANSLATION_SIZE
} OPCODE;

#endif
