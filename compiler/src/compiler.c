/*
compiler.c

	This is the compiler. It basically works as a large finite state machine, where each iteration of the compileStatement
	function corresponds to a new token-decided state. Any state has the potential to write pneumonic machine code to the
	output file using the writeObj function, or oftentimes recurse into compiling a substatement, then advancing to the
	next state.


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
#include <compiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <fileIO.h>
#include <tokenizer.h>
#include <symbolTable.h>

int compileStatement(Table *keyWords, Table *symbols, char *src, int *SC, FILE *dst, int *LC, char publicFlag, char literalFlag, char nativeFlag, char staticFlag) {
	int endOfStatement = 0;		//tells us whether or not to unstack operators
	int oldLC = *LC;		//for measuring output progress
	int fakeLC = 0;			//for measuring relative addresses sub-statement
	int fakeSC;			//for measuring relative addresses on sub-statement
	char tok[256];			//needs to be big in case of string
	long nameAddr;			//for marking important working addresses
	long DC[3];			//data counters
	int i;				//iterator
	double tempFloat;		//for working floating point values
	long tokVal;			//value of current token
	int tokLen;			//length of current token
	Table *tempTable;		//for working symbol tables
	Stack *operationStack = stackCreate(32);	//for stacking operators

	while(!endOfStatement && tokLen != -1) {
		tokLen = getToken(tok, src, SC);
//printf("token:\t%s\n", tok);
		tempTable = tableLookup(keyWords, tok);
		if(tempTable) {
			tokVal = tempTable->val;
		} else {
			tokVal = -1;
		}

		switch(tokVal) {
			case(k_if):
				//get beginning address
				nameAddr = *LC;

				//get length of statement
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);	//condition
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);	//clause
				DC[2] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);	//else

				writeObj(dst, RPUSH, LC);	writeObj(dst, DC[0]+DC[1]+8, LC);	//else address
				writeObj(dst, DLOC, LC);
				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);				//compiled condition
				writeObj(dst, BNE, LC);								//decide

				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);				//compiled statement
				writeObj(dst, RPUSH, LC);	writeObj(dst, DC[2]+4, LC);	//push end address
				writeObj(dst, DLOC, LC);
				writeObj(dst, JMP, LC);		//jump to end

				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);		//compiled else
				break;


			case(k_while):
				printf("compiling while statement:\n");
				//get beginning address
				nameAddr = *LC;

				//get section lengths
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);	//condition length
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);	//loop length

				writeObj(dst, RPUSH, LC);	writeObj(dst, DC[0]+DC[1]+8, LC);	//end address
				printf("return address = (from %lx) %lx\n", DC[0]+DC[1]+8, DC[0]+DC[1]+8 + *LC - 2);
				writeObj(dst, DLOC, LC);
				printf("compiling condition:\n");
				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);				//compiled condtion
				writeObj(dst, BNE, LC);								//decide
				printf("compiling loop:\n");
				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);				//compiled loop
				printf("iterating:\n");
				writeObj(dst, RPUSH, LC);	writeObj(dst, -(DC[0]+DC[1]+4), LC);			//begin address
				writeObj(dst, DLOC, LC);
				writeObj(dst, JMP, LC);								//iterate
				printf("ending while statement.\n");
				break;


			case(k_Function):
				writeObj(dst, 0, LC);						//result location
	
				//in a new namespace
				nameAddr = *LC;
				stackPush(nameStack, *LC);
				tokLen = getToken(tok, src, SC);
				symbols = tableAddSymbol(symbols, tok, nameAddr, staticFlag);	//change to this scope
				if(publicFlag) { publicize(symbols); publicFlag = 0; }
				symbols = tableAddLayer(symbols, tok);
	
				//count length of parameters and data sections
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);	//param length
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);	//data length
				
				writeObj(dst, DC[0]+DC[1]+1, LC);		//name pointer to statement. Add value to nameAddr when calling; it's relative
				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);		//compiled parameters section
				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);		//compiled data section
	
				//fill parameters with arguments
				for(i=DC[0]-1;i>=0;i--){
					writeObj(dst, POPTO, LC);				//POPTO each argument
					writeObj(dst, nameAddr+2+i - *LC, LC);
				}
	
				//writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr-1, LC);	//push result address
				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);		//compiled statement section
				writeObj(dst, POP, LC);						//store result
				writeObj(dst, JMP, LC);						//goto return address
	
				//no longer in this namespace
				stackPop(nameStack);
				symbols = tableRemoveLayer(symbols);
				break;


			case(k_oBrace):
				//This seemingly important symbol literally does nothing.
				break;


			case(k_cBrace):
				fillOperations(dst, LC, operationStack);
				endOfStatement = 1;
				break;


			case(k_oParen):
				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);
				break;


			case(k_cParen):
				fillOperations(dst, LC, operationStack);
				endOfStatement = 1;
				break;


			case(k_oBracket):
				//set aside call address/string for now
				writeObj(dst, PUSH, LC);		writeObj(dst, 2, LC);	//jump over word of call data
				writeObj(dst, HOP, LC);
					nameAddr = *LC;
					writeObj(dst, 0, LC);	//storage for call address
				writeObj(dst, POPTO, LC);	writeObj(dst, nameAddr - *LC + 1, LC);

				if(!nativeFlag) {
					//find length of arguments
					fakeSC = *SC;
					DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);		//compiled argument section

					//push return address of call
					writeObj(dst, PUSH, LC);	writeObj(dst, *LC+DC[0]+23, LC);			//push return address

					//push result address for function
					writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
					writeObj(dst, LOC, LC);						//turn into address
					writeObj(dst, CONT, LC);					//turn pointer into relative address base
					writeObj(dst, PUSH, LC);	writeObj(dst, 8, LC);		//push function pointer
					writeObj(dst, SUB, LC);						//combine base and offset to get result address

					//compile arguments
					compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);

					//make call
					writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
					writeObj(dst, LOC, LC);						//get actual address
					writeObj(dst, CONT, LC);					//turn pointer into relative address base
				
					writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
					writeObj(dst, LOC, LC);						//grab real address
					writeObj(dst, CONT, LC);
					//writeObj(dst, LOC, LC);						//locate new pointer
					writeObj(dst, CONT, LC);					//turn pointer into address offset
					writeObj(dst, PUSH, LC);	writeObj(dst, 8, LC);		//turn index offset into address offset
					writeObj(dst, MUL, LC);							//by multiplying by 8
					writeObj(dst, ADD, LC);						//combine base and offset to get call address
					writeObj(dst, DLOC, LC);					//turn address into MEM index
					writeObj(dst, JMP, LC);						//goto call address

					//push result address 
					writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
					writeObj(dst, LOC, LC);						//locate it
					writeObj(dst, CONT, LC);					//turn pointer into relative address base
					writeObj(dst, PUSH, LC);	writeObj(dst, 8, LC);		//push function pointer
					writeObj(dst, SUB, LC);						//combine base and offset to get result address
					writeObj(dst, CONT, LC);
				} else {
					//compiled argument section
					compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);

					//push call string with format: "foo(isf)@libbar.so" for a function that takes an integer, string, and float as parameters
					writeObj(dst, RPUSH, LC);	writeObj(dst, nameAddr - *LC + 1, LC);	//stored pointer here earlier
					writeObj(dst, CONT, LC);

					//make call
					writeObj(dst, NTV, LC);
				}
				break;


			case(k_cBracket):
				fillOperations(dst, LC, operationStack);
				endOfStatement = 1;
				break;


			case(k_prnt):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, PRNT);
				break;


			case(k_prtf):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, PRTF);
				break;


			case(k_prtc):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, PRTC);
				break;


			case(k_prts):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, PRTS);
				break;


			case(k_goto):
				stackPush(operationStack, JMP);
				break;


			case(k_singleQuote):
				tokLen = getToken(tok, src, SC);
				writeObj(dst, PUSH, LC);	writeObj(dst, tok[0], LC);
				break;


			case(k_doubleQuote):
				DC[0] = getQuote(tok, src, SC);
				if(!literalFlag) {
					writeObj(dst, RPUSH, LC);	writeObj(dst, 6, LC);	//push start of string
					//writeObj(dst, LOC, LC);
					writeObj(dst, RPUSH, LC);	writeObj(dst, 4+DC[0], LC);	//push end of string
					writeObj(dst, DLOC, LC);
					writeObj(dst, JMP, LC);						//skip over string leaving it on stack
				}
				writeStr(dst, tok, LC);
				break;


			case(k_int):
				stackPush(operationStack, FTOD);
				break;


			case(k_char):
				stackPush(operationStack, FTOD);
				break;


			case(k_pnt):
				tokLen = getToken(tok, src, SC);
				if(numeric(tok[0])) {		//is this an array?
					//create a symbol and some memory
					DC[0] = atoi(tok);
					writeObj(dst, *LC, LC);
					for(i=0;i<DC[0];i++)
						writeObj(dst, DC[0], LC);
					tokLen = getToken(tok, src, SC);
					if(dst) {
						tempTable = tableAddSymbol(symbols, tok, *LC-DC[0]-1, staticFlag);
					}
				} else {
					//create a symbol and a word
					tempTable = tableAddSymbol(symbols, tok, *LC, staticFlag);
					if(publicFlag) publicize(tempTable);
					writeObj(dst, 0, LC);
				}
				break;


			case(k_float):
				stackPush(operationStack, DTOF);
				break;


			case(k_begin):
				transferAddress = *LC;
				break;


			case(k_halt):
				writeObj(dst, HALT, LC);
				break;


			case(k_clr):
				writeObj(dst, CLR, LC);
				break;


			case(k_endStatement):
				//unstack operators and reset flags
				fillOperations(dst, LC, operationStack);
				publicFlag = 0;
				nativeFlag = 0;
				staticFlag = 0;
				break;


			case(k_cont):
				writeObj(dst, CONT, LC);
				break;


			case(k_not):
				writeObj(dst, NOT, LC);
				break;


			case(k_is):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, POP);
				break;


			case(k_set):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, BPOP);
				break;


			case(k_eq):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, EQ);
				break;


			case(k_gt):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, GT);
				break;


			case(k_lt):
				fillOperations(dst, LC, operationStack);
				stackPush(operationStack, LT);
				break;


			case(k_add):
				stackPush(operationStack, ADD);
				break;


			case(k_sub):
				stackPush(operationStack, SUB);
				break;


			case(k_mul):
				stackPush(operationStack, MUL);
				break;


			case(k_div):
				stackPush(operationStack, DIV);
				break;


			case(k_mod):
				stackPush(operationStack, MOD);
				break;


			case(k_and):
				stackPush(operationStack, AND);
				break;


			case(k_or):
				stackPush(operationStack, OR);
				break;


			case(k_fadd):
				stackPush(operationStack, FADD);
				break;


			case(k_fsub):
				stackPush(operationStack, FSUB);
				break;


			case(k_fmul):
				stackPush(operationStack, FMUL);
				break;


			case(k_fdiv):
				stackPush(operationStack, FDIV);
				break;


			case(k_return):
				stackPush(operationStack, JMP);
				stackPush(operationStack, POP);
				break;


			case(k_public):
				publicFlag = 1;
				break;


			case(k_private):
				publicFlag = 0;
				break;


			case(k_literal):
				literalFlag = !literalFlag;
				break;


			case(k_collect):
				//set the scope
				tokLen = getToken(tok, src, SC);
				symbols = tableAddSymbol(symbols, tok, *LC, staticFlag);
				symbols = tableAddLayer(symbols, tok);

				//get its own length:
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);
				*LC += fakeLC; //accommodate for change in location

				//write its length, and its body (addressed relative to right here)
				writeObj(dst, DC[0], LC);
				fakeLC = 0;
				compileStatement(keyWords, symbols, src, SC, dst, &fakeLC, publicFlag, literalFlag, nativeFlag, staticFlag);
				
				//clean up
				symbols = tableRemoveLayer(symbols);
				break;


			case(k_field):
				//set the scope
				tokLen = getToken(tok, src, SC);
				symbols = tableAddSymbol(symbols, tok, *LC, 1);
				symbols = tableAddLayer(symbols, tok);

				//compile the body
				compileStatement(keyWords, symbols, src, SC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);

				//clean up
				symbols = tableRemoveLayer(symbols);
				break;



			case(k_alloc):
				stackPush(operationStack, ALOC);
				break;


			case(k_new):
				stackPush(operationStack, NEW);
				break;


			case(k_free):
				stackPush(operationStack, FREE);
				break;


			case(k_include):
				getQuote(tok, src, SC);
				strcpy(tok, "");
				strcat(tok, "include/");
				getQuote(&tok[8], src, SC);			//to accommodate for the beginning "include/"
				strcat(tok, ".ee");
				printf("INCLUDING FILE: %s\n", tok);
				char *inc = loadFile(tok);
				trimComments(inc);
				fakeSC = 0;
				compileStatement(keyWords, symbols, inc, &fakeSC, dst, LC, publicFlag, literalFlag, nativeFlag, staticFlag);
				free(inc);
				//endOfStatement = 1;
				break; 


			case(k_native):
				nativeFlag = 1;
				//stackPush(operationStack, NTV);
				break;



			case(k_label):
				//create a symbol referring to this location
				tokLen = getToken(tok, src, SC);
				tempTable = tableAddSymbol(symbols, tok, *LC, staticFlag);
				if(publicFlag) publicize(tempTable);
				break;


			case(k_static):
				staticFlag = 1;
				break;


			default:
				//this is either an intended symbol or a number

				if(numeric(tok[0]) || (tok[0] == '-' && numeric(tok[1]))) {
					//this is a numeric literal
					if(isFloat(tok)) {
						tempFloat = atof(tok);
						if(!literalFlag) writeObj(dst, PUSH, LC);	//writeObj(dst, tempFloat, LC);
						if(dst) {
							//fwrite(&tempFloat, sizeof(long), 1, dst);
							writeObj(dst, *(long *) &tempFloat, LC);
							//++(*LC);
						}
					} else {
						if(!literalFlag) writeObj(dst, PUSH, LC);
						writeObj(dst, atol(tok), LC);	//push its value
					}
				} else {
					writeAddressCalculation(dst, tok, symbols, LC, publicFlag, literalFlag, nativeFlag, staticFlag);
				}
				break;
		}
	}
	
	stackFree(operationStack);

	return *LC - oldLC;
}

void writeObj(FILE *fn, long val, int *LC) {
	//writes a word into the output file
	if (fn) {
		fwrite(&val, sizeof(long), 1, fn);
		printf("%x:%lx\n", *LC, val);
		if(val < 0) printf("\t(%lx)\n", *LC + val - 1);
	}
	(*LC)++;
}

void writeStr(FILE *fn, char *str, int *LC) {
	//write a string of characters to the output file, padding null characters to align words
	int len = strlen(str);
	int words = (len%8)?len/8+1:len/8;
	int padding = (len%8)?8 - len%8:0;
	char pad = '\0';
	int i;

	if(fn) {
		//write the string
		fwrite(str, sizeof(char), len, fn);

		//write padding, if needed
		for(i=0;i<padding;i++) {
			fwrite(&pad, sizeof(char), 1, fn);
		}
	}

	//increase location counter
	(*LC) += words;
}

int writeAddressCalculation(FILE *dst, char *token, Table *symbols, int *LC, char publicFlag, char literalFlag, char nativeFlag, char staticFlag) {
	//this function figures out what address a non-keyword token should correlate to
	//and writes that address to the output file
	int oldLC = *LC;
	char *piece0 = strtok(token, "@");
	char *piece1 = strtok(NULL, "@");

	Table *sym = tableLookup(symbols, piece0);
	
	if(sym == NULL) {	//does the symbol not exist yet?
		tableAddSymbol(symbols, piece0, *LC, staticFlag);
		sym = tableLookup(symbols, piece0);
		printf("%x: Unknown symbol: %s:%x, %d.\nContinuing, substituting with location counter...\n", *LC, sym->token, sym->val, sym->staticFlag);
		if(publicFlag) publicize(sym);
	} else {
		//already in symbol table
	}
	
	if(piece1) {	//is this a compound symbol?
		writeObj(dst, RPUSH, LC);	writeObj(dst, tableLookup(symbols, piece1)->val - *LC + 1, LC);
		writeObj(dst, CONT, LC);	//resolve pointer
		writeObj(dst, DLOC, LC);
		writeObj(dst, PUSH, LC);	writeObj(dst, sym->val, LC);
		writeObj(dst, ADD, LC);
		writeObj(dst, LOC, LC);
	} else {
		if(!sym->staticFlag) {
			if(!literalFlag) writeObj(dst, RPUSH, LC); else printf("OOPS!\n");
			writeObj(dst, sym->val - *LC + 1, LC);
		} else {
			/* with the whole "rpushing the negative LC" to find true address of dynamic address thing,
			do we really need to keep track of what is/isn't static?

			Also, maybe do something to clean up those literalFlag sections. They aren't right.
			*/
			if(sym->parent) {
				//Add offset to parent address
				if(!literalFlag) writeObj(dst, PUSH, LC); else printf("OOPS2!\n");
				writeObj(dst, 8*sym->val, LC);
				writeObj(dst, RPUSH, LC);	writeObj(dst, -*LC + 1, LC);
				writeObj(dst, ADD, LC);
			} else {
				if(!literalFlag) writeObj(dst, PUSH, LC); else printf("OOPS3!\n");
				writeObj(dst, sym->val, LC);
				writeObj(dst, LOC, LC);
			}
		}
	}
	return *LC - oldLC;
}

void fillOperations(FILE *dst, int *LC, Stack *operationStack) {
	//unstacks operators
	int op;

	while((op = stackPop(operationStack))) {
		if(op == -1) break;
		writeObj(dst, op, LC);
	}
}

Table *prepareKeywords() {
	Table *ret = tableCreate();

	//Language keywords
	tableAddSymbol(ret, "if", k_if, 0);
	tableAddSymbol(ret, "while", k_while, 0);
	tableAddSymbol(ret, "Function", k_Function, 0);
	tableAddSymbol(ret, "[", k_oBracket, 0);
	tableAddSymbol(ret, "]", k_cBracket, 0);
	tableAddSymbol(ret, "{", k_oBrace, 0);
	tableAddSymbol(ret, "}", k_cBrace, 0);
	tableAddSymbol(ret, "(", k_oParen, 0);
	tableAddSymbol(ret, ")", k_cParen, 0);
	tableAddSymbol(ret, "printd", k_prnt, 0);
	tableAddSymbol(ret, "printf", k_prtf, 0);
	tableAddSymbol(ret, "printc", k_prtc, 0);
	tableAddSymbol(ret, "prints", k_prts, 0);
	tableAddSymbol(ret, "goto", k_goto, 0);
	tableAddSymbol(ret, "\'", k_singleQuote, 0);
	tableAddSymbol(ret, "\"", k_doubleQuote, 0);
	tableAddSymbol(ret, "int", k_int, 0);
	tableAddSymbol(ret, "char", k_char, 0);
	tableAddSymbol(ret, "given", k_pnt, 0);
	tableAddSymbol(ret, "float", k_float, 0);
	tableAddSymbol(ret, "Begin", k_begin, 0);
	tableAddSymbol(ret, "End", k_halt, 0);
	tableAddSymbol(ret, ",", k_clr, 0);
	tableAddSymbol(ret, ":", k_clr, 0);
	tableAddSymbol(ret, ";", k_endStatement, 0);
	tableAddSymbol(ret, "$", k_cont, 0);
	tableAddSymbol(ret, "!", k_not, 0);
	tableAddSymbol(ret, "=", k_is, 0);
	tableAddSymbol(ret, "set", k_set, 0);
	tableAddSymbol(ret, "==", k_eq, 0);
	tableAddSymbol(ret, ">", k_gt, 0);
	tableAddSymbol(ret, "<", k_lt, 0);
	tableAddSymbol(ret, "+", k_add, 0);
	tableAddSymbol(ret, "-", k_sub, 0);
	tableAddSymbol(ret, "*", k_mul, 0);
	tableAddSymbol(ret, "/", k_div, 0);
	tableAddSymbol(ret, "%", k_mod, 0);
	tableAddSymbol(ret, "&", k_and, 0);
	tableAddSymbol(ret, "|", k_or, 0);
	tableAddSymbol(ret, "++", k_fadd, 0);
	tableAddSymbol(ret, "--", k_fsub, 0);
	tableAddSymbol(ret, "**", k_fmul, 0);
	tableAddSymbol(ret, "//", k_fdiv, 0);
	tableAddSymbol(ret, "return", k_return, 0);
	tableAddSymbol(ret, "public", k_public, 0);
	tableAddSymbol(ret, "private", k_private, 0);
	tableAddSymbol(ret, "\\", k_literal, 0);
	tableAddSymbol(ret, "Collection", k_collect, 0);
	tableAddSymbol(ret, "Field", k_field, 0);
	tableAddSymbol(ret, "alloc", k_alloc, 0);
	tableAddSymbol(ret, "new", k_new, 0);
	tableAddSymbol(ret, "free", k_free, 0);
	tableAddSymbol(ret, "include", k_include, 0);
	tableAddSymbol(ret, "~", k_native, 0);
	tableAddSymbol(ret, "define", k_label, 0);
	tableAddSymbol(ret, "global", k_static, 0);

	return ret;
}
