/*
compiler.c

	This is the compiler.

	TODO:
		-structures
			DONE	-collect should store its length in a word of memory (for "new"-ing later)
			DONE	-everything after "collect" token should be addressed relative to that location
			DONE	-write calculateAddress(char *token) to solve for dot operator
			-check readme for info on implementing this with dynamic memory allocation
		-dynamic memory allocation in VM
			DONE	-"new" should push to STACK the address to a chunk of memory of size by popping STACK.
			-dynamically allocated objects need to know relative addresses
			-switch to relative addressing for the sake of calling functions in dynamically allocated memory
		-"including" other source files
			DONE
		-setable initial memory size for VM
		-cross assembler for bytecode->target_platform
		-standard library
	
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

	int endOfStatement = 0;
	int oldLC = *LC;
	int fakeLC = 0;
	int fakeSC;
	char tok[256];	//needs to be big in case of string
	long nameAddr;
	long DC[3];	//data counters
	int i;
	float tempFloat;
	long tokVal;
	int tokLen;
	Table *tempTable;
	Stack *operationStack = stackCreate(32);

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
				//writeObj(dst, CONT, LC);					//get call address
				writeObj(dst, PUSH, LC);		writeObj(dst, *LC+3, LC);	//jump over word of call data
				writeObj(dst, JMP, LC);
					nameAddr = *LC;
					writeObj(dst, 0, LC);	//storage for call address
				writeObj(dst, POPTO, LC);	writeObj(dst, nameAddr - *LC + 1, LC);
	
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);		//compiled argument section

				//push return address of call
				writeObj(dst, PUSH, LC);	writeObj(dst, *LC+DC[0]+16, LC);			//push return address

				//push result address for function
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
				writeObj(dst, CONT, LC);					//turn pointer into relative address base
				writeObj(dst, PUSH, LC);	writeObj(dst, 1, LC);		//push function pointer
				writeObj(dst, SUB, LC);						//combine base and offset to get result address

				//compile arguments
				compileStatement(keyWords, symbols, src, SC, dst, LC);

				//make call
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
				writeObj(dst, CONT, LC);					//turn pointer into relative address base
				
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
				writeObj(dst, CONT, LC);
				writeObj(dst, CONT, LC);					//turn pointer into address offset
				writeObj(dst, ADD, LC);						//combine base and offset to get call address
				writeObj(dst, JMP, LC);						//goto call address

				//push result address 
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
				writeObj(dst, CONT, LC);					//turn pointer into relative address base
				writeObj(dst, PUSH, LC);	writeObj(dst, 1, LC);		//push function pointer
				writeObj(dst, SUB, LC);						//combine base and offset to get result address
				writeObj(dst, CONT, LC);
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
				writeObj(dst, PUSH, LC);	writeObj(dst, *LC+4, LC);	//push start of string

				DC[0] = getQuote(tok, src, SC);
				writeObj(dst, PUSH, LC);	writeObj(dst, *LC+DC[0]+2, LC);	//push end of string
				writeObj(dst, JMP, LC);						//skip over string leaving it on stack
				
				for(i=0;i<DC[0];i++) {
					writeObj(dst, (int)tok[i], LC);				//a word for each character
				}
				break;


			case(k_int):
				stackPush(operationStack, FTOD);
				break;


			case(k_char):
				stackPush(operationStack, FTOD);
				break;


			case(k_pnt):
				tokLen = getToken(tok, src, SC);
				if(numeric(tok[0])) {
					DC[0] = atoi(tok);
					writeObj(dst, *LC, LC);
					for(i=0;i<DC[0];i++)
						writeObj(dst, DC[0], LC);
					tokLen = getToken(tok, src, SC);
					if(dst) {
						tempTable = tableAddSymbol(symbols, tok, *LC-DC[0]-1);
					}
				} else {
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
				fillOperations(dst, LC, operationStack);
				publicFlag = 0;
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
				getToken(tok, src, SC);
				printf("INCLUDING FILE: %s\n", tok);
				char *inc = loadFile(tok);
				trimComments(inc);
				fakeSC = 0;
				compileStatement(keyWords, symbols, inc, &fakeSC, dst, LC);
				free(inc);
				endOfStatement = 1;
				break;


			default:
				//this is either an intended symbol or a number

				if(numeric(tok[0]) || (tok[0] == '-' && numeric(tok[1]))) {
					//this is a numeric literal
					if(isFloat(tok)) {
						tempFloat = atof(tok);
						if(!literalFlag) writeObj(dst, PUSH, LC);	//writeObj(dst, tempFloat, LC);
						if(dst) {
							fwrite(&tempFloat, sizeof(long), 1, dst);
							++(*LC);
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
	if (fn) {
		fwrite(&val, sizeof(long), 1, fn);
		printf("%x:%lx\n", *LC, val);
	}
	(*LC)++;
}

int writeAddressCalculation(FILE *dst, char *token, Table *symbols, int *LC) {
	int oldLC = *LC;
	char *piece0 = strtok(token, "@");
	char *piece1 = strtok(NULL, "@");
	Table *sym = tableLookup(symbols, piece0);

	if(sym == NULL) {
		tableAddSymbol(symbols, piece0, *LC);
		sym = tableLookup(symbols, piece0);
	}

	if(piece1) {
		writeObj(dst, RPUSH, LC);	writeObj(dst, tableLookup(symbols, piece1)->val - *LC + 1, LC);
		writeObj(dst, CONT, LC);
		writeObj(dst, PUSH, LC);	writeObj(dst, sym->val, LC);
		writeObj(dst, ADD, LC);
	} else {
		if(!literalFlag)
			writeObj(dst, RPUSH, LC);
		writeObj(dst, sym->val - *LC + 1, LC);
	}

	return *LC - oldLC;
}

void fillOperations(FILE *dst, int *LC, Stack *operationStack) {
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
	tableAddSymbol(ret, "collect", k_collect);
	tableAddSymbol(ret, "alloc", k_alloc);
	tableAddSymbol(ret, "new", k_new);
	tableAddSymbol(ret, "free", k_free);
	tableAddSymbol(ret, "include", k_include);

	return ret;
}
