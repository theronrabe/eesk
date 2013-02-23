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
	HALT,
	RUN,
	VARY,
	JMP,
	BRN,
	BNE,
	BREAK,
	SKIP,
	PRNT,

	//stack control
	PUSH,
	POPTO,
	POP,
	CONT,
	CLR,

	//value manipulation
	ADD,
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
	EQ
} OPCODE;

int *load(char *fn);
void execute(int *MEM, Stack *STACK, CallList *CALLS, int address);
void varyMachine(int *MEM, Stack *STACK, CallList *CALLS);
void quit(int *MEM, Stack *STACK, int address);

int PC = 0, SP = 0;
int LEN = 0;
int verboseFlag = 0;
