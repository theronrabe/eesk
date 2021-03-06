/*
main.c

	This file compiles a file containing Eesk code for the Eesk Virtual Machine.

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
#include <eeskIR.h>
#include <writer.h>
#include <options.h>

int main(int argc, char **argv) {
	Context CO;
	Compiler C;
	char tok[256];
	C.src = NULL;

	contextSetOptions(argc, argv, &CO, &C);

	C.dictionary = prepareTranslation(&CO);
	C.dst = (C.src)? fopen(C.src, "w") : fopen("e.out", "w");	//contextSetOptions may have indicated output file by setting C.src
	C.src = loadFile(argv[1]);
	C.SC = 0;
	C.LC = 0;
	C.lineCounter = 0;
	C.keyWords = prepareKeywords();
	C.end = 0;
	C.anonStack = stackCreate(32);
	
	callStack = stackCreate(32);
	nameStack = stackCreate(32);

	trimComments(C.src);

	CO.publicFlag = 0;
	CO.literalFlag = 0;
	CO.nativeFlag = 0;
	CO.staticFlag = 0;
	CO.parameterFlag = 0;
	CO.instructionFlag = 1;
	CO.anonFlag = 0;
	CO.swapDepth = 0;
	CO.symbols = tableCreate();
		CO.symbols = tableAddLayer(CO.symbols, "this", 1);	//Ensures we have a "this" reference at all times
	
	if(!CO.typingFlag) writeObj(&C, TPUSH, 1);
	compileStatement(&C, &CO, tok);
	writeObj(&C, HALT, 0);
	writeObj(&C, DATA, transferAddress);

	translationFree(C.dictionary);
	tableDestroy(CO.symbols);
	free(C.src);
	fclose(C.dst);
	return 0;
}
