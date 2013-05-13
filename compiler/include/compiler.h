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
#include "tokenizer.h"
#include "stack.h"
#include "symbolTable.h"
#include "fileIO.h"
#include <stdio.h>

int transferAddress;
char publicFlag = 0;
char literalFlag = 0;
Stack *callStack;
Stack *nameStack;
Stack *varyStack;	//stores all the addresses to vary to

int compileStatement(Table *keyWords, Table *symbols, char *src, int *SC, FILE *dst, int *LC);
void writeObj(FILE *fn, long val, int *LC);
int writeAddressCalculation(FILE *fn, char *token, Table *symbols, int *LC);
Table *prepareKeywords();
void fillOperations(FILE *dst, int *LC, Stack *operationStack);

typedef enum {
	//machine control
	HALT,
	JMP,
	BRN,
	BNE,
	PRNT,

	//stack control
	PUSH,	//5
	RPUSH,
	POPTO,
	POP,
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
	FTOD,	//18
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
	ALOC,	//30
	NEW,
	FREE,

	//language keywords
	k_if, k_while,
	k_Function,
	k_oBracket, k_cBracket,
	k_oBrace, k_cBrace,
	k_oParen, k_cParen,
	k_prnt, k_prtf, k_prtc, k_prts, k_goto,
	k_singleQuote, k_doubleQuote,
	k_int, k_char, k_pnt, k_float,
	k_begin, k_halt,
	k_clr, k_endStatement, k_cont, k_not,
	k_is,
	k_eq, k_gt, k_lt,
	k_add, k_sub, k_mul, k_div, k_mod, k_and, k_or,
	k_fadd, k_fsub, k_fmul, k_fdiv,
	k_return,
	k_public, k_private, k_literal, k_collect,
	k_child,
	k_alloc, k_new, k_free
} OPCODE;
