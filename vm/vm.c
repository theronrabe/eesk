/*
Theron Rabe
vm.c
2/10/2013

	This program acts as a 32-bit, zero-address, address-addressable virtual machine. "Registers" for the machine can be accessed as negative addresses.
	The program counter is located at address -1.

	The machine has three basic states, read, execute, and vary. Execution state doesn't begin until a RUN opcode is executed. Variation loop begins
	once the VARY opcode has been reached. Once in variation state, functions stored in a call list are continuously executed until a HALT opcode has
	been encountered.
*/
#include "vm.h"

int *load(char *fn) {
	FILE *fp;
	int *ret = NULL;
	int i=0, j=0;

	fp = fopen(fn, "rt");
	fseek(fp, 0, SEEK_END);
	i = ftell(fp);
	rewind(fp);
	ret = (int *)malloc(sizeof(char) * (i));
	fread(ret,sizeof(char),i,fp);
	fclose(fp);

	//Grab transfer address
	PC = ret[(i/4)-1];
	printf("Transferring to: %d\n", PC);


	for(j=0;j<=i/4;j++) {
		//printf("%d:\t%d\n", j, ret[j]);
	}

	return ret;
}

void main(int argc, char **argv) {
	Stack *STACK = stackCreate(128);
	CallList *CALLS = NULL;
	int *MEM = load(argv[1]);
	
printf("executing machine\n");
	execute(MEM, STACK, CALLS, PC);
	
	stackFree(STACK);
	callFree(CALLS);
}

void quit(int *MEM, Stack *STACK, int PC) {
	printf("Machine halts on address %d\n\n", PC);
	
	int i;
	for(i=STACK->sp-1; i>=0; i--) {
		printf("| %4d |\n", STACK->array[i]);
	}
		printf(" ______\n");
	
	exit(0);
}

void varyMachine(int *MEM, Stack *STACK, CallList *CALLS) {
	while(MEM[PC] != HALT) {
		CallList *i = CALLS;
		
		while(i) {
			execute(MEM, STACK, CALLS, i->address);
			i = i->next;
		}
	}
}

void execute(int *MEM, Stack *STACK, CallList *CALLS, int address) {
	int tempVal, tempAddr;
	int tempFloat1, tempFloat2;

	PC = address;
	while(MEM[PC] != BREAK) {
//printf("%d:\t%d\n", PC, MEM[PC]);
		switch(MEM[PC]) {
			case(RUN):
				++PC;
				break;
			//Machine Control
			case(HALT):
				quit(MEM, STACK, PC);
				printf("%d:\tHALT\n", PC);
			case(VARY):
				callAdd(CALLS, stackPop(STACK));
				++PC;
				break;
			case(JMP):
				PC = stackPop(STACK);
				printf("JMP\t%d\n", PC);
				break;
			case(BRN):
				if(stackPop(STACK)) {
					PC = stackPop(STACK);
					printf("JMP\t%d\n", PC);
				} else {
					stackPop(STACK);
					++PC;
				}
				break;
			case(BNE):
				if(!stackPop(STACK)) {
					PC = stackPop(STACK);
					printf("JMP\t%d\n", PC);
				} else {
					stackPop(STACK);
					++PC;
				}
				break;
			case(SKIP):
				break;
			case(PRNT):
				tempVal = stackPop(STACK);
				printf("\n%d:\tPRINTS:\t%d\n\n", PC, tempVal);
				++PC;
				break;

			//Stack Control
			case(PUSH):
				stackPush(STACK, MEM[PC+1]);
				printf("%d:\tPUSH:\t%d\n", PC, MEM[PC+1]);
				PC += 2;
				break;
			case(POPTO):
				MEM[MEM[PC+1]] = stackPop(STACK);
				PC += 2;
				printf("%d:\tPOPTO\t%d\n", PC, MEM[PC-1]);
				break;
			case(POP):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				MEM[tempAddr] = tempVal;
				printf("%d:\tPOP\n", PC);
				++PC;
				break;
			case(CONT):
				tempAddr = stackPop(STACK);
				stackPush(STACK, MEM[tempAddr]);
				printf("%d:\tCONT:\t%d\n", PC, MEM[tempAddr]);
				++PC;
				break;
			case(CLR):
				stackPop(STACK);
				printf("%d:\tCLR\n", PC);
				++PC;
				break;

			//Data Manipulation
			case(ADD):
				stackPush(STACK, stackPop(STACK) + stackPop(STACK));
				printf("%d:\tADD\n", PC);
				++PC;
				break;
			case(SUB):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) - tempVal);
				printf("%d:\tSUB\n", PC);
				++PC;
				break;
			case(MUL):
				stackPush(STACK, stackPop(STACK) * stackPop(STACK));
				printf("%d:\tMUL\n", PC);
				++PC;
				break;
			case(DIV):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) / tempVal);
				printf("%d:\tDIV\n", PC);
				++PC;
				break;
			case(MOD):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) % tempVal);
				printf("%d:\tMOD\n", PC);
				++PC;
				break;
			case(AND):
				stackPush(STACK, stackPop(STACK) & stackPop(STACK));
				printf("%d:\tAND\n", PC);
				++PC;
				break;
			case(OR):
				stackPush(STACK, stackPop(STACK) | stackPop(STACK));
				printf("%d:\tOR\n", PC);
				++PC;
				break;
			case(NOT):
				stackPush(STACK, !stackPop(STACK));
				printf("%d:\tNOT\n", PC);
				++PC;
				break;

			//Float manipulation
			case(FADD):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float * )&tempVal;

				stackPush(STACK, tempFloat1 + tempFloat2);
				printf("%d:\tFADD\n", PC);
				++PC;
				break;
			case(FSUB):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *)&tempVal;

				stackPush(STACK, tempFloat1 - tempFloat2);
				printf("%d:\tFSUB\n", PC);
				++PC;
				break;
			case(FMUL):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *) &tempVal;

				stackPush(STACK, tempFloat1 * tempFloat2);
				printf("%d:\tFMUL\n", PC);
				++PC;
				break;
			case(FDIV):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *)&tempVal;

				stackPush(STACK, tempFloat1 / tempFloat2);
				printf("%d:\tFDIV\n", PC);
				++PC;
				break;

			//Comparison
			case(GT):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				stackPush(STACK, tempAddr > tempVal);
				printf("%d:\tGT\n", PC);
				++PC;
				break;
			case(LT):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				stackPush(STACK, tempAddr < tempVal);
				printf("%d:\tLT\n", PC);
				++PC;
				break;
			case(EQ):
				stackPush(STACK, stackPop(STACK) == stackPop(STACK));
				printf("%d:\tEQ\n", PC);
				++PC;
				break;
		}
	}
}
