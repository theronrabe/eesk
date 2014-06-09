/*
vm.c

	This file contains all the functionality that an Eesk-optimal machine would provide, but reality
	does not. Historically, the program described in this file was an interpreter targeting the Eesk
	Intermediate Representation. Now, it provides the high-level functionality such as operating
	system interaction.

	TODO:
		- main() should be placed somewhere else
		- fix the memory leak in quit()

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
#include <eeskIR.h>
#include <stdio.h>
#include <stdlib.h>
#include <ffi.h>
#include <string.h>
#include <dlfcn.h>
#include <kernel.h>
#include <sys/mman.h>
#include <compiler.h>
#include <jit.h>

char *PC = 0;
long SP = 0;
long LEN = 0;
char verboseFlag = 0;
Stack *loadStack;
Stack *libStack;
long *dataStack, *activationStack, *counterStack, *MEM, *typeStack, *otypeStack;
long *oldDStack, *oldAStack, *oldCStack, *oldTStack, *oldOTStack, MEMlength;
translation *dictionary;

/*
	load reads an e.out file into executable memory and sets PC to its entry point.
*/
long *load(char *fn) {
	FILE *fp;
	unsigned char *ret = NULL;
	long i=0, j=0;

	fp = fopen(fn, "rt");
	fseek(fp, 0, SEEK_END);
	i = ftell(fp);
	MEMlength = i;
	rewind(fp);
	//ret = (unsigned char *)malloc(sizeof(char) * (i));
	ret = mmap(0, i, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
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
		printf("\n");
	}

	if(MEM) {
		stackPush(loadStack, (long *) i);		//push length of JIT memory
		stackPush(loadStack, (long *) ret);		//push JIT loaded address
		return ret;
	}

	return ret;
}

/*
	Program execution starts here. Program is loaded. Stacks are set up. Registers are prepared.
	Eesk is given control.
*/
void main(int argc, char **argv) {
	prepMachine();
	MEM = load(argv[1]);
	execute(MEM);
	kernel(HALT);
}

void execute(long *P) {
	//Prepare registers and go
	//printf("MEM = %p\nPC = %p\n", MEM, PC);
	//printf("ds = %p\nas = %p\tcs = %p\n", dataStack, activationStack, counterStack);
	asm volatile (
			"movq %5, %%rsp\n\t"	//start using the data stack
			"movq %%rsp, %%rbp\n\t"	//save root stack position in rbp
			"pushq %0\n\t"		//place address space on stack
			"movq %1, %%r12\n\t"		//remember kernel location
			"movq %2, %%r14\n\t"		//remember activation stack
			"movq %3, %%r15\n\t"		//remember counter stack
			"movq %6, %%r11\n\t"		//remember type stack
			"movq %7, %%r10\n\t"		//remember secondary type stack
			"callq *%4\n\t"		//pass control to user's program
			:
			:"m" (P), "r" (kernel), "r" (activationStack), "r" (counterStack), "r" (PC), "r" (dataStack), "r" (typeStack), "r" (otypeStack)
			);
}

void prepMachine() {
	//prepare JIT compiler
	Context coDummy;
	dictionary = prepareTranslation(&coDummy);
	jitInit(dictionary);

	//prepare stacks
	loadStack = stackCreate(1000);
	//if(argc>2) verboseFlag = 1;
	libStack = stackCreate(100);
	oldDStack = mmap(NULL, 1000*sizeof(long), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		dataStack = oldDStack+999;
	oldAStack = mmap(NULL, 1000*sizeof(long), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		activationStack = oldAStack+999;
	oldCStack = mmap(NULL, 1000*sizeof(long), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		counterStack = oldCStack+999;
	oldTStack = mmap(NULL, 1000*sizeof(long), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		typeStack = oldTStack+999;
	oldOTStack = mmap(NULL, 1000*sizeof(long), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		otypeStack = oldOTStack+999;

	*counterStack = activationStack;
	counterStack -= 1;
	*counterStack = activationStack;
}

/*
	Provides HALT eeskIR functionality.
	
	TODO:
		- fix that memory leak caused by not being able to munmap(oldCStack)
*/
void quit(long *rsp, long *rbp, long *r11) {
	void *jitload;
	char swapped = (rbp-rsp >= 1000);

	rbp -= 3;
	//if(verboseFlag) printf("rsp = %lx\nrbp = %lx\n", rsp, rbp);
	printf("\n");
	
		//printf("Types: %p to %p\n", oldTStack+999, r11);
	long *typeTop = r11+(rbp-rsp);
	//printf("r11 = %lx, top = %lx, old = %lx\n", r11, typeTop, oldTStack+998);
	if(!swapped) {
		printf("{\n");
		for(;rbp >= rsp; rbp--) {
			//printf("\t%ld\t:\t%lx\t%p\n", *rbp, *typeTop, typeTop);
			printf("\t%ld\n", *rbp);
			typeTop--;
		}
		//printf("|_________________|\n");
		printf("}\n\n");
	} else {
		printf("Stacks are swapped.\n");
	}
	
	//Unload libStack
	for(int i=0;i<libStack->sp;i++) {
		dlclose(libStack->array[i]);
	}
	
	while ((jitload = stackPop(loadStack)) != -1) {
		munmap(jitload, stackPop(loadStack));
	}
	stackFree(loadStack);

	munmap(oldDStack, 1000*sizeof(long));
	munmap(oldAStack, 1000*sizeof(long));
	munmap(oldTStack, 1000*sizeof(long));
	munmap(oldOTStack, 1000*sizeof(long));
	//munmap(oldCStack, 1000*sizeof(long));
	munmap(MEM, MEMlength);
	
	exit(0);
}

/*
	newCollection provides eeskIR NEW functionality.
*/
void newCollection(long **rsp) {
	long *old = (*rsp)-2;	//two words behind the calling address is the length of the symbol
	long len = (*old) + WRDSZ;
	*rsp = mmap(0, len, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	mprotect(rsp, len, PROT_READ|PROT_WRITE|PROT_EXEC);
	memcpy(*rsp, old, len);
	*rsp += 2;	//accommodate for length and stack request words
}

/*
	loadLib provides eeskIR LOAD functionality.
*/
void loadLib(char **rsp) {
	//printf("Loading library:\trsp = %lx, *rsp = %lx : %s\n", rsp, *rsp, *rsp);
	*rsp = dlopen(*rsp, RTLD_LAZY);

	char *error = dlerror();
	stackPush(libStack, *rsp);

	if(error) {
		printf("Error opening shared library %s\n", error);
	}
}

/*
	create turns everything on top of the activationStack into a new set. This involves invoking
	a just in time compiler.
*/
void create(long **rsp, long **aStack) {
	*rsp = jitSet( ((long)*rsp)/WRDSZ, aStack);
	*rsp += 2;
}

/*
	nativeCall turns a Native structure (as explained and produced in compiler.c) and a handle
	to a LOADed shared object into an actual call to the intended native function. If it has
	never been called using the provided Native structure before, it completes the structures'
	dataset by providing it with the destination address, for faster future references.
*/
long nativeCall(long *call, void *handle, long *aStack) {
	void (*function)();
	int i;
	char *error = NULL;
	void *result = 0;
	char returnChar = (char) *(call+1);
	int argc = *(call+2);
	char *functionName = (char *) (call+3+argc);
	long **argv = malloc(argc*WRDSZ);
	ffi_cif cif;

	//if(verboseFlag) printf("Argc = %d\n", argc);
	
	//pop parameter values from stack
	for(i=0;i<argc;i++) {
		//argv[i] = &STACK->array[STACK->sp - (argc-i)];
		argv[i] = aStack + (argc-1) - i;
		//if(verboseFlag) printf("... grabbing argument:\t%d\t%p: %lx\n", argc-i-1, argv[argc-i-1], *argv[argc-i-1]);
	}


	//make call
	if(!*call) {
		//if(verboseFlag) printf("Resolving native function %s\n", functionName);
		function = dlsym(handle, functionName);
		error = dlerror();
		if(error) printf("%s\n", error);
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
					printf("Unknown argument type:\t%p: %c\n", (call+3+i), (char) *(call+3+i));
					exit(1);
					break;
			}
			//if(verboseFlag) printf("Argument type pointer:\t%p:%p\n", call+3+i, *(call+3+i));
		}
	} else {
		//if(verboseFlag) printf("Calling native function %s\n", functionName);
		function = *(void **)call;
	}
	error = dlerror();
	if(error) printf("%s\n", dlerror());

	if(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argc, *(ffi_type**)(call+1), (ffi_type**)(call+3)) == FFI_OK) {
			if(returnChar == 'v') {
				ffi_call(&cif, function, NULL, (void **)argv);
			} else {
				ffi_call(&cif, function, &result, (void **)argv);
			}
	} else {
		printf("Failure in CIF preparation.\n");
		exit(1);
		return -1;
	}

	//Clean up
	free(argv);

	//Return
	//if(verboseFlag) printf("Returning with:\t%p: %lx\n", &result, result);
	return (long) result;
}
