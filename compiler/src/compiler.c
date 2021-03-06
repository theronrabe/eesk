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
#include <compilerStates.h>
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
#include <context.h>

/*
	This function is our recursive finite state machine. It switches to a state (which writes instructions through the writer,
	or in some way alters the context) according to an incoming symbol from the tokenizer.

	TODO:
		- Each state really should be made into a seperate function for the sake of modularity. This is getting pretty
		ridiculous

*/
long compileStatement(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);

	int endOfStatement = 0;		//tells us whether or not to unstack operators
	//char string[256];		//additional string space
	//char *charPtr, *charPtr2, *charPtr3;
	//long nameAddr;			//for marking important working addresses
	//long DC[3];			//data counters
	int i;				//iterator
	//double tempFloat;		//for working floating point values
	long tokVal = -1;		//value of current token
	//int tokLen;			//length of current token
	Table *tempTable;		//for working symbol tables
	Stack *operationStack = stackCreate(128);	//for stacking operators

	while(!endOfStatement && !C->end) {
		if(getToken(C, tok) == -1) { continue; }				//grab next token to parse
		if(CO->verboseFlag) printf("token:\t%s\n", tok);			//print it, if verbose mode
		if(!tok[0]/* && C->dst*/) {
			if(C->dst) printf("%d: Expected } symbol.\n", C->lineCounter);
			C->end = 1;
			continue;
			/*
			writeObj(C, HALT, 0);
			stackFree(operationStack);
			return C->LC - begin;
			*/
			//exit(0);
		}
		tempTable = tableLookup(C->keyWords, tok, &(_C.LC));
		if(tempTable) {
			tokVal = tempTable->val;
		} else {
			tokVal = -1;
		}

		switch(tokVal) {
			case(k_if):
				compileIf(C, CO, tok);
				break;

			case(k_while):
				compileWhile(C, CO, tok);
				break;

			case(k_Function):
				compileSet(C, CO, tok);
				break;

			case(k_oBrace):
				//This seemingly important symbol literally does nothing.
				break;


			case(k_cBrace):
				fillOperations(C, operationStack);
				_CO.publicFlag = 0;
				CO->publicFlag = 0;
				_CO.nativeFlag = 0;
				CO->nativeFlag = 0;
				_CO.staticFlag = 0;
				CO->staticFlag = 0;
				endOfStatement = 1;
				break;


			case(k_oParen):
				_CO.instructionFlag = 1;
				compileStatement(C, &_CO, tok);
				_CO.instructionFlag = CO->instructionFlag;
				break;


			case(k_cParen):
				fillOperations(C, operationStack);
				endOfStatement = 1;
				break;


			case(k_oBracket):
				if(!CO->nativeFlag) {
					compileCall(C, CO, tok);
				} else {
	//printf("here...? nativeFlag = %x\n", CO->nativeFlag);
					compileNative(C, CO, tok);
				}
				break;

			case(k_cBracket):
				fillOperations(C, operationStack);
				if(C->LC - begin) {
					if(!CO->swapFlag) writeObj(C, APUSH, 0);
				}
				endOfStatement = 1;
				break;


			case(k_prnt):
				fillOperations(C,operationStack);
				stackPush(operationStack, (long *) PRNT);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_prtf):
				fillOperations(C,operationStack);
				stackPush(operationStack, (long *) PRTF);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_prtc):
				fillOperations(C, operationStack);
				stackPush(operationStack, (long *) PRTC);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_prts):
				fillOperations(C, operationStack);
				stackPush(operationStack, (long *) PRTS);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_goto):
				stackPush(operationStack, (long *) JMP);
				break;


			case(k_singleQuote):
				/*tokLen = */getToken(C, tok);	//need to do something to continue counting tokLen in new modular code
				writeObj(C, PUSH, tok[0]);
				if(CO->typingFlag) writeObj(C, TPUSH, 0);
				break;


			case(k_doubleQuote):
				compileQuote(C, CO, tok);
				if(CO->typingFlag) writeObj(C, TPUSH, 0);
				break;


			case(k_int):
				stackPush(operationStack, (long *) FTOD);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				break;


			case(k_char):
				stackPush(operationStack, (long *) CHAR);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				break;


			case(k_pnt):
				compileDeclaration(C, CO, tok);
				break;


			case(k_float):
				stackPush(operationStack, (long *) DTOF);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				break;


			case(k_begin):
				transferAddress = C->LC;
				CO->instructionFlag = 1;
				if(!CO->typingFlag) writeObj(C, TPUSH, 1);
				break;


			case(k_halt):
				fillOperations(C, operationStack);
				writeObj(C, HALT, 0);
				break;


			case(k_clr):
				writeObj(C, CLR, 0);
				if(CO->typingFlag) writeObj(C, TPOP, 0);
				break;


			case(k_endStatement):
				//unstack operators and reset flags
				fillOperations(C, operationStack);
				_CO.publicFlag = 0;
				CO->publicFlag = 0;
				_CO.nativeFlag = 0;
				CO->nativeFlag = 0;
				_CO.staticFlag = 0;
				CO->staticFlag = 0;
				break;

			case(k_argument):
				fillOperations(C, operationStack);
				if(!CO->parameterFlag) { writeObj(C, APUSH, 0); if(CO->typingFlag) writeObj(C, TPOP, 0); }
				break;


			case(k_cont):
				writeObj(C, CONT, 0);
				break;


			case(k_not):
				writeObj(C, NOT, 0);
				if(CO->typingFlag) writeObj(C, TVAL, 0);
				break;


			case(k_shift):
				stackPush(operationStack, (long *) SHIFT);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				break;


			case(k_is):
				//TODO: take care of assigning symbol type
				fillOperations(C, operationStack);
				stackPush(operationStack, (long *) POP);
				break;

			case(k_isr):
				//TODO: take care of assigning symbol type
				fillOperations(C, operationStack);
				stackPush(operationStack, (long *) RPOP);
				break;

			case(k_set):
				fillOperations(C, operationStack);
				stackPush(operationStack, (long *) BPOP);
				break;


			case(k_eq):
				fillOperations(C, operationStack);
				stackPush(operationStack, (long *) EQ);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_gt):
				fillOperations(C, operationStack);
				stackPush(operationStack, (long *) GT);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_lt):
				fillOperations(C, operationStack);
				stackPush(operationStack, (long *) LT);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_add):
				stackPush(operationStack, (long *) ADD);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_sub):
				stackPush(operationStack, (long *) SUB);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_mul):
				stackPush(operationStack, (long *) MUL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_div):
				stackPush(operationStack, (long *) DIV);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_mod):
				stackPush(operationStack, (long *) MOD);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_and):
				stackPush(operationStack, (long *) AND);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_or):
				stackPush(operationStack, (long *) OR);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_fadd):
				stackPush(operationStack, (long *) FADD);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_fsub):
				stackPush(operationStack, (long *) FSUB);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_fmul):
				stackPush(operationStack, (long *) FMUL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_fdiv):
				stackPush(operationStack, (long *) FDIV);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_return):
				stackPush(operationStack, (long *) RSR);
				break;


			case(k_public):
				_CO.publicFlag = 1;
				CO->publicFlag = 1;
				break;


			case(k_private):
				_CO.publicFlag = 0;
				CO->publicFlag = 0;
				break;


			case(k_literal):
				_CO.literalFlag = !CO->literalFlag;
				CO->literalFlag = !CO->literalFlag;
				break;

			case(k_alloc):
				stackPush(operationStack, (long *) ALOC);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				break;


			case(k_new):
				//TODO: this should be made type-safe
				stackPush(operationStack, (long *) NEW);
				break;


			case(k_free):
				stackPush(operationStack, (long *) FREE);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;


			case(k_include):
				getQuote(C, tok);
				strcpy(tok, "");
				strcat(tok, "include/");
				getQuote(C, &tok[8]);			//to accommodate for the beginning "include/"
				strcat(tok, ".ee");
				if(C->dst) printf("%d:\tIncluding file: %s\n", C->lineCounter, tok);
				char *inc = loadFile(tok);
				trimComments(inc);

				subCompiler(C, &_C);
				_C.src = inc;
				_C.SC = 0;
				C->LC += compileStatement(&_C, CO, tok);
				free(inc);
				break; 


			case(k_native):
				_CO.nativeFlag = 1;
				CO->nativeFlag = 1;
				break;



			case(k_label):
				//create a symbol referring to this location
				getToken(C, tok);
				tempTable = tableAddSymbol(CO->symbols, tok, C->LC, CO);
				if(CO->publicFlag) publicize(tempTable);
				break;


			case(k_static):
				//subContext.staticFlag = 1;
				CO->staticFlag = 1;
				break;


			case(k_redir):
				compileRedirection(C, CO, tok);
				break;

			case(k_load):
				stackPush(operationStack, (long *) LOAD);
				break;

			case(k_imply):
				stackPush(operationStack, (long *) IMPL);
				break;

			case(k_create):
				stackPush(operationStack, (long *) CREATE);
				//TODO: push a TPUSH 1 instruction
				break;

			case(k_nativeFunction):
				compileNativeStructure(C, CO, tok);
				if(CO->typingFlag) writeObj(C, TPUSH, 0);
				break;

			case(k_anon):
				i = CO->anonFlag;
				CO->anonFlag = 1;
				compileAnonSet(C, CO, tok);
				CO->anonFlag = i;
				break;

			case(k_backset):
				compileBackset(C, CO, tok);
				break;

			case(k_store):
				writeObj(C, STORE, 0);
				//TODO: assign symbol type as value
				break;

			case(k_restore):
				stackPush(operationStack, (long *) RESTORE);
				if(CO->typingFlag) stackPush(operationStack, (long *) TPOP);
				break;

			case(k_typeof):
				//stackPush(operationStack, TPOP);
				if(CO->typingFlag) stackPush(operationStack, (long *) TVAL);
				if(CO->typingFlag) stackPush(operationStack, (long *) TYPEOF);
				break;

			case(k_eval):
				//stackPush(operationStack, JSR);
				stackPush(operationStack, (long *) EVAL);
				break;

			default:
				compileAtom(C, CO, tok);
				break;
		}
	}
	
	fillOperations(C, operationStack);
	stackFree(operationStack);
	//if(tokVal == -1) writeObj(C, HALT, 0);

	return C->LC - begin;
}

/*
	writeAddressCalculation is a function that is used to write the address calculation of a user-defined symbol
	encountered by compileStatement. If this symbol has never been encountered before, it creates it as a new
	symbol in the current scope, according to the current context.
*/
long writeAddressCalculation(Compiler *C, Context *CO, char *tok) {
	char display = CO->displaySymbols;
	//this function figures out what address a non-keyword token should correlate to
	//and writes that address to the output file
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);

	long acc = -1;
	Table *sym = tableLookup(CO->symbols, tok, &acc);
	
	if(sym == NULL || CO->parameterFlag) {	//does the symbol not exist yet? Always add a new symbol for parameterFlagged expressions
		int offset = (CO->instructionFlag && !CO->literalFlag)? 5: 0;	//TODO: replace that constant with a means of figuring out the grab offset per translation
		/*if(dst)*/ tableAddSymbol(CO->symbols, tok, C->LC + offset, CO); //you have to do this with no dst, otherwise fake-compiled sections will grab instead of rpush later
		sym = tableLookup(CO->symbols, tok, &acc);
		//if(!CO->parameterFlag) {
			//This is an implicitly declared variable
			if(CO->anonFlag) {
				if(display) printf("declaring anonymous: %s\n", sym->token);
				writeObj(C, ABASE, 10);	//this is a placeholder for the sake of length-counting
				writeObj(C, AGET, 0);	//this will be changed to the correct parameter value the second time through (once the symbol table knows)
				sym->parameterFlag = 1;
				/*if(C->dst)*/ stackPush(C->anonStack, (long *) sym);
			} else {
				if(display) printf("declaring non anonymous: %s\n", sym->token);
				if(C->dst && !CO->parameterFlag) printf("%d:\tImplicitly declared symbol: %s:%lx, %d\n", C->lineCounter, sym->token, sym->val, sym->staticFlag);
				if(CO->publicFlag /*&& C->dst*/ && !CO->anonFlag) publicize(sym);
				if(!CO->literalFlag && CO->instructionFlag) { writeObj(C, GRAB, 0); if (CO->typingFlag) writeObj(C, TSYM, 0); }
				else writeObj(C, DATA, 0);
			}
		//} else {
			//this is a parameter declaration, count it and carry on
			//C->LC += WRDSZ;
		//}
		return C->LC - begin;
	}

	int value = sym->val;// + acc;
	if(acc != -1) value += acc;

	if(display) printf("recognizing symbol: %s\n\t", sym->token);
	
	if(sym->parameterFlag) {
		writeObj(C, ABASE, (CO->swapDepth+1)*0x10);
		writeObj(C, AGET, sym->val + WRDSZ);
		if(display) printf("is a parameter\n");
		if(CO->typingFlag) writeObj(C, TPUSH, 0);
	} else {
		if(display) printf("is not a parameter, ");
		if(!sym->staticFlag) {
			if(display) printf("is not static, ");
			if(acc == -1) { //!acc && sym->val) {
				if(display) printf("is not accumulated, ");
				//symbol is not being called for within a collection
				//if(dst) printf("%d:\tto child symbol %s. Val = %x, Offset = %x\n", *lineCount, sym->token, sym->val, sym->offset);
				if(!CO->literalFlag) {
					if(display) printf("is not literal, ");
					writeObj(C, RPUSH, value - C->LC - C->dictionary[RPUSH].length + 1 + sym->offset);
					if(sym->type) {
						if(display) printf("is a Set.");
						if(CO->typingFlag) writeObj(C, TPUSH, 1);
					} else {
						if(display) printf("unkown type.");
						if(CO->typingFlag) writeObj(C, TSYM, 0);
					}
				} else {
					if(display) printf("is literal, ");
					writeObj(C, DATA, value - C->LC + 1);
				}
			} else {
				if(display) printf("is accumulated %lx, ", acc);
				//if(C->dst) printf("\t%lx: to parent symbol %s. Val = %x, Offset = %x", C->LC, sym->token, sym->val, sym->offset);
				writeObj(C, RPUSH, - (C->LC + acc + C->dictionary[RPUSH].length) + 1);
				writeObj(C, PUSH, sym->val + sym->offset);
				writeObj(C, ADD, 0);
				if(sym->type) {
					if(display) printf("is a Set.");
					if(CO->typingFlag) writeObj(C, TPUSH, 1);
				} else {
					if(display) printf("unkown type.");
					if(CO->typingFlag) writeObj(C, TSYM, 0);
				}
			}
		} else {
			if(display) printf("is static, ");
			//if(dst) printf("%d:\t%s to static from somewhere\n", *lineCount, sym->token);
			if(!CO->literalFlag) {
				if(display) printf("is not literal, ");
				writeObj(C, RPUSH, - (C->LC + C->dictionary[RPUSH].length) + 1);
				writeObj(C, PUSH, value);
				writeObj(C, ADD, 0);
			} else {
				if(display) printf("is literal, ");
				writeObj(C, DATA, value);
			}
		}
	}
	if(display) printf("\n");
	return C->LC - begin;
}

/*
	compilerCreate returns a brand new Compiler
*/
Compiler *compilerCreate(char *src, long LC) {
	Compiler *C = (Compiler *) malloc(sizeof(Compiler));
	Context *CO = contextNew(NULL, 0, 0, 0, 0, 0, 0);
	C->dictionary = prepareTranslation(CO);
	C->dst = fopen("j.out", "w");
	C->src = src;
	C->SC = 0;
	C->LC = LC;
	C->lineCounter = 0;
	C->keyWords = prepareKeywords();
	C->end = 0;
	C->anonStack = stackCreate(32);
	contextDestroy(CO);

	return C;
}

/*
	compilerDestroy destroys a Compiler
*/
void compilerDestroy(Compiler *C) {
	translationFree(C->dictionary);
	tableDestroy(C->keyWords);
	stackFree(C->anonStack);
	fclose(C->dst);
	free(C);
}

/*
	subCompiler makes a second Compiler have all the same values as a first Compiler.
*/
void subCompiler(Compiler *C1, Compiler *C2) {
	C2->lineCounter = C1->lineCounter;
	C2->dst = C1->dst;
	C2->src = C1->src;
	C2->SC = C1->SC;
	C2->LC = C1->LC;
	C2->end = C1->end;
	C2->keyWords = C1->keyWords;
	C2->dictionary = C1->dictionary;
	C2->anonStack = C1->anonStack;
}

/*
	subContext copies all the values from one Context to another.
*/
void subContext(Context *C1, Context *C2) {
	C2->publicFlag = C1->publicFlag;
	C2->literalFlag = C1->literalFlag;
	C2->nativeFlag = C1->nativeFlag;
	C2->staticFlag = C1->staticFlag;
	C2->parameterFlag = C1->parameterFlag;
	C2->instructionFlag = C1->instructionFlag;
	C2->anonFlag = C1->anonFlag;
	C2->symbols = C1->symbols;
	C2->typingFlag = C1->typingFlag;
	C2->displaySymbols = C1->displaySymbols;
	C2->verboseFlag = C1->verboseFlag;
	C2->swapDepth = C1->swapDepth;
	C2->swapFlag = C1->swapFlag;
}

/*
	fillOperations is used to represent the application of all accumulated operators (as when
	a ; is used). It uses the writer to produce each operator's (from the operatorStack)
	machine code in the output file.
*/
void fillOperations(Compiler *C, Stack *operationStack) {
	//unstacks operators
	long op;

	while((op = stackPop(operationStack))) {
		if(op == -1) break;
		writeObj(C, op, 0);
	}
}

/*
	prepareKeywords returns a new symbolTable the associates language keywords to different states
	of compileStatement.
*/
Table *prepareKeywords() {
	Table *ret = tableCreate();
	Context CO;
		CO.staticFlag = 0;
		CO.parameterFlag = 0;

	//Language keywords
	tableAddSymbol(ret, "if", k_if, &CO);
	tableAddSymbol(ret, "while", k_while, &CO);
	tableAddSymbol(ret, "Function", k_Function, &CO);
	tableAddSymbol(ret, "[", k_oBracket, &CO);
	tableAddSymbol(ret, "]", k_cBracket, &CO);
	tableAddSymbol(ret, "{", k_oBrace, &CO);
	tableAddSymbol(ret, "}", k_cBrace, &CO);
	tableAddSymbol(ret, "(", k_oParen, &CO);
	tableAddSymbol(ret, ")", k_cParen, &CO);
	tableAddSymbol(ret, "printd", k_prnt, &CO);
	tableAddSymbol(ret, "printf", k_prtf, &CO);
	tableAddSymbol(ret, "printc", k_prtc, &CO);
	tableAddSymbol(ret, "prints", k_prts, &CO);
	tableAddSymbol(ret, "goto", k_goto, &CO);
	tableAddSymbol(ret, "\'", k_singleQuote, &CO);
	tableAddSymbol(ret, "\"", k_doubleQuote, &CO);
	tableAddSymbol(ret, "int", k_int, &CO);
	//tableAddSymbol(ret, "char", k_char, &CO);
	tableAddSymbol(ret, "given", k_pnt, &CO);
	tableAddSymbol(ret, "float", k_float, &CO);
	tableAddSymbol(ret, "Begin", k_begin, &CO);
	tableAddSymbol(ret, "End", k_halt, &CO);
	tableAddSymbol(ret, ",", k_argument, &CO);
	tableAddSymbol(ret, "^", k_clr, &CO);
	tableAddSymbol(ret, ";", k_endStatement, &CO);
	tableAddSymbol(ret, "$", k_cont, &CO);
	tableAddSymbol(ret, "!", k_not, &CO);
	tableAddSymbol(ret, ">>", k_shift, &CO);
	tableAddSymbol(ret, "=", k_is, &CO);
	tableAddSymbol(ret, "=>", k_isr, &CO);
	tableAddSymbol(ret, "set", k_set, &CO);
	tableAddSymbol(ret, "==", k_eq, &CO);
	tableAddSymbol(ret, ">", k_gt, &CO);
	tableAddSymbol(ret, "<", k_lt, &CO);
	tableAddSymbol(ret, "+", k_add, &CO);
	tableAddSymbol(ret, "-", k_sub, &CO);
	tableAddSymbol(ret, "*", k_mul, &CO);
	tableAddSymbol(ret, "/", k_div, &CO);
	tableAddSymbol(ret, "%", k_mod, &CO);
	tableAddSymbol(ret, "&", k_and, &CO);
	tableAddSymbol(ret, "|", k_or, &CO);
	tableAddSymbol(ret, "++", k_fadd, &CO);
	tableAddSymbol(ret, "--", k_fsub, &CO);
	tableAddSymbol(ret, "**", k_fmul, &CO);
	tableAddSymbol(ret, "//", k_fdiv, &CO);
	tableAddSymbol(ret, "return", k_return, &CO);
	tableAddSymbol(ret, "public", k_public, &CO);
	tableAddSymbol(ret, "private", k_private, &CO);
	tableAddSymbol(ret, "\\", k_literal, &CO);
	tableAddSymbol(ret, "Collection", k_collect, &CO);
	tableAddSymbol(ret, "Field", k_field, &CO);
	tableAddSymbol(ret, "alloc", k_alloc, &CO);
	tableAddSymbol(ret, "new", k_new, &CO);
	tableAddSymbol(ret, "free", k_free, &CO);
	tableAddSymbol(ret, "include", k_include, &CO);
	tableAddSymbol(ret, "@", k_native, &CO);
	tableAddSymbol(ret, "define", k_label, &CO);
	tableAddSymbol(ret, "->", k_redir, &CO);
	tableAddSymbol(ret, "load", k_load, &CO);
	tableAddSymbol(ret, "Native", k_nativeFunction, &CO);
	tableAddSymbol(ret, "Set", k_Function, &CO);
	//tableAddSymbol(ret, "<-", k_imply, &CO);
		tableAddSymbol(ret, "<-", k_backset, &CO);
	tableAddSymbol(ret, "create", k_create, &CO);
	tableAddSymbol(ret, ":", k_store, &CO);
	tableAddSymbol(ret, "...", k_restore, &CO);
	tableAddSymbol(ret, "`", k_anon, &CO);
	tableAddSymbol(ret, "char", k_char, &CO);
	tableAddSymbol(ret, "isSet", k_typeof, &CO);
	tableAddSymbol(ret, "read", k_eval, &CO);

	return ret;
}

/*
	prepareTranslation is a function that returns a translation dictionary between the Eesk Intermediate Representation
	and the machine code of the target platform. This dictionary is later used by a writer to do cross-assembly before
	writing to the output file.
*/
translation *prepareTranslation(Context *CO) {
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
	translationAdd(ret, POPTO, c_popto, 3, 0);
	translationAdd(ret, RPUSH, c_rpush, 3, 1); 
	translationAdd(ret, GRAB, c_grab, 5, 0); 
	translationAdd(ret, POP, c_pop, -1, 0); 
	translationAdd(ret, RPOP, c_rpop, -1, 0);
	translationAdd(ret, BPOP, c_bpop, -1, 0); 
	translationAdd(ret, CONT, c_cont, -1, 0); 
	translationAdd(ret, CLR, c_clr, -1, 0); 
	if(CO->typingFlag) {
		translationAdd(ret, JSR, c_tjsr, -1, 0);
		translationAdd(ret, RSR, c_trsr, -1, 0);
	} else {
		translationAdd(ret, JSR, c_jsr, -1, 0); 
		translationAdd(ret, RSR, c_rsr, -1, 0); 
	}
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
	translationAdd(ret, PRTF, c_prtf, -1, 0);
	translationAdd(ret, FADD, c_fadd, -1, 0);
	translationAdd(ret, PRTS, c_prts, -1, 0);
	translationAdd(ret, GT, c_gt, -1, 0);
	translationAdd(ret, LT, c_lt, -1, 0);
	translationAdd(ret, EQ, c_eq, -1, 0);
	translationAdd(ret, ALOC, c_aloc, -1, 0);
	translationAdd(ret, NEW, c_new, -1, 0);
	translationAdd(ret, FREE, c_free, -1, 0);
	translationAdd(ret, LOAD, c_load, -1, 0);
	translationAdd(ret, DATA, c_data, 0, 0);
	translationAdd(ret, IMPL, c_impl, -1, 0);
	translationAdd(ret, CREATE, c_create, -1, 0);
	translationAdd(ret, BKSET, c_backset, 13, 0);
	translationAdd(ret, STORE, c_store, -1, 0);
	translationAdd(ret, RESTORE, c_restore, -1, 0);
	translationAdd(ret, CHAR, c_char, -1, 0);
	translationAdd(ret, PRTC, c_prtc, -1, 0);
	translationAdd(ret, NPUSH, c_npush, -1, 0);
	translationAdd(ret, TPUSH, c_tpush, 6, 0);
	translationAdd(ret, TSYM, c_tsym, -1, 0);
	translationAdd(ret, TYPEOF, c_typeof, -1, 0);
	translationAdd(ret, TASGN, c_tasgn, 2, 0);
	translationAdd(ret, TPOP, c_tpop, -1, 0);
	translationAdd(ret, TVAL, c_tval, -1, 0);
	translationAdd(ret, TSET, c_tset, -1, 0);
	translationAdd(ret, TDUP, c_tdup, -1, 0);
	translationAdd(ret, SWAP, c_swap, -1, 0);
	translationAdd(ret, RESWAP, c_reswap, -1, 0);
	translationAdd(ret, ABASE, c_abase, 5, 0);
	translationAdd(ret, EVAL, c_eval, -1, 0);
	
	return ret;
}
