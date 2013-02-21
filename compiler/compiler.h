#include "tokenizer.h"
#include "stack.h"
#include "symbolTable.h"
#include "fileIO.h"
#include <stdio.h>

int transferAddress;
Stack *callStack;
Stack *nameStack;
Stack *varyStack;	//stores all the addresses to vary to

int compileStatement(Table *keyWords, Table *symbols, char *src, int *SC, FILE *dst, int *LC);
void writeObj(FILE *fn, int val, int *LC);
Table *prepareKeywords();
void fillOperations(FILE *dst, int *LC, Stack *operationStack);
void fillVary(FILE *dst, int *LC);

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
	PUSH,	//9
	POPTO,
	POP,
	CONT,
	CLR,

	//value manipulation
	ADD,	//14
	SUB,
	MUL,
	DIV,
	MOD,
	AND,
	OR,
	NOT,

	//float manipulation
	FADD,	//22
	FSUB,
	FMUL,
	FDIV,

	//comparison
	GT,	//26
	LT,
	EQ,

	//language keywords
	k_if, k_while,
	k_State,
	k_VaryForm,	//VaryForm = high level
	k_Function,
	k_oBracket, k_cBracket,
	k_oBrace, k_cBrace,
	k_oParen, k_cParen,
	k_prnt,
	k_singleQuote, k_doubleQuote,
	k_vary,
	k_by,
	k_int, k_char, k_pnt, k_float,
	k_begin, k_halt,
	k_clr, k_endStatement, k_cont, k_not,
	k_is,
	k_eq, k_gt, k_lt,
	k_add, k_sub, k_mul, k_div, k_mod, k_and, k_or,
	k_fadd, k_fsub, k_fmul, k_fdiv,
	k_return,
	k_public, k_private, k_child
} OPCODE;
