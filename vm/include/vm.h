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
#include <stdlib.h>
#include <stdio.h>
#include "stack.h"
#include "callList.h"

typedef enum {
	//machine control
	HALT,	//0
	JMP,
	BRN, BNE, NTV,
	LOC,
	DLOC,
	PRNT,

	//stack control
	PUSH,	//6
	RPUSH,
	POPTO,
	POP,
	CONT,
	CLR,

	//value manipulation
	ADD,	//c
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
void execute(long *MEM, Stack *STACK, CallList *CALLS, long address);
void quit(long *MEM, Stack *STACK, long address);
void nativeCall(char *cs, Stack *STACK);
long loc(long start, long offset);
long dloc(long start, long address);

long PC = 0, SP = 0;
int LEN = 0;
int verboseFlag = 0;
