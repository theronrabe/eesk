/*
main.c

	This file compiles another file that was written in Eesk for the Eesk Virtual Machine.

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

#include <compiler.h>

int main(int argc, char **argv) {
	char *src = loadFile(argv[1]);
	FILE *dst = fopen("e.out", "w");
	Table *keyWords = prepareKeywords();
	Table *symbols = tableCreate();
	int SC = 0, LC = 0;
	
	callStack = stackCreate(32);
	nameStack = stackCreate(32);

	trimComments(src);
	
	compileStatement(keyWords, symbols, src, &SC, dst, &LC);
	writeObj(dst, transferAddress, &LC);

	fclose(dst);
	free(src);
	return 0;
}
