/*
symbolTable.c

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
#include <symbolTable.h>
#include <compiler.h>
#include <stdlib.h>
#include <stdio.h>

char layerChar = '0';

Table *tableCreate() {
	Table *ret = malloc(sizeof(Table));
	ret->token = malloc(sizeof(char) * TOKSIZE);
	strcpy(ret->token, "Eesk");
	ret->parent = NULL;
	ret->right = NULL;
	ret->left = NULL;
	ret->layerRoot = ret;
	ret->val = 0;
	ret->staticFlag = 1;
	ret->searchUp = 0;
	ret->parameterFlag = 0;
	ret->offset = 0;
	ret->backset = 0;
	return ret;
}

void publicize(Table *node) {
	long offset, backset;
	Context CO;
		CO.staticFlag = 0;
		CO.parameterFlag = 0;
		CO.expectedLength = 0;
	if(node) {
		//printf("%s staticity: %d\n", node->token, node->staticFlag);
		//printf("publicizing node %s\n", node->token);
		char publicToken[TOKSIZE];
		if(node->parent) {
			strcpy(publicToken, node->parent->token);
			strcat(publicToken, ".");
			strcat(publicToken, node->token);
			if(!node->parent->searchUp) {
				//This symbol was relatively addressed, remember an offset
				offset = node->parent->val;
				backset = node->backset;
			}
			node = tableAddSymbol(node->parent, publicToken, node->val + node->offset, &CO);
			node->offset = offset;
			node->backset = backset;
			publicize(node);
		}
	}
}

Table *tableAddSymbol(Table *T, char *token, int address, Context *CO) {
	int cmp;

	if(!T->token) {
		cmp = 1;
	} else {
		//printf("into node %s, with token %s\n", T->token, token);
		cmp = strcmp(token, T->token);
	}
	
	if(!cmp) {
		T->val = address;
		return T;
	} else if(cmp < 0) {
		if(T->left) {
			return tableAddSymbol(T->left, token, address, CO);
		} else {
			T->left = malloc(sizeof(Table));
			T->left->token = malloc(sizeof(char)*TOKSIZE);
			strcpy(T->left->token, token);
			T->left->val = address;
			T->left->parent = T->parent;
			T->left->left = NULL;
			T->left->right = NULL;
			T->left->layerRoot = T->layerRoot;
			T->left->staticFlag = CO->staticFlag;
			T->left->searchUp = T->searchUp;
			T->left->parameterFlag = CO->parameterFlag;
			T->left->offset = 0;
			T->left->backset = CO->expectedLength - address;
			//printf("Inserting %s as %x left of %s on layer %s. Is static? %d\n", token, address, T->token, T->layerRoot->token, T->left->staticFlag);
			return T->left;
		}
	} else if(cmp > 0) {
		if(T->right) {
			return tableAddSymbol(T->right, token, address, CO);
		} else {
			T->right = malloc(sizeof(Table));
			T->right->token = malloc(sizeof(char)*TOKSIZE);
			strcpy(T->right->token, token);
			T->right->val = address;
			T->right->parent = T->parent;
			T->right->left = NULL;
			T->right->right = NULL;
			T->right->layerRoot = T->layerRoot;
			T->right->staticFlag = CO->staticFlag;
			T->right->searchUp = T->searchUp;
			T->right->parameterFlag = CO->parameterFlag;
			T->right->offset = 0;
			T->right->backset = CO->expectedLength - address;
			//printf("Inserting %s as %x right of %s on layer %s. Is static? %d\n", token, address, T->token, T->layerRoot->token, T->right->staticFlag);
			return T->right;
		}
	}
	return NULL; //this should be unreachable
}

Table *tableAddLayer(Table *T, char *token, char isObject) {
	//Probably gonna have to change the way this works, seeing as adding another layer
	//doesn't always mean adding another symbol
	Table *ret = malloc(sizeof(Table));
	ret->parent = T;

	ret->token = malloc(sizeof(char)*TOKSIZE);
	ret->right = NULL;
	ret->left = NULL;
	ret->layerRoot = ret;
	ret->searchUp = !isObject;
	ret->staticFlag = 0;
	ret->parameterFlag = 0;
	ret->val = 0;

	//strcpy(ret->token, token);
	strcpy(ret->token, "this");
	//ret->address = address;
	//printf("\nNew Layer %s under %s\n", token, T->token);

	return ret;
}

Table *tableRemoveLayer(Table *T) {
	Table *ret = T->parent;
	Table *right = T->right;
	Table *left = T->left;

	//printf("Remove %s from layer %s\n", T->token, T->layerRoot->token);
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

Table *tableLookup(Table *T, char *token, int *accOff) {
	if(!T) {
		//printf("\tdidn't find %s.\n", token);
		return NULL;
	}
	//printf("\tIn layer %s, node %s with %s\n", T->layerRoot->token, T->token, token);

	int cmp;
	if(T->token) {
		cmp = strcmp(token, T->token);
	} else {
		cmp = 1;
	}

	if(!cmp) {
		//printf("\tfound %s in layer %s.\n", token, T->layerRoot->token);
		return T;
	} else if(cmp < 0) {
		if(T->left) {
			//printf("\t\tsearching left of %s for %s\n", T->token, token);
			return tableLookup(T->left, token, accOff);
		} else {
			//printf("\t\tNO LEFT... searching parent layer of %s for %s\n", T->token, token);
			if(T->parent) {
				if(!T->searchUp) {
					//build accumulated offset
					if(*accOff < 0) *accOff = 0;
					*accOff += T->parent->val;
				}
				return tableLookup(T->parent->layerRoot, token, accOff);
			} else {
				//printf("\tdidn't find %s.\n", token);
				return NULL;
			}
		}
	} else if(cmp > 0) {
		if(T->right) {
			//printf("\t\tsearching right of %s for %s\n", T->token, token);
			return tableLookup(T->right, token, accOff);
		} else {
			//printf("\t\tNO RIGHT... searching parent layer of %s for %s\n", T->token, token);
			if(T->parent) {
				if(!T->searchUp) {
					//build accumulated offset
					if(*accOff < 0) *accOff = 0;
					*accOff += T->parent->val;
				}
				return tableLookup(T->parent->layerRoot, token, accOff);
			} else {
				//printf("\tdidn't find %s.\n", token);
				return NULL;
			}
		}
	}
	return NULL;	//this should be unreachable
}
