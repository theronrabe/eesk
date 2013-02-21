/*
Theron Rabe
callList.h
2/10/2013

	A data structure to manage call lists for the virtual machine's varying state.
*/

typedef struct CallList {
	int address;
	struct CallList *next;
} CallList;

void callFree(CallList *cl) {
	if(!cl->next) {
		free(cl);
		return;
	} else {
		callFree(cl->next);
		free(cl);
		return;
	}
}

CallList *callAdd(CallList *cl, int address) {
	CallList *ret = malloc(sizeof(CallList));
	
	ret->address = address;
	ret->next = cl;

	return ret;
}
