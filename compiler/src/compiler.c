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

int compileStatement(Table *keyWords, Table *symbols, char *src, int *SC, FILE *dst, int *LC) {
	publicFlag = 0;
	literalFlag = 0;
	nativeFlag = 0;

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
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//condition
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//clause
				DC[2] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//else

				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr+DC[0]+DC[1]+6, LC);	//else address
				compileStatement(keyWords, symbols, src, SC, dst, LC);				//compiled condition
				writeObj(dst, BNE, LC);								//decide

				compileStatement(keyWords, symbols, src, SC, dst, LC);				//compiled statement
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr+DC[0]+DC[1]+6+DC[2], LC);	//push end address
				writeObj(dst, JMP, LC);		//jump to end

				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled else
				break;


			case(k_while):
				//get beginning address
				nameAddr = *LC;

				//get section lengths
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//condition length
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//loop length

				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr+DC[0]+DC[1]+6, LC);	//end address
				compileStatement(keyWords, symbols, src, SC, dst, LC);				//compiled condtion
				writeObj(dst, BNE, LC);								//decide
				compileStatement(keyWords, symbols, src, SC, dst, LC);				//compiled loop
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);			//begin address
				writeObj(dst, JMP, LC);								//iterate
				break;


			case(k_Function):
				writeObj(dst, 0, LC);						//result location
	
				//in a new namespace
				nameAddr = *LC;
				stackPush(nameStack, *LC);
				tokLen = getToken(tok, src, SC);
				symbols = tableAddSymbol(symbols, tok, nameAddr);	//change to this scope
				if(publicFlag) { publicize(symbols); publicFlag = 0; }
				symbols = tableAddLayer(symbols, tok);
	
				//count length of parameters and data sections
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//param length
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//data length
				
				writeObj(dst, DC[0]+DC[1]+1, LC);		//name pointer to statement. Add value to nameAddr when calling; it's relative
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled parameters section
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled data section
	
				//fill parameters with arguments
				for(i=DC[0]-1;i>=0;i--){
					writeObj(dst, POPTO, LC);				//POPTO each argument
					writeObj(dst, nameAddr+2+i - *LC, LC);
				}
	
				//writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr-1, LC);	//push result address
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled statement section
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
				compileStatement(keyWords, symbols, src, SC, dst, LC);
				break;


			case(k_cParen):
				fillOperations(dst, LC, operationStack);
				endOfStatement = 1;
				break;


			case(k_oBracket):
				//set aside call address/string for now
				writeObj(dst, PUSH, LC);		writeObj(dst, *LC+3, LC);	//jump over word of call data
				writeObj(dst, JMP, LC);
					nameAddr = *LC;
					writeObj(dst, 0, LC);	//storage for call address
				writeObj(dst, POPTO, LC);	writeObj(dst, nameAddr - *LC + 1, LC);

				if(!nativeFlag) {
					//find length of arguments
					fakeSC = *SC;
					DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);		//compiled argument section

					//push return address of call
					writeObj(dst, PUSH, LC);	writeObj(dst, *LC+DC[0]+23, LC);			//push return address

					//push result address for function
					writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
					writeObj(dst, LOC, LC);						//turn into address
					writeObj(dst, CONT, LC);					//turn pointer into relative address base
					writeObj(dst, PUSH, LC);	writeObj(dst, 8, LC);		//push function pointer
					writeObj(dst, SUB, LC);						//combine base and offset to get result address

					//compile arguments
					compileStatement(keyWords, symbols, src, SC, dst, LC);

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
					compileStatement(keyWords, symbols, src, SC, dst, LC);

					//push call string with format: "foo(isf)@libbar.so" for a function that takes an integer, string, and float as parameters
					writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//stored pointer here earlier
					writeObj(dst, LOC, LC);
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
					writeObj(dst, PUSH, LC);	writeObj(dst, *LC+5, LC);	//push start of string
					writeObj(dst, LOC, LC);
					writeObj(dst, PUSH, LC);	writeObj(dst, *LC+2+DC[0], LC);	//push end of string
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
						tempTable = tableAddSymbol(symbols, tok, *LC-DC[0]-1);
					}
				} else {
					//create a symbol and a word
					tempTable = tableAddSymbol(symbols, tok, *LC);
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
				symbols = tableAddSymbol(symbols, tok, *LC);
				symbols = tableAddLayer(symbols, tok);

				//get its own length:
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);
				*LC += fakeLC; //accommodate for change in location

				//write its length, and its body (addressed relative to right here)
				writeObj(dst, DC[0], LC);
				fakeLC = 0;
				compileStatement(keyWords, symbols, src, SC, dst, &fakeLC);
				
				//clean up
				symbols = tableRemoveLayer(symbols);
				break;


			case(k_field):
				//set the scope
				tokLen = getToken(tok, src, SC);
				symbols = tableAddSymbol(symbols, tok, *LC);
				symbols = tableAddLayer(symbols, tok);

				//compile the body
				compileStatement(keyWords, symbols, src, SC, dst, LC);

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
				getQuote(tok, src, SC);			//to accommodate for the beginning "
				printf("INCLUDING FILE: %s\n", tok);
				char *inc = loadFile(tok);
				trimComments(inc);
				fakeSC = 0;
				compileStatement(keyWords, symbols, inc, &fakeSC, dst, LC);
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
				tempTable = tableAddSymbol(symbols, tok, *LC);
				if(publicFlag) publicize(tempTable);
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
					writeAddressCalculation(dst, tok, symbols, LC);
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

int writeAddressCalculation(FILE *dst, char *token, Table *symbols, int *LC) {
	//this function figures out what address a non-keyword token should correlate to
	//and writes that address to the output file
	int oldLC = *LC;
	char *piece0 = strtok(token, "@");
	char *piece1 = strtok(NULL, "@");

	Table *sym = tableLookup(symbols, piece0);
	
	if(sym == NULL) {	//does the symbol not exist yet?
		tableAddSymbol(symbols, piece0, *LC);
		sym = tableLookup(symbols, piece0);
		if(publicFlag) publicize(sym);
	}
	
	if(piece1) {	//is this a compound symbol?
		writeObj(dst, RPUSH, LC);	writeObj(dst, tableLookup(symbols, piece1)->val - *LC + 1, LC);
		writeObj(dst, CONT, LC);	//resolve pointer
		writeObj(dst, DLOC, LC);
		writeObj(dst, PUSH, LC);	writeObj(dst, sym->val, LC);
		writeObj(dst, ADD, LC);
		writeObj(dst, LOC, LC);
	} else {
		if(!literalFlag) writeObj(dst, RPUSH, LC);
		writeObj(dst, sym->val - *LC + 1, LC);
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
	tableAddSymbol(ret, "if", k_if);
	tableAddSymbol(ret, "while", k_while);
	tableAddSymbol(ret, "Function", k_Function);
	tableAddSymbol(ret, "[", k_oBracket);
	tableAddSymbol(ret, "]", k_cBracket);
	tableAddSymbol(ret, "{", k_oBrace);
	tableAddSymbol(ret, "}", k_cBrace);
	tableAddSymbol(ret, "(", k_oParen);
	tableAddSymbol(ret, ")", k_cParen);
	tableAddSymbol(ret, "printd", k_prnt);
	tableAddSymbol(ret, "printf", k_prtf);
	tableAddSymbol(ret, "printc", k_prtc);
	tableAddSymbol(ret, "prints", k_prts);
	tableAddSymbol(ret, "goto", k_goto);
	tableAddSymbol(ret, "\'", k_singleQuote);
	tableAddSymbol(ret, "\"", k_doubleQuote);
	tableAddSymbol(ret, "int", k_int);
	tableAddSymbol(ret, "char", k_char);
	tableAddSymbol(ret, "given", k_pnt);
	tableAddSymbol(ret, "float", k_float);
	tableAddSymbol(ret, "Begin", k_begin);
	tableAddSymbol(ret, "End", k_halt);
	tableAddSymbol(ret, ",", k_clr);
	tableAddSymbol(ret, ":", k_clr);
	tableAddSymbol(ret, ";", k_endStatement);
	tableAddSymbol(ret, "$", k_cont);
	tableAddSymbol(ret, "!", k_not);
	tableAddSymbol(ret, "=", k_is);
	tableAddSymbol(ret, "==", k_eq);
	tableAddSymbol(ret, ">", k_gt);
	tableAddSymbol(ret, "<", k_lt);
	tableAddSymbol(ret, "+", k_add);
	tableAddSymbol(ret, "-", k_sub);
	tableAddSymbol(ret, "*", k_mul);
	tableAddSymbol(ret, "/", k_div);
	tableAddSymbol(ret, "%", k_mod);
	tableAddSymbol(ret, "&", k_and);
	tableAddSymbol(ret, "|", k_or);
	tableAddSymbol(ret, "++", k_fadd);
	tableAddSymbol(ret, "--", k_fsub);
	tableAddSymbol(ret, "**", k_fmul);
	tableAddSymbol(ret, "//", k_fdiv);
	tableAddSymbol(ret, "return", k_return);
	tableAddSymbol(ret, "public", k_public);
	tableAddSymbol(ret, "private", k_private);
	tableAddSymbol(ret, "\\", k_literal);
	tableAddSymbol(ret, "Collection", k_collect);
	tableAddSymbol(ret, "Field", k_field);
	tableAddSymbol(ret, "alloc", k_alloc);
	tableAddSymbol(ret, "new", k_new);
	tableAddSymbol(ret, "free", k_free);
	tableAddSymbol(ret, "include", k_include);
	tableAddSymbol(ret, "~", k_native);
	tableAddSymbol(ret, "define", k_label);

	return ret;
}
