/*
Theron Rabe.
stack.h
2/10/2013

	An array-based integer stack implementation.
*/
#include <stdlib.h>

typedef struct Stack {
	int *array;
	int sp;
} Stack;

Stack *stackCreate(int size) {
	Stack *ret = malloc(sizeof(Stack));
	ret->array = malloc(sizeof(int) * size);
	ret->sp = 0;
	return ret;
}

Stack *stackFree(Stack *st) {
	free(st->array);
	free(st);
}

void stackPush(Stack *st, int val) {
	st->array[st->sp++] = val;
}

int stackPop(Stack *st) {
	if(st->sp) {
		return st->array[--st->sp];
	} else {
		return -1;
	}
}
