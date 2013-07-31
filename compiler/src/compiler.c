/*
compiler.c


	This is the compiler. It basically works as a large finite state machine, where each iteration of the compileStatement
	function corresponds to a new token-decided state, along with a context. Any state has the potential to write pneumonic 
	machine code to the output file using the writeObj function, or oftentimes recurse into compiling a substatement, then
	advancing to the next state.


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
#include <ffi.h>
#include <dlfcn.h>

int compileStatement(Table *keyWords, Table *symbols, char *src, int *SC, FILE *dst, int *LC, Context *context, int *lineCount) {
	int endOfStatement = 0;		//tells us whether or not to unstack operators
	int oldLC = *LC;		//for measuring output progress
	int fakeLC = 0;			//for measuring relative addresses sub-statement
	int fakeSC;			//for measuring relative addresses on sub-statement
	char tok[256];			//needs to be big in case of string
	char string[256];		//additional string space
	char *charPtr;			
	char *charPtr2;
	long nameAddr;			//for marking important working addresses
	long DC[3];			//data counters
	int i;				//iterator
	double tempFloat;		//for working floating point values
	long tokVal;			//value of current token
	int tokLen;			//length of current token
	Table *tempTable;		//for working symbol tables
	Stack *operationStack = stackCreate(32);	//for stacking operators

	Context subContext;
	subContext.publicFlag = context->publicFlag;
	subContext.literalFlag = context->literalFlag;
	subContext.nativeFlag = context->nativeFlag;
	subContext.staticFlag = context->staticFlag;
	subContext.parameterFlag = context->parameterFlag;
	subContext.instructionFlag = context->instructionFlag;

	while(!endOfStatement && tokLen != -1) {
		tokLen = getToken(tok, src, SC, lineCount);
//printf("token:\t%s\n", tok);
		if(!tok[0]) {
			printf("Error: Expected } symbol.\n");
			//exit(0);
		}
		tempTable = tableLookup(keyWords, tok, &fakeLC);
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
				subContext.instructionFlag = 1;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//condition
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//clause
				DC[2] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//else

				writeObj(dst, RPUSH, LC);	writeObj(dst, DC[0]+DC[1]+6, LC);	//else address
				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled condition
				writeObj(dst, BNE, LC);								//decide

				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, lineCount);		//compiled statement
				writeObj(dst, RPUSH, LC);	writeObj(dst, DC[2]+3, LC);	//push end address
				writeObj(dst, JMP, LC);		//jump to end

				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled else
				break;


			case(k_while):
				//get beginning address
				nameAddr = *LC;

				//get section lengths
				fakeSC = *SC;
				subContext.instructionFlag = 1;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//condition length
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//loop length

				writeObj(dst, RPUSH, LC);	writeObj(dst, DC[0]+DC[1]+6, LC);	//end address
				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled condtion
				writeObj(dst, BNE, LC);								//decide
				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);			//compiled loop
				writeObj(dst, RPUSH, LC);	writeObj(dst, -(DC[0]+DC[1]+3), LC);			//begin address
				writeObj(dst, JMP, LC);								//iterate
				break;


			case(k_Function):
				//in a new namespace
				nameAddr = *LC + ((context->instructionFlag)?3:0);	//if these are instructions, jump over this
				stackPush(nameStack, nameAddr);
				tokLen = getToken(tok, src, SC, lineCount);
				symbols = tableAddSymbol(symbols, tok, nameAddr, context->staticFlag, context->parameterFlag);	//change to this scope
				if(context->publicFlag) { publicize(symbols); context->publicFlag = 0; }
				symbols = tableAddLayer(symbols, tok, 0);
	
				//count length of parameters, data, and statement sections
				fakeSC = *SC;
				fakeLC = 0;
				subContext.parameterFlag = 1;
				subContext.instructionFlag = 0;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//param length
				subContext.parameterFlag = 0;
				fakeLC = 0;	//because parameters don't increment location counter
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//data length
				subContext.instructionFlag = 1;
				DC[2] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//statement length
				subContext.instructionFlag = 0;
				
				if(context->instructionFlag) {
					writeObj(dst, PUSH, LC);	writeObj(dst, DC[1]+DC[2]+6, LC);
					writeObj(dst, HOP, LC);		//Hop over the definition
				}

				writeObj(dst, (DC[1]+2), LC);		//name pointer to statement. Add value to nameAddr when calling; it's relative
				writeObj(dst, DC[0], LC);
				fakeLC = 0;
				subContext.parameterFlag = 1;
				compileStatement(keyWords, symbols, src, SC, dst, &fakeLC, &subContext, (dst)?lineCount:&i);	//compiled parameters section
				subContext.parameterFlag = 0;
				//*LC += fakeLC;
				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);	//compiled data section
				subContext.instructionFlag = 1;
				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);	//compiled statement section
				subContext.instructionFlag = context->instructionFlag;
				writeObj(dst, RPUSH, LC);	writeObj(dst, nameAddr - *LC + 1, LC);
				writeObj(dst, RSR, LC);
	
				if(context->instructionFlag) {
					//basically, act like a lambda function
					writeObj(dst, RPUSH, LC);	writeObj(dst, nameAddr - *LC+1, LC);
				}

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
				subContext.instructionFlag = 1;
				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, lineCount);
				subContext.instructionFlag = context->instructionFlag;
				break;


			case(k_cParen):
				fillOperations(dst, LC, operationStack);
				endOfStatement = 1;
				break;


			case(k_oBracket):
				if(!context->nativeFlag) {
					//set aside call address/string for now
					writeObj(dst, PUSH, LC);		writeObj(dst, 2, LC);	//jump over word of call data
					writeObj(dst, HOP, LC);
					nameAddr = *LC;
					writeObj(dst, 0, LC);	//storage for call address
					writeObj(dst, POPTO, LC);	writeObj(dst, -1, LC); //writeObj(dst, nameAddr - *LC + 1, LC);

					//find length of arguments
					fakeSC = *SC;
					subContext.instructionFlag = 1;
					DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);

					//push return address of call
					writeObj(dst, RPUSH, LC);	writeObj(dst, DC[0]+5, LC);			//push return address

					//compile arguments
					compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);
					subContext.instructionFlag = context->instructionFlag;

					//make call
					writeObj(dst, RPUSH, LC);	writeObj(dst, nameAddr - *LC+1, LC);	//push function pointer
					writeObj(dst, JSR, LC);

					//return to right here

				} else {
					//compiled argument section
					subContext.instructionFlag = 1;
					compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);
					subContext.instructionFlag = context->instructionFlag;

					//make call
					writeObj(dst, NTV, LC);
				}
				break;


			case(k_cBracket):
				fillOperations(dst, LC, operationStack);
				if(*LC - oldLC) writeObj(dst, APUSH, LC);
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
				tokLen = getToken(tok, src, SC, lineCount);
				writeObj(dst, PUSH, LC);	writeObj(dst, tok[0], LC);
				break;


			case(k_doubleQuote):
				DC[0] = getQuote(tok, src, SC);
				if(!context->literalFlag) {
					writeObj(dst, RPUSH, LC);	writeObj(dst, 5, LC);	//push start of string
					writeObj(dst, RPUSH, LC);	writeObj(dst, 3+DC[0], LC);	//push end of string
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
				tokLen = getToken(tok, src, SC, lineCount);
				if(numeric(tok[0])) {		//is this an array?
					//create a symbol and some memory
					DC[0] = atoi(tok);
					writeObj(dst, *LC, LC);
					for(i=0;i<DC[0];i++)
						writeObj(dst, DC[0], LC);
					tokLen = getToken(tok, src, SC, lineCount);
					if(dst) {
						tempTable = tableAddSymbol(symbols, tok, *LC-DC[0]-1, context->staticFlag, context->parameterFlag);
					}
				} else {
					//create a symbol and a word
					tempTable = tableAddSymbol(symbols, tok, *LC + ((context->instructionFlag)?1:0), context->staticFlag, context->parameterFlag);
					if(!context->parameterFlag) {
						if(context->publicFlag) publicize(tempTable);
						if(context->instructionFlag) writeObj(dst, GRAB, LC);
						writeObj(dst, 0, LC);
					} else {
						*LC++;
					}
				}
				break;


			case(k_float):
				stackPush(operationStack, DTOF);
				break;


			case(k_begin):
				transferAddress = *LC;
				context->instructionFlag = 1;
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
				subContext.publicFlag = 0;
				context->publicFlag = 0;
				subContext.nativeFlag = 0;
				context->nativeFlag = 0;
				subContext.staticFlag = 0;
				context->staticFlag = 0;
				break;

			case(k_argument):
				fillOperations(dst, LC, operationStack);
				if(!context->parameterFlag) writeObj(dst, APUSH, LC);
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
				stackPush(operationStack, RSR);
				break;


			case(k_public):
				subContext.publicFlag = 1;
				context->publicFlag = 1;
				break;


			case(k_private):
				subContext.publicFlag = 0;
				context->publicFlag = 0;
				break;


			case(k_literal):
				subContext.literalFlag = !context->literalFlag;
				context->literalFlag = !context->literalFlag;
				break;


			case(k_collect):
				//set the scope
				tokLen = getToken(tok, src, SC, lineCount);
				symbols = tableAddSymbol(symbols, tok, *LC + ((context->instructionFlag)?3:0), context->staticFlag, context->parameterFlag);	//add 3 if instructions
				symbols = tableAddLayer(symbols, tok, 1);

				//get its own length:
				fakeSC = *SC;
				fakeLC = 1;
				subContext.instructionFlag = 0;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);

				//hop over the definition
				if(context->instructionFlag) {
					writeObj(dst, PUSH, LC);	writeObj(dst, DC[0] + 2, LC);
					writeObj(dst, HOP, LC);
				}

				//write its length, and its body (addressed relative to right here)
				writeObj(dst, DC[0], LC);
				fakeLC = 1;
				subContext.instructionFlag = 0;
				compileStatement(keyWords, symbols, src, SC, dst, &fakeLC, &subContext, (dst)?lineCount:&i);
				subContext.instructionFlag = context->instructionFlag;
				*LC += fakeLC - 1; //accommodate for change in location
				
				//clean up
				symbols = tableRemoveLayer(symbols);
				break;


			case(k_field):
				//set the scope
				tokLen = getToken(tok, src, SC, lineCount);
				symbols = tableAddSymbol(symbols, tok, *LC + ((context->instructionFlag)?3:0), 0, context->parameterFlag);
				symbols = tableAddLayer(symbols, tok, 0);

				//get its own length:
				fakeSC = *SC;
				fakeLC = 0;
				subContext.instructionFlag = 0;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC, &subContext, &i);
				subContext.instructionFlag = context->instructionFlag;

				//hop over the definition
				if(context->instructionFlag) {
					writeObj(dst, PUSH, LC);	writeObj(dst, DC[0] + 1, LC);
					writeObj(dst, HOP, LC);
				}

				//compile the body
				subContext.instructionFlag = 0;
				compileStatement(keyWords, symbols, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);
				subContext.instructionFlag = context->instructionFlag;

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
				printf("%d:\tIncluding file: %s\n", *lineCount, tok);
				char *inc = loadFile(tok);
				trimComments(inc);
				fakeSC = 0;
				compileStatement(keyWords, symbols, inc, &fakeSC, dst, LC, &subContext, (dst)?lineCount:&i);
				free(inc);
				break; 


			case(k_native):
				subContext.nativeFlag = 1;
				context->nativeFlag = 1;
				break;



			case(k_label):
				//create a symbol referring to this location
				tokLen = getToken(tok, src, SC, lineCount);
				tempTable = tableAddSymbol(symbols, tok, *LC, context->staticFlag, context->parameterFlag);
				if(context->publicFlag) publicize(tempTable);
				break;


			case(k_static):
				subContext.staticFlag = 1;
				context->staticFlag = 1;
				break;


			case(k_redir):
				writeObj(dst, CONT, LC);
				tokLen = getToken(tok, src, SC, lineCount);
				tempTable = tableLookup(symbols, tok, &fakeLC);
				if(!tempTable) {
					if(numeric(tok[0])) {
						DC[0] = atoi(tok);
					} else {
						printf("%d:\tImplicitly declared offset: %s.\n", *lineCount, tok);
						DC[0] = 0;
					}
				} else {
					DC[0] = tempTable->val;
				}
				writeObj(dst, PUSH, LC);	writeObj(dst, DC[0]*WRDSZ, LC);
				writeObj(dst, ADD, LC);
				break;


			case(k_load):
				stackPush(operationStack, LOAD);
				break;

			case(k_nativeFunction):
				/*builds a struct like this:
					function pointer;
					return type;
					argument count;
					argument types
					...
					function name;
				*/
				tokLen = getToken(tok, src, SC, lineCount);
				tempTable = tableAddSymbol(symbols, tok, *LC, context->staticFlag, context->parameterFlag);
				getQuote(tok, src, SC);		//once first, to accommodate for opening "
				getQuote(tok, src, SC);
				strcpy(string, tok);

				charPtr = strtok(string, "():");	//grab function name

				writeObj(dst, 0, LC);		//space for function pointer

				//get return type
				charPtr2 = strtok(NULL, "():");
				writeObj(dst, charPtr2[0], LC);

				//get argument information
				charPtr2 = strtok(NULL, "():");
				writeObj(dst, strlen(charPtr2), LC);	//write argument count
				
				for(i=0;i<strlen(charPtr2);i++) {
					writeObj(dst, (long) charPtr2[i], LC);
				}

				//write function name
				writeStr(dst, charPtr, LC);
				break;



			default:
				//this is either an intended symbol or a number

				if(numeric(tok[0]) || (tok[0] == '-' && numeric(tok[1]))) {
					//this is a numeric literal
					if(isFloat(tok)) {
						tempFloat = atof(tok);
						if(!context->literalFlag) writeObj(dst, PUSH, LC);
						if(dst) {
							writeObj(dst, *(long *) &tempFloat, LC);
						}
					} else {
						if(!context->literalFlag) writeObj(dst, PUSH, LC);
						writeObj(dst, atol(tok), LC);	//push its value
					}
				} else {
					writeAddressCalculation(dst, tok, symbols, LC, context, lineCount);
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
		//printf("%x:%lx\n", *LC, val);
		//if(val < 0) printf("\tValue to write: -%lx (relatively %lx)\n", -val, *LC + val - 1);
	}
	//(*LC) += WRDSZ;
	++(*LC);
}

void writeStr(FILE *fn, char *str, int *LC) {
	//write a string of characters to the output file, padding null characters to align words
	int len = strlen(str) + 1;	//+1 to account for \0
	int words = (len%WRDSZ)?len/WRDSZ+1:len/WRDSZ;
	int padding = (len%WRDSZ)?WRDSZ - len%WRDSZ:0;
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

int writeAddressCalculation(FILE *dst, char *token, Table *symbols, int *LC, Context *context, int *lineCount) {
	//this function figures out what address a non-keyword token should correlate to
	//and writes that address to the output file
	int oldLC = *LC;

	int fakeLC = 0;
	Table *sym = tableLookup(symbols, token, &fakeLC);
	
	if(sym == NULL) {	//does the symbol not exist yet?
		if(dst) tableAddSymbol(symbols, token, *LC+((context->instructionFlag)?1:0), context->staticFlag, context->parameterFlag);
		sym = tableLookup(symbols, token, &fakeLC);
		if(!context->parameterFlag) {
			//This is an implicitly declared variable
			if(dst) printf("%d:\tImplicitly declared symbol: %s:%x, %d\n", *lineCount, sym->token, sym->val, sym->staticFlag);
			if(context->publicFlag) publicize(sym);
			if(context->instructionFlag) writeObj(dst, GRAB, LC);
			writeObj(dst, 0, LC);
		} else {
			//this is a parameter declaration, count it and carry on
			(*LC)++;
		}
		return *LC - oldLC;
	}

	int value = sym->val + fakeLC;
	
	if(sym->parameterFlag) {
		writeObj(dst, AGET, LC);
		writeObj(dst, value, LC);
	} else {
		if(!sym->staticFlag) {
			if(!fakeLC) {
				//symbol is not being called for within a collection
				//if(dst) printf("%d:\t%s, to dynamic, no transversal\n", *lineCount, sym->token);
				if(!context->literalFlag) {
					writeObj(dst, RPUSH, LC);
				}
				writeObj(dst, value - *LC + 1, LC);
			} else {
				//if(dst) printf("%d:\t%s to static from dynamic\n", *lineCount, sym->token);
				writeObj(dst, PUSH, LC);	writeObj(dst, sym->val, LC);
				writeObj(dst, LOC, LC);
			}
		} else {
			//if(dst) printf("%d:\t%s to static from somewhere\n", *lineCount, sym->token);
			if(!context->literalFlag) writeObj(dst, PUSH, LC);
			writeObj(dst, value, LC);
			writeObj(dst, LOC, LC);
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
	tableAddSymbol(ret, "if", k_if, 0, 0);
	tableAddSymbol(ret, "while", k_while, 0, 0);
	tableAddSymbol(ret, "Function", k_Function, 0, 0);
	tableAddSymbol(ret, "[", k_oBracket, 0, 0);
	tableAddSymbol(ret, "]", k_cBracket, 0, 0);
	tableAddSymbol(ret, "{", k_oBrace, 0, 0);
	tableAddSymbol(ret, "}", k_cBrace, 0, 0);
	tableAddSymbol(ret, "(", k_oParen, 0, 0);
	tableAddSymbol(ret, ")", k_cParen, 0, 0);
	tableAddSymbol(ret, "printd", k_prnt, 0, 0);
	tableAddSymbol(ret, "printf", k_prtf, 0, 0);
	tableAddSymbol(ret, "printc", k_prtc, 0, 0);
	tableAddSymbol(ret, "prints", k_prts, 0, 0);
	tableAddSymbol(ret, "goto", k_goto, 0, 0);
	tableAddSymbol(ret, "\'", k_singleQuote, 0, 0);
	tableAddSymbol(ret, "\"", k_doubleQuote, 0, 0);
	tableAddSymbol(ret, "int", k_int, 0, 0);
	tableAddSymbol(ret, "char", k_char, 0, 0);
	tableAddSymbol(ret, "given", k_pnt, 0, 0);
	tableAddSymbol(ret, "float", k_float, 0, 0);
	tableAddSymbol(ret, "Begin", k_begin, 0, 0);
	tableAddSymbol(ret, "End", k_halt, 0, 0);
	tableAddSymbol(ret, ",", k_argument, 0, 0);
	tableAddSymbol(ret, "^", k_clr, 0, 0);
	tableAddSymbol(ret, ";", k_endStatement, 0, 0);
	tableAddSymbol(ret, "$", k_cont, 0, 0);
	tableAddSymbol(ret, "!", k_not, 0, 0);
	tableAddSymbol(ret, "=", k_is, 0, 0);
	tableAddSymbol(ret, "set", k_set, 0, 0);
	tableAddSymbol(ret, "==", k_eq, 0, 0);
	tableAddSymbol(ret, ">", k_gt, 0, 0);
	tableAddSymbol(ret, "<", k_lt, 0, 0);
	tableAddSymbol(ret, "+", k_add, 0, 0);
	tableAddSymbol(ret, "-", k_sub, 0, 0);
	tableAddSymbol(ret, "*", k_mul, 0, 0);
	tableAddSymbol(ret, "/", k_div, 0, 0);
	tableAddSymbol(ret, "%", k_mod, 0, 0);
	tableAddSymbol(ret, "&", k_and, 0, 0);
	tableAddSymbol(ret, "|", k_or, 0, 0);
	tableAddSymbol(ret, "++", k_fadd, 0, 0);
	tableAddSymbol(ret, "--", k_fsub, 0, 0);
	tableAddSymbol(ret, "**", k_fmul, 0, 0);
	tableAddSymbol(ret, "//", k_fdiv, 0, 0);
	tableAddSymbol(ret, "return", k_return, 0, 0);
	tableAddSymbol(ret, "public", k_public, 0, 0);
	tableAddSymbol(ret, "private", k_private, 0, 0);
	tableAddSymbol(ret, "\\", k_literal, 0, 0);
	tableAddSymbol(ret, "Collection", k_collect, 0, 0);
	tableAddSymbol(ret, "Field", k_field, 0, 0);
	tableAddSymbol(ret, "alloc", k_alloc, 0, 0);
	tableAddSymbol(ret, "new", k_new, 0, 0);
	tableAddSymbol(ret, "free", k_free, 0, 0);
	tableAddSymbol(ret, "include", k_include, 0, 0);
	tableAddSymbol(ret, "@", k_native, 0, 0);
	tableAddSymbol(ret, "define", k_label, 0, 0);
	tableAddSymbol(ret, "global", k_static, 0, 0);
	tableAddSymbol(ret, "->", k_redir, 0, 0);
	tableAddSymbol(ret, "load", k_load, 0, 0);
	tableAddSymbol(ret, "Native", k_nativeFunction, 0, 0);

	return ret;
}
