/*
symbolTable.h

	The data structure used by the compiler to keep track of user-defined symbols and their associated addresses.

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
#include <string.h>

char layerChar = '0';

typedef struct Table {
	char *token;
	int val;
	int publicFlag;
	struct Table *left;
	struct Table *right;
	struct Table *parent;
	struct Table *layerRoot;
} Table;

Table *tableCreate();
//void publicize(Table *node);
Table *tableAddSymbol(Table *T, char *token, int val);


Table *tableCreate() {
	Table *ret = malloc(sizeof(Table));
	ret->token = malloc(sizeof(char) * 32);
	strcpy(ret->token, "Eesk");
	ret->parent = NULL;
	ret->right = NULL;
	ret->left = NULL;
	ret->layerRoot = ret;
	ret->val = 0;
	return ret;
}

void publicize(Table *node) {
	char publicToken[128];
	if(node->parent) {
		strcpy(publicToken, node->parent->token);
		strcat(publicToken, ".");
		strcat(publicToken, node->token);
		node = tableAddSymbol(node->parent, publicToken, node->val);
		publicize(node);
	}
}

Table *tableAddSymbol(Table *T, char *token, int address) {
	int cmp;

	if(!T->token) {
		cmp = 1;
	} else {
		//printf("into node %s, with token %s\n", T->token, token);
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
			T->left->left = NULL;
			T->left->right = NULL;
			T->left->layerRoot = T->layerRoot;
			//printf("Inserting symbol %s as %d on left\n", token, address);
			return T->left;
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
			T->right->left = NULL;
			T->right->right = NULL;
			T->right->layerRoot = T->layerRoot;
			//printf("Inserting symbol %s as %d on right\n", token, address);
			return T->right;
		}
	}
}

Table *tableAddLayer(Table *T, char *token) {
	//Probably gonna have to change the way this works, seeing as adding another layer
	//doesn't always mean adding another symbol
	Table *ret = malloc(sizeof(Table));
	ret->parent = T;

	ret->token = malloc(sizeof(char)*32);
	ret->right = NULL;
	ret->left = NULL;
	ret->layerRoot = ret;

	strcpy(ret->token, token);
	//ret->address = address;

	return ret;
}

Table *tableRemoveLayer(Table *T) {
	Table *ret = T->parent;
	Table *right = T->right;
	Table *left = T->left;

	if(left) {
		tableRemoveLayer(left);
	}

	free(T->token);
	free(T);

	if(right) {
		tableRemoveLayer(right);
	}

	return ret->layerRoot;
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
