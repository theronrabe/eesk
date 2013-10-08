/*
vm.c

	This program acts as a 64-bit, zero-address, address-addressable virtual machine.

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
#include <stdio.h>
#include <stdlib.h>
#include <ffi.h>
#include <string.h>
#include <dlfcn.h>
#include <kernel.h>
#include <sys/mman.h>

char *PC = 0;
long SP = 0;
long LEN = 0;
char verboseFlag = 0;
Stack *libStack;

long *load(char *fn) {
	FILE *fp;
	unsigned char *ret = NULL;
	long i=0, j=0;

	fp = fopen(fn, "rt");
	fseek(fp, 0, SEEK_END);
	i = ftell(fp);
	rewind(fp);
	//ret = (unsigned char *)malloc(sizeof(char) * (i));
	ret = mmap(0, i, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
	fread(ret,sizeof(char),i,fp);
	mprotect(ret, i, PROT_READ|PROT_WRITE|PROT_EXEC);
	fclose(fp);

	//long offset = ret[i/WRDSZ - 1];
	long offset = * (long *)&ret[i-8];
	//PC = (long *)(((long) &ret[0]) + offset);
	PC = ret + offset;
	if(verboseFlag) printf("PC = %lx + %lx = %lx\n", ret, offset, PC);

	if(verboseFlag) {
		for(j=0;j<=i;j++) {
			if(!(j%8)) printf("%lx: ", (long)&ret[j]);
			printf("\t%02x\t", ret[j]);
			if((j%8)==7) printf("\n");
		}
	}

	return ret;
}

void main(int argc, char **argv) {
	if(argc>2) verboseFlag = 1;
	long *MEM = load(argv[1]);
	long *activationStack = malloc(1000*sizeof(long));
	activationStack += 1001;
	long *counterStack = malloc(1000*sizeof(long));
	counterStack += 1000;

	*counterStack = activationStack;
	counterStack -= 1;
	*counterStack = activationStack;
	//counterStack -= 1;
	//printf("activationStack = %lx\n\t", activationStack);
	asm volatile (
			"movq %%rsp, %%rbp\n\t"	//save root stack position in rbp
			"pushq %0\n\t"		//place address space on stack
			"movq %1, %%r12\n\t"		//push kernel location
			"movq %2, %%r10\n\t"		//push activation stack
			"movq %3, %%r11\n\t"		//push counter stack
			"callq *%4\n\t"		//pass control to user's program
			:
			:"r" (MEM), "r" (kernel), "r" (activationStack), "r" (counterStack), "r" (PC)
			);
	kernel(HALT);

	/*
	Stack *STACK = stackCreate(10000);
	libStack = stackCreate(32);

	if(verboseFlag) printf("________________________________________________________________________________\n\n");
	execute(MEM, STACK, PC);
	
	stackFree(STACK);
	*/
}

void quit(long *rsp, long *rbp) {
	//long *rsp, *rbp;
	//asm("movq %%rsp, %0\n\t": "=m" (rsp)::"memory");
	//asm("movq %%rbp, %0\n\t": "=m" (rbp)::"memory");

	//rsp += 8;
	rbp -= 2;
	//if(verboseFlag) printf("rsp = %lx\nrbp = %lx\n", rsp, rbp);
	//for(i=STACK->sp-1; i>=0; i--) {
	printf("\n");
	for(;rsp < rbp; rsp++){
		printf("| %4lx |\n", *rsp);
	}
		printf("|______|\n");
	
	//Unload libStack
	/*
	for(int i=0;i<libStack->sp;i++) {
		dlclose(libStack->array[i]);
	}
	*/
	
	exit(0);
}

void execute(long *MEM, Stack *STACK, long *address) {
	Stack *activationStack = stackCreate(10000);
	Stack *counterStack = stackCreate(128);
	stackPush(counterStack, 0);
	long tempVal;
	long *tempAddr;
	int i;
	double tempFloat1, tempFloat2, floatResult;
	char string[256];

	PC = address;
	while(1) {
//printf("%ld:\t%ld\n", PC, MEM[PC]);
		switch(*PC) {
			//Machine Control
			case(HALT):
			//	quit(MEM, STACK, PC);
				//if(verboseFlag) printf("%lp:\tHALT\n", PC);
				break;
			case(JMP):
				PC = (long *) stackPop(STACK);
				//if(verboseFlag) printf("JMP\t%lp\n", PC);
				break;
			case(HOP):
				PC += stackPop(STACK);
				//if(verboseFlag) printf("HOP\t%lp\n", PC);
				break;
			case(BRN):
				if(stackPop(STACK)) {
					PC = (long *) stackPop(STACK);
					//if(verboseFlag) printf("BRN\t%lp\n", PC);
				} else {
					stackPop(STACK);
					++PC;
				}
				break;
			case(BNE):
				if(!stackPop(STACK)) {
					PC = (long *) stackPop(STACK);
					//if(verboseFlag) printf("BNE\t%lp\n", PC);
				} else {
					//if(verboseFlag) printf("%lp\tNOT BNE\n", PC);
					stackPop(STACK);
					++PC;
				}
				break;
			case(NTV):
				//call string of format: "foo(pif)@bar.so:v" takes parameters pointer, integer, float, and returns void on stack
				i=0;
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				
				//if(verboseFlag) printf("%lp\tNTV:\t\n", PC);

				stackPush(counterStack, activationStack->sp);
				stackPush(STACK, nativeCall(tempAddr, (void *) tempVal, activationStack));
				activationStack->sp = stackPop(counterStack)-1;
				++PC;
				break;
			case(LOC):
				//locates the MEM index atop the stack into its absolute address
				tempVal = stackPop(STACK);
				stackPush(STACK, &MEM[tempVal]);
				//if(verboseFlag) printf("%lp:\tLOC:\t%lx becomes %lp\n", PC, tempVal, &MEM[tempVal]);
				++PC;
				break;
			case(DLOC):
				//turns an absolute address into its relative MEM index
				tempAddr = stackPop(STACK);
				stackPush(STACK, dloc((long) &MEM[0], tempAddr));
				//if(verboseFlag) printf("%p:\tDLOC:\t%p becomes %lx\n", PC, tempAddr, dloc((long)&MEM[0], tempAddr));
				++PC;
				break;
			case(PRNT):
				tempVal = stackPop(STACK);
				printf("PRINTD:\t%lx\n", tempVal);
				++PC;
				break;

			//Stack Control
			case(PUSH):
				stackPush(STACK, *(PC+1));
				//if(verboseFlag) printf("%p:\tPUSH:\t%lx\n", PC, *(PC+1));
				PC += 2;
				break;
			case(RPUSH):
				stackPush(STACK, PC + *(PC+1));
				//if(verboseFlag) printf("%p:\tRPUSH:\t%lx to make address %p\n", PC, *(PC+1), PC + *(PC+1));
				PC += 2;
				break;
			case(GRAB):
				stackPush(STACK, PC + 1);
				//if(verboseFlag) printf("%p:\tGRAB:\t%lx\n", PC, *(PC+1));
				PC += 2;
				break;
			case(POPTO):
				//MEM[PC+MEM[PC+1]] = stackPop(STACK);
				*(PC+*(PC+1)) = stackPop(STACK);
				//if(verboseFlag) printf("%p:\tPOPTO\t%p(%lx) = %lx\n", PC, PC+*(PC+1), *(PC+1), *(PC+*(PC+1)));
				PC += 2;
				break;
			case(POP):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				*tempAddr = tempVal;
				//if(verboseFlag) printf("%p:\tPOP\t%p = %lx\n", PC, tempAddr, tempVal);
				++PC;
				break;
			case(BPOP):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				*((char *)tempAddr) = (char) tempVal;
				//if(verboseFlag) printf("%p:\tBPOP\t%p = %lx\n", PC, tempAddr, tempVal);
				++PC;
				break;
			case(CONT):
				tempAddr = stackPop(STACK);
				stackPush(STACK, *tempAddr);
				//if(verboseFlag) printf("%p:\tCONT:\t%p is %lx\n", PC, tempAddr, *tempAddr);
				++PC;
				break;
			case(CLR):
				stackPop(STACK);
				//if(verboseFlag) printf("%p:\tCLR\n", PC);
				++PC;
				break;

			//Activation Stack
			case(JSR):
				stackPush(counterStack, activationStack->sp);
				tempAddr = STACK->array[STACK->sp-2];	//stackPop(STACK);

				//tempVal = *(((long *)(*tempAddr))+1) - (counterStack->array[counterStack->sp-1] - counterStack->array[counterStack->sp-2]);
				tempVal = *(tempAddr-1) - (counterStack->array[counterStack->sp-1] - counterStack->array[counterStack->sp-2]);
						//read above as (wordsNeeded) - ((topArg) - (bottomArg))
				activationStack->sp += tempVal;
				counterStack->array[counterStack->sp-1] += tempVal;	//account for stack memory that wasn't passed as arguments

				//tempAddr = (long *)*tempAddr + *(long *)(*tempAddr);	//combine contents with offset
				
				//if(verboseFlag) printf("%p:\tJSR\t%p, counter = %d\n", PC, tempAddr, activationStack->sp);
				PC = tempAddr;
				break;
			case(RSR):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				stackPop(STACK);			//an extra pop for the calling address
				stackPush(STACK, tempVal);
				stackPop(counterStack);
				activationStack->sp = counterStack->array[counterStack->sp-1];
				//if(verboseFlag) printf("%p:\tRSR\t to %p with %lx, counter=%d\n", PC, tempAddr, tempVal, activationStack->sp);
				PC = tempAddr;
				break;
			case(APUSH):
				tempVal = stackPop(STACK);
				stackPush(activationStack, tempVal);
				//if(verboseFlag) printf("%p:\tAPUSH\t%d = %lx\n", PC, activationStack->sp-1, tempVal);
				++PC;
				break;
			case(AGET):
				i = counterStack->array[counterStack->sp - 2] + (*(PC+1));
				tempAddr = &(activationStack->array[i]);
				stackPush(STACK, tempAddr);
				//if(verboseFlag) printf("%p:\tAGET\t%lx,  %lx:%lx\n", PC, *(PC+1), i, *tempAddr);
				PC += 2;
				break;

			//Data Manipulation
			case(ADD):
				stackPush(STACK, stackPop(STACK) + stackPop(STACK));
				//if(verboseFlag) printf("%p:\tADD\t%lx\n", PC, STACK->array[STACK->sp-1]);
				++PC;
				break;
			case(SUB):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) - tempVal);
				//if(verboseFlag) printf("%p:\tSUB\n", PC);
				++PC;
				break;
			case(MUL):
				stackPush(STACK, stackPop(STACK) * stackPop(STACK));
				//if(verboseFlag) printf("%p:\tMUL\n", PC);
				++PC;
				break;
			case(DIV):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) / tempVal);
				//if(verboseFlag) printf("%p:\tDIV\n", PC);
				++PC;
				break;
			case(MOD):
				tempVal = stackPop(STACK);
				stackPush(STACK, stackPop(STACK) % tempVal);
				//if(verboseFlag) printf("%p:\tMOD\n", PC);
				++PC;
				break;
			case(AND):
				stackPush(STACK, stackPop(STACK) & stackPop(STACK));
				//if(verboseFlag) printf("%p:\tAND\n", PC);
				++PC;
				break;
			case(OR):
				stackPush(STACK, stackPop(STACK) | stackPop(STACK));
				//if(verboseFlag) printf("%p:\tOR\n", PC);
				++PC;
				break;
			case(NOT):
				stackPush(STACK, !stackPop(STACK));
				//if(verboseFlag) printf("%p:\tNOT\n", PC);
				++PC;
				break;
			case(SHIFT):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);

				if(tempVal > 0) {
					stackPush(STACK, ((long)tempAddr) >> tempVal);
					//if(verboseFlag) printf("%p:\tSHIFT\t%lx by %lx is %lx\n", PC, tempAddr, tempVal, ((long)tempAddr)>>tempVal);
				} else {
					stackPush(STACK, ((long)tempAddr) << -tempVal);
					//if(verboseFlag) printf("%p:\tSHIFT\t%lx by %lx is %lx\n", PC, tempAddr, tempVal, ((long)tempAddr)<<-tempVal);
				}
				++PC;
				break;

			//Float manipulation
			case(FTOD):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = (long) tempFloat1;

				stackPush(STACK, tempVal);
				++PC;
				break;
			case(DTOF):
				tempFloat1 = (double) stackPop(STACK);
				stackPush(STACK, *(long *) &tempFloat1);
				++PC;
				break;
			case(PRTF):
				tempVal = stackPop(STACK);
				tempFloat1 = *(double *) &tempVal;
				printf("%p:\tPRINTS:\t%lf from value %lx\n", PC, tempFloat1, tempVal);
				++PC;
				break;
			case(FADD):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float * )&tempVal;
				floatResult = tempFloat1 + tempFloat2;

				stackPush(STACK, *(long *)&floatResult);
				//if(verboseFlag) printf("%p:\tFADD\n", PC);
				++PC;
				break;
			case(FSUB):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *)&tempVal;
				floatResult = tempFloat2 - tempFloat1;

				stackPush(STACK, *(long *)&floatResult);
				//if(verboseFlag) printf("%p:\tFSUB\n", PC);
				++PC;
				break;
			case(FMUL):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *) &tempVal;
				tempVal = stackPop(STACK);
				//tempFloat2 = *(float *) &tempVal;
				floatResult = tempFloat1 * tempFloat2;

				stackPush(STACK, *(long *)&floatResult);
				if(verboseFlag) printf("%p:\tFMUL\n", PC);
				++PC;
				break;
			case(FDIV):
				tempVal = stackPop(STACK);
				tempFloat1 = *(float *)&tempVal;
				tempVal = stackPop(STACK);
				tempFloat2 = *(float *)&tempVal;
				floatResult = tempFloat2 / tempFloat1;

				stackPush(STACK, *(long *)&floatResult);
				//if(verboseFlag) printf("%p:\tFDIV\n", PC);
				++PC;
				break;

			//string manipulation
			case(PRTC):
				tempVal = stackPop(STACK);
				printf("%p:\tPRINTS:\t%c\n", PC, (char)tempVal);
				++PC;
				break;
			case(PRTS):
				tempAddr = stackPop(STACK);
				printf("%s", (char *) tempAddr);
				++PC;
				break;

			//Comparison
			case(GT):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				stackPush(STACK, tempAddr > tempVal);
				//if(verboseFlag) printf("%p:\tGT\n", PC);
				++PC;
				break;
			case(LT):
				tempVal = stackPop(STACK);
				tempAddr = stackPop(STACK);
				stackPush(STACK, tempAddr < tempVal);
				//if(verboseFlag) printf("%p:\tLT\n", PC);
				++PC;
				break;
			case(EQ):
				stackPush(STACK, stackPop(STACK) == stackPop(STACK));
				//if(verboseFlag) printf("%p:\tEQ\n", PC);
				++PC;
				break;

			//Memory handling
			case(ALOC):
				tempVal = stackPop(STACK);
				tempAddr = (long)malloc(tempVal*sizeof(long));
				stackPush(STACK, tempAddr);
				//if(verboseFlag) printf("%p:\tALOC\n", PC);
				++PC;
				break;
			case(NEW):
				//if(verboseFlag) printf("%p:\tNEW\n", PC);
				tempVal = stackPop(STACK); //this location contains size to allocate and precedes start of copying
				tempAddr = malloc((*((long *)tempVal)+1)*sizeof(long));
				for(i=0;i<=*((long *)tempVal);i++) {
					*(tempAddr+i) = *((long*)tempVal+i);
					//if(verboseFlag) printf("\tcopying value %lx to address %p\n", *((long*)tempVal+i), tempAddr+i);
				}
				stackPush(STACK, tempAddr);
				++PC;
				break;
			case(FREE):
				tempAddr = stackPop(STACK);
				free((long *)tempAddr);
				//if(verboseFlag) printf("%p:\tFREE\n", PC);
				++PC;
				break;
			case(LOAD):
				tempAddr = stackPop(STACK);
				tempVal = dlopen((char *)tempAddr, RTLD_LAZY);
				stackPush(STACK, tempVal);
				stackPush(libStack, tempVal);

				tempVal = dlerror();
				if(tempVal) {
					printf("Error opening shared library: %s\n", (char *)tempVal);
					//quit(MEM, STACK, PC);
				}
				//if(verboseFlag) printf("%p:\tLOAD\t%s\n", PC, (char *)tempAddr);
				++PC;
				break;
			default:
				printf("Unknown opcode: %p\n at %lx", *PC, PC);
				exit(1);
		}
	}
}

long nativeCall(long *call, void *handle, Stack *STACK) {
	void (*function)();
	int i;
	void *result = 0;
	char returnChar = (char) *(call+1);
	int argc = *(call+2);
	char *functionName = (char *) (call+3+argc);
	long **argv = malloc(argc*WRDSZ);
	ffi_cif cif;

	//if(verboseFlag) printf("Argc = %d\n", argc);
	
	//pop parameter values from stack
	for(i=0;i<argc;i++) {
		argv[i] = &STACK->array[STACK->sp - (argc-i)];
		//if(verboseFlag) printf("... grabbing argument:\t%d\t%p: %lx\n", argc-i-1, argv[argc-i-1], *argv[argc-i-1]);
	}


	//make call
	if(!*call) {
		//if(verboseFlag) printf("Resolving native function %s\n", functionName);
		function = dlsym(handle, functionName);
		printf(dlerror());
		*call = function;

		//resolve return type
		switch(returnChar) {
			case('d'):
				*(call+1) = &ffi_type_sint64;
				break;
			case('f'):
				*(call+1) = &ffi_type_double;
				break;
			case('p'):
				*(call+1) = &ffi_type_pointer;
				break;
			case('v'):
				*(call+1) = &ffi_type_void;
				break;
			case('c'):
				*(call+1) = &ffi_type_uchar;
				break;
			default:
				printf("Unknown return type:\t%x\n", returnChar);
				exit(1);
				break;
		}
		//if(verboseFlag) printf("Function return type:\t%c: %p\n", returnChar, *(call+1));

		//resolve parameter types
		for(i=0;i<argc;i++) {
			//if(verboseFlag) printf("Argument type:\t%p: %c\n", call+3+i, *(char *)(call+3+i));
			switch(*(char *)(call+3+i)) {
				case('d'):
					*(call+3+i) = &ffi_type_sint64;
					break;
				case('f'):
					*(call+3+i) = &ffi_type_double;
					break;
				case('p'):
					*(call+3+i) = &ffi_type_pointer;
					break;
				case('c'):
					*(call+3+i) = &ffi_type_uchar;
					break;
				default:
					printf("Unknown argument type:\t%p: %x\n",(char) *(call+3+i));
					exit(1);
					break;
			}
			//if(verboseFlag) printf("Argument type pointer:\t%p:%p\n", call+3+i, *(call+3+i));
		}
	} else {
		//if(verboseFlag) printf("Calling native function %s\n", functionName);
		function = *call;
	}
	printf(dlerror());

	if(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argc, *(call+1), (call+3)) == FFI_OK) {
			if(returnChar == 'v') {
				ffi_call(&cif, function, NULL, argv);
			} else {
				ffi_call(&cif, function, &result, argv);
			}
	} else {
		printf("Failure in CIF preparation.\n");
		exit(1);
		return NULL;
	}

	//Clean up
	free(argv);

	//Return
	//if(verboseFlag) printf("Returning with:\t%p: %lx\n", &result, result);
	return (long) result;
}

long loc(long start, long offset) {
	return offset*8+ start;
}
long dloc(long start, long address) {
	return (address-start)/8;
}
