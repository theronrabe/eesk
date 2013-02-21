/*
Theron Rabe
symbolTable.h
2/11/2013

	The data structure used by the compiler to keep track of user-defined symbols and their associated addresses.
*/
#include <string.h>

typedef struct Table {
	char *token;
	int val;
	int publicFlag;
	struct Table *left;
	struct Table *right;
	struct Table *parent;
	struct Table *child;
} Table;

Table *tableCreate() {
	Table *ret = malloc(sizeof(Table));
	return ret;
}

void tableAddSymbol(Table *T, char *token, int address) {
	int cmp;

	if(!T->token) {
		cmp = 1;
	} else {
		cmp = strcmp(token, T->token);
	}
	
	if(!cmp) {
		T->val = address;
	} else if(cmp < 0) {
		if(T->left) {
			tableAddSymbol(T->left, token, address);
		} else {
			T->left = malloc(sizeof(Table));
			T->left->token = malloc(sizeof(char)*32);
			strcpy(T->left->token, token);
			T->left->val = address;
			T->left->parent = T->parent;
			printf("Inserting symbol %s as %d\n", token, address);
		}
	} else if(cmp > 0) {
		if(T->right) {
			tableAddSymbol(T->right, token, address);
		} else {
			T->right = malloc(sizeof(Table));
			T->right->token = malloc(sizeof(char)*32);
			strcpy(T->right->token, token);
			T->right->val = address;
			T->right->parent = T->parent;
			printf("Inserting symbol %s as %d\n", token, address);
		}
	}
}

Table *tableAddLayer(Table *T) {
	//Probably gonna have to change the way this works, seeing as adding another layer
	//doesn't always mean adding another symbol
	Table *ret = malloc(sizeof(Table));
	ret->parent = T;
	T->child = ret;
	ret->token = malloc(sizeof(char)*32);
	
	//strcpy(ret->token, token);
	//ret->address = address;

	return ret;
}

Table *tableRemoveLayer(Table *T) {
	if(T->left) tableRemoveLayer(T->left);
	if(T->right) tableRemoveLayer(T->right);
	
	Table *ret = T->parent;

	free(T->token);
	free(T);

	return ret;
}

int tableLookup(Table *T, char *token) {
	if(!T) {
		return -1;
	}

	int cmp;
	if(T->token) {
		cmp = strcmp(token, T->token);
	} else {
		cmp = 1;
	}

	if(!cmp) {
		return T->val;
	} else if(cmp < 0) {
		if(T->left) {
			return tableLookup(T->left, token);
		} else {
			return tableLookup(T->parent, token);
		}
	} else if(cmp > 0) {
		if(T->right) {
			return tableLookup(T->right, token);
		} else {
			return tableLookup(T->parent, token);
		}
	}
}
