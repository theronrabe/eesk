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
	FADD,
	FSUB,
	FMUL,
	FDIV,

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
