/*
stack.h

	An array-based integer stack implementation.

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
#ifndef _stack.h_
#define _stack.h_
#include <stdlib.h>

typedef struct Stack {
	long *array;
	int sp;
} Stack;

Stack *stackCreate(int size);
void stackFree(Stack *st);
void stackPush(Stack *st, long val);
long stackPop(Stack *st);

#endif
