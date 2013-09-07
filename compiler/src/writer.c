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

void writeObj(FILE *fn, long val, int *LC) {
	//writes a word into the output file
	if (fn) {
		fwrite(&val, sizeof(long), 1, fn);
		//printf("%x:%lx\n", *LC, val);
		//if(val < 0) printf("\tValue to write: -%lx (relatively %lx)\n", -val, *LC + val - 1);
	}
	//(*LC) += WRDSZ;
	++(*LC);
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
		for(i=0;i<padding;i++) {
			fwrite(&pad, sizeof(char), 1, fn);
		}
	}

	//increase location counter
	(*LC) += words;
}
