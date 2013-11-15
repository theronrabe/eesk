
#ifndef _definitions.h_
#define _definitions.h_

#include <stdio.h>
#include <stack.h>

typedef struct translation {
	int param;
	char dWord;
	int length;
	unsigned char *code;
} translation;

typedef struct Table {
	char *token;
	long val;
	long offset;
	long backset;
	char staticFlag;
	char searchUp;
	char parameterFlag;
	struct Table *left;
	struct Table *right;
	struct Table *parent;
	struct Table *layerRoot;
} Table;

typedef struct Context {
	char publicFlag;
	char literalFlag;
	char nativeFlag;
	char staticFlag;
	char parameterFlag;
	char instructionFlag;
	char anonFlag;
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
