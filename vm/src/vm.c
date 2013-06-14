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
#include <stdlib.h>
#include <ffi.h>
#include <string.h>
#include <dlfcn.h>

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
	char string[256];

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
			case(NTV):
				//call string of format: "foo(pif)@bar.so:v" takes parameters pointer, integer, float, and returns void on stack
				i=0;
				tempAddr = stackPop(STACK);
				do {
					string[i] = (char) MEM[tempAddr + i];
					++i;
				} while(string[i-1]);

				nativeCall(string, STACK);
				++PC;
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
				printf("%lx:\tPRINTS:\t%lf\n", PC, tempFloat1);
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

void nativeCall(char *cs, Stack *STACK) {
	char *functionName, *parameters, *libName, *returnType;
	int i;
	long result = -1;
	ffi_type *ret;

	//format reminder: "foo(sid)@bar.so:f"
	functionName = strtok(cs, "()@:");
	parameters = strtok(NULL, "()@:");
	libName = strtok(NULL, "()@:");
	returnType = strtok(NULL, "()@:");

	//printf("func: %s\nparam: %s\nlib: %s\nret: %s\n", functionName, parameters, libName, returnType);

	//find parameter count and types
	int argc = strlen(parameters);
	ffi_type **types = malloc(sizeof(ffi_type*) * argc);
	void **argv = malloc(sizeof(void*) * argc);
	long *args = malloc(sizeof(void*) * argc);
	for(i=0;i<argc;i++) {
		switch(parameters[i]) {
			case('i'):
				types[i] = &ffi_type_sint64;
				break;
			case('f'):
				types[i] = &ffi_type_double;
				break;
			case('p'):
				types[i] = &ffi_type_pointer;
				break;
			case('c'):
				types[i] = &ffi_type_uchar;
				break;
		}
		//printf("argument %d is type %p\n", i, types[i]);
	}

	//pop parameter values from stack
	for(i=0;i<argc;i++) {
		args[i] = stackPop(STACK);
		argv[i] = &args[i];
		//printf("argument %d is value %lx at address %p\n", i, args[i], argv[i]);
	}

	//resolve return type
	switch(returnType[0]) {
		case('i'):
			ret = &ffi_type_sint64;
			break;
		case('f'):
			ret = &ffi_type_double;
			break;
		case('p'):
			ret = &ffi_type_pointer;
			break;
		case('c'):
			ret = &ffi_type_uchar;
			break;
		case('v'):
			ret = &ffi_type_void;
			break;
	}

	//make call
	printf("Calling %s in %s with %d args: %lx\n", functionName, libName, argc, args[0]);
	void *handle = dlopen(libName, RTLD_LAZY);
	if(!handle) printf("Error opening shared library: %s\n", dlerror());
	void (*function)() = dlsym(handle, functionName);
	printf(dlerror());

	ffi_cif cif;
	if(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argc, ret, types) == FFI_OK) {
	printf("result before: %lx\n", result);
		if(returnType[0] != 'v') {
			ffi_call(&cif, function, &result, argv);
		} else {
			ffi_call(&cif, function, NULL, argv);
		}
	printf("result after: %lx\n", result);
	} else {
		printf("Failure in CIF preparation.\n");
	}

	//push result to stack
	if(returnType[0] != 'v') {
		stackPush(STACK, result);
	}
	printf("Returning from native call with value %lx\n", result);

	//clean up
	//free(functionName);
	//free(parameters);
	//free(libName);
	free(types);
	free(argv);
	free(args);
	//free(returnType);
	dlclose(handle);
}
