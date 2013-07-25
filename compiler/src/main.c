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
	Context context;
	int SC = 0, LC = 0, lineCount = 1;
	
	callStack = stackCreate(32);
	nameStack = stackCreate(32);

	trimComments(src);

	context.publicFlag = 0;
	context.literalFlag = 0;
	context.nativeFlag = 0;
	context.staticFlag = 0;
	context.parameterFlag = 0;
	context.instructionFlag = 0;
	
	compileStatement(keyWords, symbols, src, &SC, dst, &LC, &context, &lineCount);
	writeObj(dst, transferAddress, &LC);

	free(src);
	fclose(dst);
	return 0;
}
