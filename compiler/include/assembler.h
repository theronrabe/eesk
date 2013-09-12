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

typedef struct translation {
	int param;
	char dWord;
	int length;
	unsigned char *code;
} translation;

translation *translationCreate();
void translationAdd(translation *m, int eeskVal, unsigned char *code, int param, char dWord);
void translationExtend(translation *m);
void translationFree(translation *m);

unsigned char *translationFormCode(translation *m, int eeskVal, long arg, int *C);
#endif
