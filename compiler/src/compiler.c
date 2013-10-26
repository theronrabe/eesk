/*
compiler.c


	This is the compiler. It basically works as a large finite state machine, where each iteration of the compileStatement
	function corresponds to a new token-decided state, along with a context. Any state has the potential to write mnemonic
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
#include <writer.h>
#include <eeskIR.h>
#include <translations.h>

int compileStatement(Table *keyWords, Table *symbols, translation *dictionary, char *src, int *SC, FILE *dst, int *LC, Context *context, int *lineCount) {
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
		if(!tok[0] && dst) {
			printf("%d: Expected } symbol.\n", *lineCount);
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
				symbols = tableAddLayer(symbols, tok, 0);
				DC[0] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//condition
				DC[1] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//clause
				DC[2] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//else
				symbols = tableRemoveLayer(symbols);

				fakeLC = dictionary[BNE].length + dictionary[RPUSH].length + dictionary[JMP].length + 1;
				writeObj(dst, RPUSH, DC[0]+DC[1]+fakeLC, dictionary, LC);						//else address
				compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled condition
				writeObj(dst, BNE, 0, dictionary, LC);								//decide

				compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, lineCount);		//compiled statement
				fakeLC = dictionary[JMP].length + 1;
				writeObj(dst, RPUSH, DC[2]+fakeLC, dictionary, LC);						//push end address
				writeObj(dst, JMP, 0, dictionary, LC);							//jump to end

				compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled else
				break;


			case(k_while):
				//get beginning address
				nameAddr = *LC;

				//get section lengths
				fakeSC = *SC;
				fakeLC = 0;
				subContext.instructionFlag = 1;
				symbols = tableAddLayer(symbols, tok, 0);
				DC[0] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//condition length
				DC[1] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//loop length
				symbols = tableRemoveLayer(symbols);

				fakeLC = dictionary[BNE].length + dictionary[RPUSH].length + dictionary[JMP].length + 1;
				writeObj(dst, RPUSH, DC[0]+DC[1]+fakeLC, dictionary, LC);	//end address

				DC[0] = compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled condtion
				writeObj(dst, BNE, 0, dictionary, LC);								//decide
				DC[1] = compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);			//compiled loop

				fakeLC = dictionary[RPUSH].length;
				writeObj(dst, RPUSH, /*-(DC[0]+DC[1]+fakeLC)+1*/nameAddr - *LC - fakeLC + 1, dictionary, LC);			//begin address
				writeObj(dst, JMP, 0, dictionary, LC);								//iterate
				break;


			case(k_Function):
				//in a new namespace
				nameAddr = *LC + ((context->instructionFlag)?3:0);	//if these are instructions, jump over this
				stackPush(nameStack, nameAddr);
				tokLen = getToken(tok, src, SC, lineCount);
				symbols = tableAddSymbol(symbols, tok, nameAddr, context->staticFlag, context->parameterFlag);	//change to this scope
				if(context->publicFlag) { publicize(symbols); }
				symbols = tableAddLayer(symbols, tok, 0);
	
				//count length of parameters, data, and statement sections
				fakeSC = *SC;
				fakeLC = 0;
				subContext.parameterFlag = 1;
				subContext.instructionFlag = 0;
				symbols = tableAddLayer(symbols, tok, 0);
				DC[0] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//param length
				subContext.parameterFlag = 0;
				fakeLC = 0;	//because parameters don't increment location counter
				DC[1] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//data length
				subContext.instructionFlag = 1;
				DC[2] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);	//statement length
				symbols = tableRemoveLayer(symbols);
				subContext.instructionFlag = 0;
				
				if(context->instructionFlag) {
					fakeLC = dictionary[JMP].length + dictionary[DATA].length + dictionary[RPUSH].length + dictionary[RSR].length + 1;
					writeObj(dst, RPUSH, DC[1]+DC[2]+fakeLC, dictionary, LC);	//this used to be a PUSH
					writeObj(dst, JMP, 0, dictionary, LC);		//Hop over the definition	//this used to be a HOP
				}

				//writeObj(dst, (DC[1]+2), LC);		//name pointer to statement. Add value to nameAddr when calling; it's relative

				//reset calling address
				nameAddr = *LC + DC[1] + WRDSZ;
				tableAddSymbol(symbols->parent->layerRoot, tok, nameAddr, context->staticFlag, context->parameterFlag);		//overwrite previous definition
				if(context->publicFlag) { publicize(symbols->parent); }

				fakeLC = 0;
				subContext.parameterFlag = 1;
				compileStatement(keyWords, symbols, dictionary, src, SC, dst, &fakeLC, &subContext, (dst)?lineCount:&i);	//compiled parameters section
				subContext.parameterFlag = 0;
				compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);	//compiled data section
				subContext.instructionFlag = 1;

				writeObj(dst, DATA, DC[0]+WRDSZ, dictionary, LC);	//write argument number at callAddress-1, extra word for return address
				//reset calling address to right here

				compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);	//compiled statement section
				subContext.instructionFlag = context->instructionFlag;
				writeObj(dst, RPUSH, nameAddr - *LC + 1 - WRDSZ, dictionary, LC);
				writeObj(dst, RSR, 0, dictionary, LC);
	
				if(context->instructionFlag) {
					//basically, act like a lambda function
					writeObj(dst, RPUSH, nameAddr - *LC+1 - WRDSZ, dictionary, LC);
				}

				//no longer in this namespace
				stackPop(nameStack);
				symbols = tableRemoveLayer(symbols);
				break;


			case(k_oBrace):
				//This seemingly important symbol literally does nothing.
				break;


			case(k_cBrace):
				fillOperations(dst, LC, operationStack, dictionary);
				endOfStatement = 1;
				break;


			case(k_oParen):
				subContext.instructionFlag = 1;
				compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, lineCount);
				subContext.instructionFlag = context->instructionFlag;
				break;


			case(k_cParen):
				fillOperations(dst, LC, operationStack, dictionary);
				endOfStatement = 1;
				break;


			case(k_oBracket):
				if(!context->nativeFlag) {
					//set aside call address/string for now
					/*
					writeObj(dst, PUSH, LC);		writeObj(dst, 2, LC);	//jump over word of call data
					writeObj(dst, HOP, LC);
					nameAddr = *LC;
					writeObj(dst, 0, LC);	//storage for call address
					writeObj(dst, POPTO, LC);	writeObj(dst, -1, LC); //writeObj(dst, nameAddr - *LC + 1, LC);
					*/

					//find length of arguments
					fakeSC = *SC;
					subContext.instructionFlag = 1;
					symbols = tableAddLayer(symbols, tok, 0);
					DC[0] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);
					symbols = tableRemoveLayer(symbols);

					//push return address of call
					writeObj(dst, RPUSH, DC[0]+dictionary[JSR].length+1, dictionary, LC);			//push return address

					//compile arguments
					compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);
					subContext.instructionFlag = context->instructionFlag;

					//make call
					//writeObj(dst, RPUSH, LC);	writeObj(dst, nameAddr - *LC+1, LC);	//push function pointer
					writeObj(dst, JSR, 0, dictionary, LC);

					//return to right here

				} else {
					//compiled argument section
					subContext.instructionFlag = 1;
					compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);
					subContext.instructionFlag = context->instructionFlag;

					//make call
					writeObj(dst, NTV, 0, dictionary, LC);
				}
				break;


			case(k_cBracket):
				fillOperations(dst, LC, operationStack, dictionary);
				if(*LC - oldLC) {
					writeObj(dst, APUSH, 0, dictionary, LC);
				}
				endOfStatement = 1;
				break;


			case(k_prnt):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, PRNT);
				break;


			case(k_prtf):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, PRTF);
				break;


			case(k_prtc):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, PRTC);
				break;


			case(k_prts):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, PRTS);
				break;


			case(k_goto):
				stackPush(operationStack, JMP);
				break;


			case(k_singleQuote):
				tokLen = getToken(tok, src, SC, lineCount);
				writeObj(dst, PUSH, tok[0], dictionary, LC);
				break;


			case(k_doubleQuote):
				DC[0] = getQuote(tok, src, SC);
				if(!context->literalFlag) {
					writeObj(dst, RPUSH, dictionary[RPUSH].length+dictionary[JMP].length+1, dictionary, LC);	//push start of string
					writeObj(dst, RPUSH, DC[0] + dictionary[JMP].length+1, dictionary, LC);	//push end of string
					writeObj(dst, JMP, 0, dictionary, LC);						//skip over string leaving it on stack
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
					writeObj(dst, DATA, *LC, dictionary, LC);
					for(i=0;i<DC[0];i++)
						writeObj(dst, DATA, DC[0], dictionary, LC);
					tokLen = getToken(tok, src, SC, lineCount);
					if(dst) {
						tempTable = tableAddSymbol(symbols, tok, *LC-DC[0]-1, context->staticFlag, context->parameterFlag);
					}
				} else {
					//create a symbol and a word
					tempTable = tableAddSymbol(symbols, tok, *LC + ((context->instructionFlag)?1:0), context->staticFlag, context->parameterFlag);
					if(!context->parameterFlag) {
						if(context->publicFlag) publicize(tempTable);
						if(context->instructionFlag) writeObj(dst, GRAB, 0, dictionary, LC);
						writeObj(dst, DATA, 0, dictionary, LC);
					} else {
						//++(*LC);
						(*LC) += WRDSZ;
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
				fillOperations(dst, LC, operationStack, dictionary);
				writeObj(dst, HALT, 0, dictionary, LC);
				break;


			case(k_clr):
				writeObj(dst, CLR, 0, dictionary, LC);
				break;


			case(k_endStatement):
				//unstack operators and reset flags
				fillOperations(dst, LC, operationStack, dictionary);
				subContext.publicFlag = 0;
				context->publicFlag = 0;
				subContext.nativeFlag = 0;
				context->nativeFlag = 0;
				subContext.staticFlag = 0;
				context->staticFlag = 0;
				break;

			case(k_argument):
				fillOperations(dst, LC, operationStack, dictionary);
				if(!context->parameterFlag) writeObj(dst, APUSH, 0, dictionary, LC);
				break;


			case(k_cont):
				writeObj(dst, CONT, 0, dictionary, LC);
				break;


			case(k_not):
				writeObj(dst, NOT, 0, dictionary, LC);
				break;


			case(k_shift):
				stackPush(operationStack, SHIFT);
				break;


			case(k_is):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, POP);
				break;

			case(k_isr):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, RPOP);
				break;

			case(k_set):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, BPOP);
				break;


			case(k_eq):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, EQ);
				break;


			case(k_gt):
				fillOperations(dst, LC, operationStack, dictionary);
				stackPush(operationStack, GT);
				break;


			case(k_lt):
				fillOperations(dst, LC, operationStack, dictionary);
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
				nameAddr = *LC + ((context->instructionFlag)?3:0);	//add 3 if instructions
				symbols = tableAddSymbol(symbols, tok, nameAddr, context->staticFlag, context->parameterFlag);
				symbols = tableAddLayer(symbols, tok, 1);

				//get its own length:
				fakeSC = *SC;
				fakeLC = 8;
				subContext.instructionFlag = 0;
				symbols = tableAddLayer(symbols, tok, 1);
				DC[0] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);
				symbols = tableRemoveLayer(symbols);

				//hop over the definition
				if(context->instructionFlag) {
					fakeLC = dictionary[JMP].length + WRDSZ + 1;
					writeObj(dst, RPUSH, DC[0] + fakeLC, dictionary, LC);
					writeObj(dst, JMP, 0, dictionary, LC);
				}

				//write its length, and its body (addressed relative to right here)
				writeObj(dst, DATA, DC[0], dictionary, LC);
				fakeLC = 8;
				subContext.instructionFlag = 0;
				compileStatement(keyWords, symbols, dictionary, src, SC, dst, &fakeLC, &subContext, (dst)?lineCount:&i);
				subContext.instructionFlag = context->instructionFlag;
				*LC += fakeLC - 8; //accommodate for change in location

				//push newing address, if we're in the middle of instructions
				if(context->instructionFlag) {
					writeObj(dst, RPUSH, nameAddr - *LC+1, dictionary, LC);
				}
				
				//clean up
				symbols = tableRemoveLayer(symbols);
				break;


			case(k_field):
				//set the scope
				tokLen = getToken(tok, src, SC, lineCount);
				nameAddr = *LC + ((context->instructionFlag)?3:0);
				symbols = tableAddSymbol(symbols, tok, nameAddr, 0, context->parameterFlag);
				symbols = tableAddLayer(symbols, tok, 0);

				//get its own length:
				fakeSC = *SC;
				fakeLC = 0;
				subContext.instructionFlag = 0;
				subContext.staticFlag = 0;
				symbols = tableAddLayer(symbols, tok, 0);
				DC[0] = compileStatement(keyWords, symbols, dictionary, src, &fakeSC, NULL, &fakeLC, &subContext, &i);
				symbols = tableRemoveLayer(symbols);
				subContext.instructionFlag = context->instructionFlag;

				//hop over the definition
				if(context->instructionFlag) {
					writeObj(dst, PUSH, DC[0] + 1, dictionary, LC);
					writeObj(dst, HOP, 0, dictionary, LC);
				}

				//compile the body
				subContext.instructionFlag = 0;
				subContext.staticFlag = 0;
				compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);
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
				if(dst) printf("%d:\tIncluding file: %s\n", *lineCount, tok);
				char *inc = loadFile(tok);
				trimComments(inc);
				fakeSC = 0;
				compileStatement(keyWords, symbols, dictionary, inc, &fakeSC, dst, LC, &subContext, (dst)?lineCount:&i);
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
				//subContext.staticFlag = 1;
				context->staticFlag = 1;
				break;


			case(k_redir):
				writeObj(dst, CONT, 0, dictionary, LC);
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
				writeObj(dst, PUSH, DC[0], dictionary, LC);
				writeObj(dst, ADD, 0, dictionary, LC);
				break;


			case(k_load):
				stackPush(operationStack, LOAD);
				break;

			case(k_r14):
				writeObj(dst, R14, 0, dictionary, LC);
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

				writeObj(dst, DATA, 0, dictionary, LC);		//space for function pointer

				//get return type
				charPtr2 = strtok(NULL, "():");
				writeObj(dst, DATA, charPtr2[0], dictionary, LC);

				//get argument information
				charPtr2 = strtok(NULL, "():");
				writeObj(dst, DATA, strlen(charPtr2), dictionary, LC);	//write argument count
				
				for(i=0;i<strlen(charPtr2);i++) {
					writeObj(dst, DATA, (long) charPtr2[i], dictionary, LC);
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
						if(!context->literalFlag) {
							writeObj(dst, PUSH, *(long *) &tempFloat, dictionary, LC);
						} else {
							if(dst) {
								writeObj(dst, DATA, *(long *) &tempFloat, dictionary, LC);
							}
						}
					} else {
						if(!context->literalFlag) writeObj(dst, PUSH, atol(tok), dictionary, LC);
						else writeObj(dst, DATA, atol(tok), dictionary, LC);	//push its value
					}
				} else {
					writeAddressCalculation(dst, tok, symbols, dictionary, LC, context, lineCount);
				}
				break;
		}
	}
	
	stackFree(operationStack);

	return *LC - oldLC;
}


int writeAddressCalculation(FILE *dst, char *token, Table *symbols, translation *dictionary, int *LC, Context *context, int *lineCount) {
	//this function figures out what address a non-keyword token should correlate to
	//and writes that address to the output file
	int oldLC = *LC;

	int fakeLC = 0;
	Table *sym = tableLookup(symbols, token, &fakeLC);
	
	if(sym == NULL) {	//does the symbol not exist yet?
		int offset = (context->instructionFlag && !context->literalFlag)? 5: 0;	//TODO: replace that 6 with a means of figuring out the grab offset per translation
		/*if(dst)*/ tableAddSymbol(symbols, token, *LC+offset, context->staticFlag, context->parameterFlag); //you have to do this with no dst, otherwise fake-compiled sections will grab instead of rpush later
		sym = tableLookup(symbols, token, &fakeLC);
		if(!context->parameterFlag) {
			//This is an implicitly declared variable
			if(dst) printf("%d:\tImplicitly declared symbol: %s:%x, %d\n", *lineCount, sym->token, sym->val, sym->staticFlag);
			if(context->publicFlag) publicize(sym);
			if(!context->literalFlag && context->instructionFlag) writeObj(dst, GRAB, 0, dictionary, LC);
			else writeObj(dst, DATA, 0, dictionary, LC);
		} else {
			//this is a parameter declaration, count it and carry on
			(*LC) += WRDSZ;
		}
		return *LC - oldLC;
	}

	int value = sym->val + fakeLC;
	
	if(sym->parameterFlag) {
		writeObj(dst, AGET, value + WRDSZ, dictionary, LC);
	} else {
		if(!sym->staticFlag) {
			if(!fakeLC) {
				//symbol is not being called for within a collection
				//if(dst) printf("%d:\t%s, to dynamic, no transversal\n", *lineCount, sym->token);
				if(!context->literalFlag) {
					writeObj(dst, RPUSH, value - *LC - dictionary[RPUSH].length + 1, dictionary, LC);
				} else {
					writeObj(dst, DATA, value - *LC + 1, dictionary, LC);
				}
			} else {
				//if(dst) printf("%d:\t%s to static from dynamic\n", *lineCount, sym->token);
				if(dst) printf("%d:\there for %s\tsym = %lx\n", *lineCount, sym->token, sym->val);
				writeObj(dst, PUSH, sym->val, dictionary, LC);
				writeObj(dst, LOC, 0, dictionary, LC);
				/*
				writeObj(dst, RPUSH, - (*LC + dictionary[RPUSH].length) + 1, dictionary, LC);
				writeObj(dst, PUSH, sym->val, dictionary, LC);
				writeObj(dst, ADD, 0, dictionary, LC);
				*/
			}
		} else {
			//if(dst) printf("%d:\t%s to static from somewhere\n", *lineCount, sym->token);
			if(!context->literalFlag) {
				writeObj(dst, RPUSH, - (*LC +dictionary[RPUSH].length) + 1, dictionary, LC);
				writeObj(dst, PUSH, value, dictionary, LC);
				writeObj(dst, ADD, 0, dictionary, LC);
			} else {
				writeObj(dst, DATA, value, dictionary, LC);
			}
		}
	}
	return *LC - oldLC;
}

void fillOperations(FILE *dst, int *LC, Stack *operationStack, translation *dictionary) {
	//unstacks operators
	long op;

	while((op = stackPop(operationStack))) {
		if(op == -1) break;
		writeObj(dst, op, 0, dictionary, LC);
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
	tableAddSymbol(ret, ">>", k_shift, 0, 0);
	tableAddSymbol(ret, "=", k_is, 0, 0);
	tableAddSymbol(ret, "=>", k_isr, 0, 0);
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
	tableAddSymbol(ret, "r14", k_r14, 0, 0);

	return ret;
}

translation *prepareTranslation() {
	translation *ret = translationCreate(); 
	translationAdd(ret, HALT, c_halt, -1, 0); 
	translationAdd(ret, JMP, c_jmp, -1, 0); 
	translationAdd(ret, HOP, c_hop, -1, 0); 
	translationAdd(ret, BRN, c_brn, -1, 0); 
	translationAdd(ret, BNE, c_bne, -1, 0); 
	translationAdd(ret, NTV, c_ntv, -1, 0); 
	translationAdd(ret, LOC, c_loc, -1, 0); 
	translationAdd(ret, PRNT, c_prnt, -1, 0);
	translationAdd(ret, PUSH, c_push, 2, 0); 
	translationAdd(ret, RPUSH, c_rpush, 3, 1); 
	translationAdd(ret, GRAB, c_grab, -1, 0); 
	translationAdd(ret, POP, c_pop, -1, 0); 
	translationAdd(ret, RPOP, c_rpop, -1, 0);
	translationAdd(ret, BPOP, c_bpop, -1, 0); 
	translationAdd(ret, CONT, c_cont, -1, 0); 
	translationAdd(ret, CLR, c_clr, -1, 0); 
	translationAdd(ret, JSR, c_jsr, -1, 0); 
	translationAdd(ret, RSR, c_rsr, -1, 0); 
	translationAdd(ret, APUSH, c_apush, -1, 0); 
	translationAdd(ret, AGET, c_aget, 2, 0);
	translationAdd(ret, ADD, c_add, -1, 0);
	translationAdd(ret, SUB, c_sub, -1, 0);
	translationAdd(ret, MUL, c_mul, -1, 0);
	translationAdd(ret, DIV, c_div, -1, 0);
	translationAdd(ret, MOD, c_mod, -1, 0);
	translationAdd(ret, AND, c_and, -1, 0);
	translationAdd(ret, OR, c_or, -1, 0);
	translationAdd(ret, NOT, c_not, -1, 0);
	translationAdd(ret, PRTS, c_prts, -1, 0);
	translationAdd(ret, GT, c_gt, -1, 0);
	translationAdd(ret, LT, c_lt, -1, 0);
	translationAdd(ret, EQ, c_eq, -1, 0);
	translationAdd(ret, ALOC, c_aloc, -1, 0);
	translationAdd(ret, NEW, c_new, -1, 0);
	translationAdd(ret, FREE, c_free, -1, 0);
	translationAdd(ret, LOAD, c_load, -1, 0);
	translationAdd(ret, DATA, c_data, 0, 0);
	translationAdd(ret, R14, c_r14, -1, 0);
	
	return ret;
}
