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
#ifndef _compiler.h_
#define _compiler.h_ 

#include <tokenizer.h>
#include <stack.h>
#include <symbolTable.h>
#include <fileIO.h>
#include <stdio.h>

#define WRDSZ 8

int transferAddress;
Stack *callStack;
Stack *nameStack;
//Stack *varyStack;	//stores all the addresses to vary to

typedef struct Context {
	char publicFlag;
	char literalFlag;
	char nativeFlag;
	char staticFlag;
	char parameterFlag;
	char instructionFlag;
} Context;

int compileStatement(Table *keyWords, Table *symbols, char *src, int *SC, FILE *dst, int *LC, Context *con, int *lineCounter);
void writeObj(FILE *fn, long val, int *LC);
void writeStr(FILE *fn, char *str, int *LC);
int writeAddressCalculation(FILE *fn, char *token, Table *symbols, int *LC, Context *context, int *lineCounter);
Table *prepareKeywords();
void fillOperations(FILE *dst, int *LC, Stack *operationStack);

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
	PUSH,	//6
	RPUSH,
	GRAB,
	POPTO,
	POP,
	BPOP,
	CONT,
	CLR,

	//activation stack
	JSR,
	RSR,
	APUSH,
	AGET,

	//value manipulation
	ADD,	//10
	SUB,
	MUL,
	DIV,
	MOD,
	AND,
	OR,
	NOT,
	SHIFT,

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
	LOAD,

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
	k_is, k_set,
	k_eq, k_gt, k_lt,
	k_add, k_sub, k_mul, k_div, k_mod, k_and, k_or, k_shift,
	k_fadd, k_fsub, k_fmul, k_fdiv,
	k_return,
	k_public, k_private, k_literal, k_collect, k_field, k_static,
	k_child,
	k_alloc, k_new, k_free,
	k_include,
	k_native,
	k_label,
	k_argument,
	k_redir,
	k_load, k_nativeFunction
} OPCODE;

#endif
