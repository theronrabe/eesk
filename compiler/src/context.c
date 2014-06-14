/*
context.c

	This file contains functions that do things with/to a Context struct.

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
#include <context.h>
#include <stdlib.h>
#include <symbolTable.h>

Context *contextNew(Table *symbols, char literalFlag, char anonFlag, char typingFlag, char displayFlag, char verboseFlag, char swapFlag) {
	Context *ret = (Context *) malloc(sizeof(Context));
	ret->publicFlag = 0;
	ret->literalFlag = literalFlag;
	ret->nativeFlag = 0;
	ret->staticFlag = 0;
	ret->parameterFlag = 0;
	ret->instructionFlag = 1;
	ret->anonFlag = anonFlag;
	ret->verboseFlag = verboseFlag;
	ret->swapDepth = 0;
	ret->swapFlag = swapFlag;
	ret->displaySymbols = displayFlag;
	if (symbols == NULL) {
		ret->symbols = tableCreate();
		ret->symbols = tableAddLayer(ret->symbols, 1);
	} else {
		ret->symbols = symbols;
	}
	ret->typingFlag = typingFlag;
	return ret;
}

void contextDestroy(Context *CO) {
	//tableDestroy(CO->symbols);
	free(CO);
}
