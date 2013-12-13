/*
stack.c

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

#include <stack.h>
#include <stdlib.h>

Stack *stackCreate(int size) {
	Stack *ret = malloc(sizeof(Stack));
	ret->array = malloc(sizeof(long) * size);
	ret->sp = 0;
	ret->array[0] = 0;
	return ret;
}

void stackFree(Stack *st) {
	free(st->array);
	free(st);
}

void stackPush(Stack *st, long val) {
	st->array[st->sp++] = val;
}

long stackPop(Stack *st) {
	if(st->sp) {
		return st->array[--st->sp];
	} else {
		return -1;
	}
}
*/
