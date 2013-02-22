/*
Theron Rabe
compiler.c
2/11/2013

	This is the compiler.
	
	TODO:	introduce floats
		introduce arrays
		introduce strings of characters
		introduce private/public directive
*/
#include "compiler.h"

void main(int argc, char **argv) {
	char *src = loadFile(argv[1]);
	FILE *dst = fopen("b.out", "w");
	char token[32];
	Table *keyWords = prepareKeywords();
	Table *symbols = tableCreate();
	int SC = 0, LC = 0;
	
	callStack = stackCreate(32);
	nameStack = stackCreate(32);
	varyStack = stackCreate(128);

	trimComments(src);
	
	compileStatement(keyWords, symbols, src, &SC, dst, &LC);
//	fillVary(dst, &LC);
	writeObj(dst, transferAddress, &LC);

	fclose(dst);
}

void fillVary(FILE *dst, int *LC) {
	int addr;
	while(addr = stackPop(varyStack)) {
		if(addr == -1) break;
		writeObj(dst, VARY, LC);	writeObj(dst, addr, LC);
	}
}

int compileStatement(Table *keyWords, Table *symbols, char *src, int *SC, FILE *dst, int *LC) {
	int endOfStatement = 0;
	int oldLC = *LC;
	int fakeLC = 0;
	int fakeSC;
	char tok[256];	//needs to be big in case of string
	char prevTok[24];
	int nameAddr;
	int DC[3];	//data counters
	int i;
	int tempFloat;
	int tokVal;
	int tokLen;
	Stack *operationStack = stackCreate(32);
	Stack *braceStack = stackCreate(32);

	while(!endOfStatement && tokLen != -1) {
		tokLen = getToken(tok, src, SC);
		tokVal = tableLookup(keyWords, tok);
//printf("token:\t%s\n", tok);


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
			case(k_State):
				writeObj(dst, 0, LC);						//result address

				//in a new namespace
				nameAddr = *LC;
				stackPush(nameStack, nameAddr);
				tokLen = getToken(tok, src, SC);
				tableAddSymbol(symbols, tok, nameAddr);
				symbols = tableAddLayer(symbols);
	
				//count data section
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);
				
				writeObj(dst, nameAddr+DC[0], LC);			//name pointer to statement
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled data section
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr-1, LC);	//PUSH result address
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled statement
				writeObj(dst, POP, LC);						//store result
				writeObj(dst, JMP, LC);						//back to return address
	
				//no longer in this namespace
				stackPop(nameStack);
				symbols = tableRemoveLayer(symbols);
				break;
			case(k_VaryForm):
				writeObj(dst, BREAK, LC);	//jump here at BNE
	
				//in a new namespace
				nameAddr = *LC;
				stackPush(nameStack, nameAddr);
				stackPush(varyStack, nameAddr);
				symbols = tableAddLayer(symbols);
				
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr-1, LC);	//PUSH BNE address
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled address
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled statement
				writeObj(dst, POP, LC);						//address and result already on stack
				writeObj(dst, BREAK, LC);					//finish vary
	
				//no longer in this namespace
				stackPop(nameStack);
				symbols = tableRemoveLayer(symbols);
				break;
			case(k_Function):
				writeObj(dst, 0, LC);						//result location
	
				//in a new namespace
				nameAddr = *LC;
				stackPush(nameStack, *LC);
				tokLen = getToken(tok, src, SC);
				tableAddSymbol(symbols, tok, nameAddr);
				symbols = tableAddLayer(symbols);
	
				//count length of parameters and data sections
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//param length
				DC[1] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);	//data length
				
				writeObj(dst, nameAddr+DC[0]+DC[1]+1, LC);		//name pointer to statement
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled parameters section
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled data section
	
				//fill parameters with arguments
				for(i=DC[0]-1;i>=0;i--){
					writeObj(dst, POPTO, LC);				//POPTO each argument
					writeObj(dst, nameAddr+1+i, LC);
				}
	
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr-1, LC);	//push result address
				compileStatement(keyWords, symbols, src, SC, dst, LC);		//compiled statement section
				writeObj(dst, POP, LC);						//store result
				writeObj(dst, JMP, LC);						//goto return address
	
				//no longer in this namespace
				stackPop(nameStack);
				symbols = tableRemoveLayer(symbols);
				break;
			case(k_oBrace):
				//stackPush(braceStack, *LC);
				//compileStatement(keyWords, symbols, src, SC, dst, LC);
				break;
			case(k_cBrace):
				fillOperations(dst, LC, operationStack);
				//stackPop(braceStack);
				endOfStatement = 1; //!(braceStack->sp);
				break;
			case(k_oParen):
				//stackPush(braceStack, *LC);
				compileStatement(keyWords, symbols, src, SC, dst, LC);
				break;
			case(k_cParen):
				fillOperations(dst, LC, operationStack);
				//stackPop(braceStack);
				endOfStatement = 1;
				break;
			case(k_oBracket):
				writeObj(dst, CLR, LC);						//CLR
	
				//handle stacks
				nameAddr = tableLookup(symbols, prevTok);
				stackPush(callStack, nameAddr);
				//stackPush(braceStack, nameAddr);
	
				//count length to next }
				fakeSC = *SC;
				DC[0] = compileStatement(keyWords, symbols, src, &fakeSC, NULL, &fakeLC);		//compiled argument section

				//push return address of call
				writeObj(dst, PUSH, LC);	writeObj(dst, *LC+DC[0]+5, LC);			//push return address

				//compile arguments
				compileStatement(keyWords, symbols, src, SC, dst, LC);

				//do closing brace stuff so that k_cBrace doesn't pop global stacks, just ends statement.
				nameAddr = stackPop(callStack);
				
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push function pointer
				writeObj(dst, CONT, LC);					//turn pointer into address
				writeObj(dst, JMP, LC);						//goto call address
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr-1, LC);	//return here, put result on stack
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
			case(k_singleQuote):
				tokLen = getToken(tok, src, SC);
				writeObj(dst, PUSH, LC);	writeObj(dst, tok[0], LC);
				break;
			case(k_doubleQuote):
				writeObj(dst, PUSH, LC);	writeObj(dst, *LC+2, LC);	//push start of string

				DC[0] = getQuote(tok, src, SC);
				writeObj(dst, PUSH, LC);	writeObj(dst, *LC+DC[0], LC);	//push end of string
				writeObj(dst, JMP, LC);						//skip over string leaving it on stack
				
				for(i=0;i<DC[0];i++) {
					writeObj(dst, tok[i], LC);				//a word for each character
				}
				break;
			case(k_vary):
				writeObj(dst, VARY, LC);
				break;
			case(k_by):
				//get call address
				tokLen = getToken(tok, src, SC);
				nameAddr = tableLookup(symbols, tok);

				writeObj(dst, PUSH, LC);	writeObj(dst, *LC+5, LC);	//push return address
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr, LC);	//push call address
				writeObj(dst, CONT, LC);					//get contents from pointer
				writeObj(dst, JMP, LC);						//go to function
				writeObj(dst, PUSH, LC);	writeObj(dst, nameAddr-1, LC);	//put result on stack

				endOfStatement = 1;
				break;
			case(k_int):
				tokLen = getToken(tok, src, SC);
				if(dst) tableAddSymbol(symbols, tok, *LC);
				writeObj(dst, 0, LC);
				break;
			case(k_char):
				tokLen = getToken(tok, src, SC);
				if(dst) tableAddSymbol(symbols, tok, *LC);
				writeObj(dst, 0, LC);
				break;
			case(k_pnt):
				tokLen = getToken(tok, src, SC);
				if(dst) tableAddSymbol(symbols, tok, *LC);
				writeObj(dst, 0, LC);
				break;
			case(k_begin):
				//writeObj(dst, RUN, LC);
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
				//endOfStatement = !(braceStack->sp);
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
			default:
				//this is either a declared symbol or not
				tokVal = tableLookup(symbols, tok);

				if(tokVal == -1) {
					if(numeric(tok[0]) || (tok[0] == '-' && numeric(tok[1]))) {
						//this is a numeric literal
						if(isFloat(tok)) {
							tempFloat = atof(tok);
							writeObj(dst, PUSH, LC);	writeObj(dst, tempFloat, LC);
						} else {
							writeObj(dst, PUSH, LC);	writeObj(dst, atoi(tok), LC);	//push its value
						}
					} else {
						//this is an undeclared symbol
						tableAddSymbol(symbols, tok, *LC);
						writeObj(dst, PUSH, LC);	writeObj(dst, *LC, LC);		//push new label's address
					}
				} else {
					//this is a declared symbol
					strcpy(prevTok, tok);
					writeObj(dst, PUSH, LC);	writeObj(dst, tokVal, LC);		//push the symbol's address
				}
				break;
		}
	}
	
	stackFree(operationStack);
	stackFree(braceStack);

	return *LC - oldLC;
}

void writeObj(FILE *fn, int val, int *LC) {
	if (fn) {
		fwrite(&val, sizeof(int), 1, fn);
	//	printf("%d:%d\n", *LC, val);
	}
	(*LC)++;
}

void fillOperations(FILE *dst, int *LC, Stack *operationStack) {
	int op;

	while(op = stackPop(operationStack)) {
		if(op == -1) break;
		writeObj(dst, op, LC);
	}
}

Table *prepareKeywords() {
	Table *ret = tableCreate();

	//Language keywords
	tableAddSymbol(ret, "if", k_if);
	tableAddSymbol(ret, "while", k_while);
	tableAddSymbol(ret, "State", k_State);
	tableAddSymbol(ret, "Vary", k_VaryForm);
	tableAddSymbol(ret, "Function", k_Function);
	tableAddSymbol(ret, "[", k_oBracket);
	tableAddSymbol(ret, "]", k_cBracket);
	tableAddSymbol(ret, "{", k_oBrace);
	tableAddSymbol(ret, "}", k_cBrace);
	tableAddSymbol(ret, "(", k_oParen);
	tableAddSymbol(ret, ")", k_cParen);
	tableAddSymbol(ret, "print", k_prnt);
	tableAddSymbol(ret, "\'", k_singleQuote);
	tableAddSymbol(ret, "\"", k_doubleQuote);
	tableAddSymbol(ret, "by", k_by);
	tableAddSymbol(ret, "int", k_int);
	tableAddSymbol(ret, "char", k_char);
	tableAddSymbol(ret, "pnt", k_pnt);
	tableAddSymbol(ret, "Begin", k_begin);
	tableAddSymbol(ret, "End", k_halt);
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
	tableAddSymbol(ret, "->", k_child);

	return ret;
}
