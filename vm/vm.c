/*
vm.c

	This program acts as a 32-bit, zero-address, address-addressable virtual machine. "Registers" for the machine can be accessed as negative addresses.
	The program counter is located at address -1.

	The machine has three basic states, read, execute, and vary. Execution state doesn't begin until a RUN opcode is executed. Variation loop begins
	once the VARY opcode has been reached. Once in variation state, functions stored in a call list are continuously executed until a HALT opcode has
	been encountered.

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
	//PC = ret[(i/4)-1];
	PC = ret[ i/sizeof(int)-1 ];
	//printf("Transferring to: %d\n", PC);


	if(verboseFlag) {
		for(j=0;j<=i/4;j++) {
			printf("%d:\t%d\n", j, ret[j]);
		}
	}

	return ret;
}

void main(int argc, char **argv) {
	if(argc>2) verboseFlag = 1;

	Stack *STACK = stackCreate(128);
	CallList *CALLS = NULL;
	int *MEM = load(argv[1]);

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
	int i;
	float tempFloat1, tempFloat2, floatResult;

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
				if(verboseFlag) printf("%p:\tHALT\n", &MEM[PC]);
			case(VARY):
				callAdd(CALLS, stackPop(STACK));
				++PC;
				break;
			case(JMP):
				PC = stackPop(STACK);
				if(verboseFlag) printf("JMP\t%p\n", &MEM[PC]);
				break;
			case(BRN):
				if(stackPop(STACK)) {
					PC = stackPop(STACK);
					if(verboseFlag) printf("BRN\t%p\n", &MEM[PC]);
				} else {
					stackPop(STACK);
					++PC;
				}
				break;
			case(BNE):
				if(!stackPop(STACK)) {
					PC = stackPop(STACK);
					if(verboseFlag) printf("BNE\t%p\n", &MEM[PC]);
				} else {
					stackPop(STACK);
					++PC;
				}
				break;
			case(SKIP):
				break;
			case(PRNT):
				tempVal = stackPop(STACK);
				printf("%p:\tPRINTS:\t%d\n", &MEM[PC], tempVal);
				++PC;
				break;

			//Stack Control
			case(PUSH):
				stackPush(STACK, MEM[PC+1]);
				if(verboseFlag) printf("%p:\tPUSH:\t%d\n", &MEM[PC], MEM[PC+1]);
				PC += 2;
				break;
			case(POPTO):
				MEM[MEM[PC+1]] = stackPop(STACK);
				PC += 2;
				if(verboseFlag) printf("%p:\tPOPTO\t%d\n", &MEM[PC], MEM[PC-1]);
				break;
			case(POP):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				MEM[tempAddr] = tempVal;
				if(verboseFlag) printf("%p:\tPOP\n", &MEM[PC]);
				++PC;
				break;
			case(CONT):
				tempAddr = stackPop(STACK);
				stackPush(STACK, MEM[tempAddr]);
				if(verboseFlag) printf("%p:\tCONT:\t%d\n", &MEM[PC], MEM[tempAddr]);
				++PC;
				break;
			case(CLR):
				stackPop(STACK);
				if(verboseFlag) printf("%p:\tCLR\n", &MEM[PC]);
				++PC;
				break;

			//Data Manipulation
			case(ADD):
				stackPush(STACK, stackPop(STACK) + stackPop(STACK));
				if(verboseFlag) printf("%p:\tADD\n", &MEM[PC]);
				++PC;
				break;
			case(SUB):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) - tempVal);
				if(verboseFlag) printf("%p:\tSUB\n", &MEM[PC]);
				++PC;
				break;
			case(MUL):
				stackPush(STACK, stackPop(STACK) * stackPop(STACK));
				if(verboseFlag) printf("%p:\tMUL\n", &MEM[PC]);
				++PC;
				break;
			case(DIV):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) / tempVal);
				if(verboseFlag) printf("%p:\tDIV\n", &MEM[PC]);
				++PC;
				break;
			case(MOD):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) % tempVal);
				if(verboseFlag) printf("%p:\tMOD\n", &MEM[PC]);
				++PC;
				break;
			case(AND):
				stackPush(STACK, stackPop(STACK) & stackPop(STACK));
				if(verboseFlag) printf("%p:\tAND\n", &MEM[PC]);
				++PC;
				break;
			case(OR):
				stackPush(STACK, stackPop(STACK) | stackPop(STACK));
				if(verboseFlag) printf("%p:\tOR\n", &MEM[PC]);
				++PC;
				break;
			case(NOT):
				stackPush(STACK, !stackPop(STACK));
				if(verboseFlag) printf("%p:\tNOT\n", &MEM[PC]);
				++PC;
				break;

			//Float manipulation
			case(FTOD):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = (int) tempFloat1;

				stackPush(STACK, tempVal);
				++PC;
				break;
			case(DTOF):
				tempFloat1 = (float) stackPop(STACK);
				stackPush(STACK, *(int *) &tempFloat1);
				++PC;
				break;
			case(PRTF):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				printf("%p:\tPRINTS:\t%f\n", &MEM[PC], tempFloat1);
				++PC;
				break;
			case(FADD):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float * )&tempVal;
				floatResult = tempFloat1 + tempFloat2;

				stackPush(STACK, *(int *)&floatResult);
				if(verboseFlag) printf("%p:\tFADD\n", &MEM[PC]);
				++PC;
				break;
			case(FSUB):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *)&tempVal;
				floatResult = tempFloat2 - tempFloat1;

				stackPush(STACK, *(int *)&floatResult);
				if(verboseFlag) printf("%p:\tFSUB\n", &MEM[PC]);
				++PC;
				break;
			case(FMUL):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *) &tempVal;
				floatResult = tempFloat1 * tempFloat2;

				stackPush(STACK, *(int *)&floatResult);
				if(verboseFlag) printf("%p:\tFMUL\n", &MEM[PC]);
				++PC;
				break;
			case(FDIV):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *)&tempVal;
				floatResult = tempFloat2 / tempFloat1;

				stackPush(STACK, *(int *)&floatResult);
				if(verboseFlag) printf("%p:\tFDIV\n", &MEM[PC]);
				++PC;
				break;

			//string manipulation
			case(PRTC):
				tempVal = stackPop(STACK);
				printf("%p:\tPRINTS:\t%c\n", &MEM[PC], tempVal);
				++PC;
				break;
			case(PRTS):
				tempAddr = stackPop(STACK);
				tempVal = 0;
				while(MEM[tempAddr+tempVal]) {
					printf("%c", MEM[tempAddr+tempVal]);
					++tempVal;
				}
				printf("\n");
				++PC;
				break;

			//Comparison
			case(GT):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				stackPush(STACK, tempAddr > tempVal);
				if(verboseFlag) printf("%p:\tGT\n", &MEM[PC]);
				++PC;
				break;
			case(LT):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				stackPush(STACK, tempAddr < tempVal);
				if(verboseFlag) printf("%p:\tLT\n", &MEM[PC]);
				++PC;
				break;
			case(EQ):
				stackPush(STACK, stackPop(STACK) == stackPop(STACK));
				if(verboseFlag) printf("%p:\tEQ\n", &MEM[PC]);
				++PC;
				break;
		}
	}
}
