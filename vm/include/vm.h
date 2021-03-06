/*
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
#include <stack.h>
#define WRDSZ 8

typedef struct LoadedCode {
	void **location;
	long size;
} LoadedCode;


long *load(char *fn);
//void execute(long *MEM, Stack *STACK, long *address);
void execute(long *P);
void quit(long *rsp, long *rbp, long *r11);
void newCollection(long **rsp);
void loadLib(char **rsp);
void create(long **rsp, long **aStack);
long nativeCall(long *call, void *handle, long *aStack);
long loc(long start, long offset);
long dloc(long start, long address);
void prepMachine();
