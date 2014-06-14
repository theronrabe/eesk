/*
jit.c
	
	This provides the Eesk runtime environment access to a compiler. This seems to be the only good
	way for a CREATE instruction to produce executable Sets.

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
#include <jit.h>
#include <vm.h>
#include <assembler.h>
#include <compiler.h>
#include <context.h>
#include <eeskIR.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <writer.h>
#include <symbolTable.h>

translation *dictionary;
Compiler *C;
Context *CO;
Table *symbols;
long LC = 0;
extern Stack *loadStack;

long *jitSet(int count, long *values) {
	int i;
	long *SC;
	long totalBytes = 2*WRDSZ + dictionary[GRAB].length*count + dictionary[CONT].length*count + dictionary[RSR].length;
	long *ret = mmap(0, totalBytes, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	SC = ret;

	writeJit((long *) &SC, DATA, totalBytes);
	writeJit((long *) &SC, DATA, WRDSZ);
	for(i=0; i<count; i++) {
		writeJit((long *)&SC, GRAB, *(values+i));
		writeJit((long *)&SC, CONT, 0);
	}
	writeJit((long *)&SC, RSR, 0);
	return ret;
}

void writeJit(long *SC, int eeskIR, long arg) {
	int C = 0;
	void *code = translationFormCode(dictionary, eeskIR, arg, &C);
	memcpy((void *) *SC, code, C);
	(*SC) += C;
}

void jitInit(translation *eesk) {
	dictionary = eesk;
	symbols = tableCreate();
	symbols = tableAddLayer(symbols, 1);
}

void jitEnd() {
	tableDestroy(symbols);
}

void *jitCompile(char *src) {
	char buffer[100];
	C = compilerCreate(src, LC);
	CO = contextNew(symbols, 0, 0, 0, 0, 0, 1);
	writeObj(C, DATA, 0);
	writeObj(C, DATA, 0);
	compileStatement(C, CO, buffer);
	writeObj(C, RSR, 0);
	contextDestroy(CO);
	compilerDestroy(C);

	long *ret = load("j.out");
	LC += loadStack->array[loadStack->sp-2] - 0x1000;
	printf("loaded: %ld @ %p, %ld\n", loadStack->array[loadStack->sp-2], ret, ret - loadStack->array[1]);
	return ret;
}
