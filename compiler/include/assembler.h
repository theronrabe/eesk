/*
assembler.h

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
#ifndef _assembler.h_
#define _assembler.h_

typedef struct codeLib {
	long eeskVal;
	char *code;
	int param;
	
	codeLib *right;
	codeLib *left;
} codeLib;

codeLib *codeLibCreate();
codeLib *codeLibAdd(codeLib *m, long eeskVal, char *code, int param);
codeLib *codeLibLookup(codeLib *m, long eeskVal);
void codeLibFree(codeLib *m);

char *formCode(codeLib *m, long eeskVal, long arg);
#endif
