/*
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
#include <stack.h>
#define WRDSZ 8

typedef enum {
	//machine control
	HALT,	//0
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
	POPTO,
	POP,
	BPOP,
	CONT,
	CLR,

	//value manipulation
	ADD,	//10
	SUB,
	MUL,
	DIV,
	MOD,
	AND,
	OR,
	NOT,

	//float manipulation
	FTOD,
	DTOF,
	PRTF,
	FADD,
	FSUB,
	FMUL,
	FDIV,

	//string manipulation
	PRTC,
	PRTS,

	//comparison
	GT,
	LT,
	EQ,

	//memory handling
	ALOC,
	NEW,
	FREE
} OPCODE;

long *load(char *fn);
void execute(long *MEM, Stack *STACK, long *address);
void quit(long *MEM, Stack *STACK, long address);
void nativeCall(char *cs, Stack *STACK);
long loc(long start, long offset);
long dloc(long start, long address);
