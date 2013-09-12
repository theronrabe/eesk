/*
assembler.c
	
	This file contains a set of functions that can be used to substitute the eesk virtual machine's instructions
	into native machine code.

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

#include <assembler.h>
#include <eeskIR.h>
#include <stdlib.h>
#include <stdio.h>

translation *translationCreate() {
	translation *ret = malloc(sizeof(translation) * TRANSLATION_SIZE);
	return ret;
}

void translationAdd(translation *m, int eeskVal, unsigned char *code, int param, char dWord) {
	m[eeskVal].code = code;
	m[eeskVal].param = param;
	m[eeskVal].dWord = dWord;
	
	m[eeskVal].length = 0;
	for(int i=0; !(code[i] == 0xC3 && code[i+1] == 0xC3); i++)	//0xC3 is the retq instruction. Two consecutive 0xC3 bytes mark the end of a code translation.
		m[eeskVal].length++;
}

void translationExtend(translation *m) {
	for(int i=0; i<TRANSLATION_SIZE; i++) {
		if(!m[i].code) {
			unsigned char c_nop[] = {0x90, 0xC3, 0xC3};
			translationAdd(m, i, c_nop, -1, 0);
		}
	}
}

void translationFree(translation *m) {
	free(m);
}

unsigned char *translationFormCode(translation *m, int eeskVal, long arg, int *C) {
	translation *a = &(m[eeskVal]);
	if(a->param >= 0) {
		if(a->dWord) {
			int *param = (int *) a->code[a->param];
			param = (int) arg;
		} else {
			long *param = (long *) (a->code[a->param]);
			param = arg;
		}
	}
	(*C) += a->length;
	return a->code;
}
