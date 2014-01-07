
#ifndef _definitions.h_
#define _definitions.h_

#include <stdio.h>
#include <stack.h>

//Following declarations' functions in assembler.c

typedef struct translation {
	int param;
	char dWord;
	int length;
	unsigned char *code;
} translation;

//Following declaration's functions in symbolTable.c

typedef struct Table {
	char *token;
	long val;
	long offset;
	long backset;
	char staticFlag;
	char searchUp;
	char parameterFlag;
	char type;
	struct Table *left;
	struct Table *right;
	struct Table *parent;
	struct Table *layerRoot;
} Table;

//Following declarations' functions in context.c

typedef struct Context {
	char publicFlag;
	char literalFlag;
	char nativeFlag;
	char staticFlag;
	char parameterFlag;
	char instructionFlag;
	char anonFlag;
	char typingFlag;
	char displaySymbols;
	char verboseFlag;
	long expectedLength;
	Table *symbols;
} Context;

typedef struct Compiler {
	int lineCounter;
	FILE *dst;
	char *src;
	long SC;
	long LC;
	char end;
	Table *keyWords;
	translation *dictionary;
	Stack *anonStack;
} Compiler;
#endif
