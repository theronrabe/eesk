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
#include <assembler.h>

int transferAddress;
Stack *callStack;
Stack *nameStack;

typedef struct Context {
	char publicFlag;
	char literalFlag;
	char nativeFlag;
	char staticFlag;
	char parameterFlag;
	char instructionFlag;
} Context;

int compileStatement(Table *keyWords, Table *symbols, translation *dictionary, char *src, int *SC, FILE *dst, int *LC, Context *con, int *lineCounter);
int writeAddressCalculation(FILE *fn, char *token, Table *symbols, translation *dictionary, int *LC, Context *context, int *lineCounter);
Table *prepareKeywords();
translation *prepareTranslation();
void fillOperations(FILE *dst, int *LC, Stack *operationStack, translation *dictionary);

typedef enum {
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
} KEYWORD;

#endif
