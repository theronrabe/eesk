/*
writer.c

	This contains all the functions that write logic to the output file.

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
#include <writer.h>
#include <string.h>
#include <assembler.h>
#include <eeskIR.h>

void writeObj(Compiler *C, long instr, long param) {
	//This function turns an eeskIR instruction and argument, translates it, and writes its code.
	int len = 0;
	unsigned char *out = translationFormCode(C->dictionary, instr, param, &len);

	//if we have a file, write to it
	if (C->dst) {
		if(instr == RPUSH) {
			int rip = C->LC + C->dictionary[RPUSH].param + (C->dictionary[RPUSH].dWord?WRDSZ/2:WRDSZ);	//because RIP contains the next instruction, but LC contains the current instruction
			printf("\n%lx:\t%lx, %lx\t[%lx]\n", C->LC, instr, param, param+rip);
		} else {
			printf("\n%lx:\t%lx, %lx\n", C->LC, instr, param);
		}
		fwrite(out, 1, len, C->dst);

		/*
		printf("%x:", C->LC);
		for(int i=0; i<len; i++) {
			printf("%02x ", out[i]);
		}
		printf("\n");
		*/
	}

	C->LC += len;
}

void writeStr(FILE *fn, char *str, int *LC) {
	//write a string of characters to the output file, padding null characters to align words
	int len = strlen(str) + 1;	//+1 to account for \0
	int words = (len%WRDSZ)?len/WRDSZ+1:len/WRDSZ;
	int padding = (len%WRDSZ)?WRDSZ - len%WRDSZ:0;
	char pad = '\0';
	int i;

	if(fn) {
		//write the string
		fwrite(str, sizeof(char), len, fn);
		//write padding, if needed
		//for(i=0;i<padding;i++) {
			//fwrite(&pad, sizeof(char), 1, fn);
		//}
	}

	//increase location counter
	(*LC) += len;
}
