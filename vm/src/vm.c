/*
vm.c

	TODO:
		change to 64-bit word size (for the sake of pointers)
		change default addressing mode to relative
		add direct addressing mode

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
#include <vm.h>
#include <stdlib.h>

long *load(char *fn) {
	FILE *fp;
	long *ret = NULL;
	int i=0, j=0;

	fp = fopen(fn, "rt");
	fseek(fp, 0, SEEK_END);
	i = ftell(fp);
	rewind(fp);
	ret = (long *)malloc(sizeof(char) * (i));
	fread(ret,sizeof(char),i,fp);
	fclose(fp);

	PC = ret[ i/sizeof(long)-1 ];

	if(verboseFlag) {
		for(j=0;j<=i/sizeof(long);j++) {
			printf("%x:\t%lx\n", j, ret[j]);
		}
	}

	return ret;
}

void main(int argc, char **argv) {
	if(argc>2) verboseFlag = 1;

	Stack *STACK = stackCreate(128);
	CallList *CALLS = NULL;
	long *MEM = load(argv[1]);

	execute(MEM, STACK, CALLS, PC);
	
	stackFree(STACK);
	callFree(CALLS);
}

void quit(long *MEM, Stack *STACK, long PC) {
	printf("Machine halts on address %lx\n\n", PC);
	
	int i;
	for(i=STACK->sp-1; i>=0; i--) {
		printf("| %4lx |\n", STACK->array[i]);
	}
		printf("|______|\n");
	
	exit(0);
}

void execute(long *MEM, Stack *STACK, CallList *CALLS, long address) {
	long tempVal, tempAddr;
	int i;
	float tempFloat1, tempFloat2, floatResult;

	PC = address;
	while(1) {
//printf("%ld:\t%ld\n", PC, MEM[PC]);
		switch(MEM[PC]) {
			//Machine Control
			case(HALT):
				quit(MEM, STACK, PC);
				if(verboseFlag) printf("%lx:\tHALT\n", PC);
				break;
			case(JMP):
				PC = stackPop(STACK);
				if(verboseFlag) printf("JMP\t%lx\n", PC);
				break;
			case(BRN):
				if(stackPop(STACK)) {
					PC = stackPop(STACK);
					if(verboseFlag) printf("BRN\t%lx\n", PC);
				} else {
					stackPop(STACK);
					++PC;
				}
				break;
			case(BNE):
				if(!stackPop(STACK)) {
					PC = stackPop(STACK);
					if(verboseFlag) printf("BNE\t%lx\n", PC);
				} else {
					stackPop(STACK);
					++PC;
				}
				break;
			case(PRNT):
				tempVal = stackPop(STACK);
				printf("%lx:\tPRINTS:\t%lx\n", PC, tempVal);
				++PC;
				break;

			//Stack Control
			case(PUSH):
				stackPush(STACK, MEM[PC+1]);
				if(verboseFlag) printf("%lx:\tPUSH:\t%lx\n", PC, MEM[PC+1]);
				PC += 2;
				break;
			case(RPUSH):
				stackPush(STACK, PC + MEM[PC+1]);
				if(verboseFlag) printf("%lx:\tRPUSH:\t%lx to make %lx\n", PC, MEM[PC+1], PC+MEM[PC+1]);
				PC += 2;
				break;
			case(POPTO):
				//made this relative for now
				MEM[PC+MEM[PC+1]] = stackPop(STACK);
				if(verboseFlag) printf("%lx:\tPOPTO\t%lx = %lx\n", PC, PC+MEM[PC+1], MEM[PC+MEM[PC+1]]);
				PC += 2;
				break;
			case(POP):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				MEM[tempAddr] = tempVal;
				if(verboseFlag) printf("%lx:\tPOP\t%lx = %lx\n", PC, tempAddr, tempVal);
				++PC;
				break;
			case(CONT):
				tempAddr = stackPop(STACK);
				stackPush(STACK, MEM[tempAddr]);
				if(verboseFlag) printf("%lx:\tCONT:\t%lx is %lx\n", PC, tempAddr, MEM[tempAddr]);
				++PC;
				break;
			case(CLR):
				stackPop(STACK);
				if(verboseFlag) printf("%lx:\tCLR\n", PC);
				++PC;
				break;

			//Data Manipulation
			case(ADD):
				stackPush(STACK, stackPop(STACK) + stackPop(STACK));
				if(verboseFlag) printf("%lx:\tADD\n", PC);
				++PC;
				break;
			case(SUB):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) - tempVal);
				if(verboseFlag) printf("%lx:\tSUB\n", PC);
				++PC;
				break;
			case(MUL):
				stackPush(STACK, stackPop(STACK) * stackPop(STACK));
				if(verboseFlag) printf("%lx:\tMUL\n", PC);
				++PC;
				break;
			case(DIV):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) / tempVal);
				if(verboseFlag) printf("%lx:\tDIV\n", PC);
				++PC;
				break;
			case(MOD):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) % tempVal);
				if(verboseFlag) printf("%lx:\tMOD\n", PC);
				++PC;
				break;
			case(AND):
				stackPush(STACK, stackPop(STACK) & stackPop(STACK));
				if(verboseFlag) printf("%lx:\tAND\n", PC);
				++PC;
				break;
			case(OR):
				stackPush(STACK, stackPop(STACK) | stackPop(STACK));
				if(verboseFlag) printf("%lx:\tOR\n", PC);
				++PC;
				break;
			case(NOT):
				stackPush(STACK, !stackPop(STACK));
				if(verboseFlag) printf("%lx:\tNOT\n", PC);
				++PC;
				break;

			//Float manipulation
			case(FTOD):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = (long) tempFloat1;

				stackPush(STACK, tempVal);
				++PC;
				break;
			case(DTOF):
				tempFloat1 = (float) stackPop(STACK);
				stackPush(STACK, *(long *) &tempFloat1);
				++PC;
				break;
			case(PRTF):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				printf("%lx:\tPRINTS:\t%f\n", PC, tempFloat1);
				++PC;
				break;
			case(FADD):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float * )&tempVal;
				floatResult = tempFloat1 + tempFloat2;

				stackPush(STACK, *(long *)&floatResult);
				if(verboseFlag) printf("%lx:\tFADD\n", PC);
				++PC;
				break;
			case(FSUB):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *)&tempVal;
				floatResult = tempFloat2 - tempFloat1;

				stackPush(STACK, *(long *)&floatResult);
				if(verboseFlag) printf("%lx:\tFSUB\n", PC);
				++PC;
				break;
			case(FMUL):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *) &tempVal;
				floatResult = tempFloat1 * tempFloat2;

				stackPush(STACK, *(long *)&floatResult);
				if(verboseFlag) printf("%lx:\tFMUL\n", PC);
				++PC;
				break;
			case(FDIV):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *)&tempVal;
				floatResult = tempFloat2 / tempFloat1;

				stackPush(STACK, *(long *)&floatResult);
				if(verboseFlag) printf("%lx:\tFDIV\n", PC);
				++PC;
				break;

			//string manipulation
			case(PRTC):
				tempVal = stackPop(STACK);
				printf("%lx:\tPRINTS:\t%c\n", PC, (char)tempVal);
				++PC;
				break;
			case(PRTS):
				tempAddr = stackPop(STACK);
				tempVal = 0;
				while(MEM[tempAddr+tempVal]) {
					printf("%c", (char)MEM[tempAddr+tempVal]);
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
				if(verboseFlag) printf("%lx:\tGT\n", PC);
				++PC;
				break;
			case(LT):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				stackPush(STACK, tempAddr < tempVal);
				if(verboseFlag) printf("%lx:\tLT\n", PC);
				++PC;
				break;
			case(EQ):
				stackPush(STACK, stackPop(STACK) == stackPop(STACK));
				if(verboseFlag) printf("%lx:\tEQ\n", PC);
				++PC;
				break;

			//Memory handling
			case(ALOC):
				tempVal = stackPop(STACK);
				tempAddr = (long)malloc(tempVal*sizeof(long));
				stackPush(STACK, (long *)tempAddr - &MEM[0]);
				if(verboseFlag) printf("%lx:\tALOC\n", PC);
				++PC;
				break;
			case(NEW):
				if(verboseFlag) printf("%lx:\tNEW\n", PC);
				tempVal = stackPop(STACK); //this location contains size to allocate and precedes start of copying
				tempAddr = (long *)malloc(MEM[tempVal]*sizeof(long)) - &MEM[0];
				for(i=0;i<MEM[tempVal];i++) {
					MEM[tempAddr+i] = MEM[tempVal+1+i];
					if(verboseFlag) printf("\tcopying value %lx to %lx\n", MEM[tempVal+1+i], tempAddr+i);
				}
				stackPush(STACK, tempAddr);
				++PC;
				break;
			case(FREE):
				tempAddr = stackPop(STACK);
				free((long *)(tempAddr + &MEM[0]));
				if(verboseFlag) printf("%lx:\tFREE\n", PC);
				++PC;
				break;
			default:
				printf("Unknown opcode: %lx\n at %lx", MEM[PC], PC);
				exit(1);
		}
	}
}
