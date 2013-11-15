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
	char string[256];		//additional string space
	char *charPtr, *charPtr2, *charPtr3;
	long nameAddr;			//for marking important working addresses
	long DC[3];			//data counters
	int i;				//iterator
	double tempFloat;		//for working floating point values
	long tokVal;			//value of current token
	int tokLen;			//length of current token
	Table *tempTable;		//for working symbol tables
	Stack *operationStack = stackCreate(32);	//for stacking operators

	while(!endOfStatement && !C->end) {
		getToken(C, tok);
//printf("token:\t%s\n", tok);
		if(!tok[0] && C->dst) {
			if(C->dst) printf("%d: Expected } symbol.\n", C->lineCounter);
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
				//compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, lineCount);
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
					writeObj(C, APUSH, 0);
				}
				endOfStatement = 1;
				break;


			case(k_prnt):
				fillOperations(C,operationStack);
				stackPush(operationStack, PRNT);
				break;


			case(k_prtf):
				fillOperations(C,operationStack);
				stackPush(operationStack, PRTF);
				break;


			case(k_prtc):
				fillOperations(C, operationStack);
				stackPush(operationStack, PRTC);
				break;


			case(k_prts):
				fillOperations(C, operationStack);
				stackPush(operationStack, PRTS);
				break;


			case(k_goto):
				stackPush(operationStack, JMP);
				break;


			case(k_singleQuote):
				tokLen = getToken(C, tok);	//need to do something to continue counting tokLen in new modular code
				writeObj(C->dst, PUSH, tok[0]);
				break;


			case(k_doubleQuote):
				compileQuote(C, CO, tok);
				break;


			case(k_int):
				stackPush(operationStack, FTOD);
				break;


			case(k_char):
				stackPush(operationStack, FTOD);
				break;


			case(k_pnt):
				compileDeclaration(C, CO, tok);
				break;


			case(k_float):
				stackPush(operationStack, DTOF);
				break;


			case(k_begin):
				transferAddress = C->LC;
				CO->instructionFlag = 1;
				break;


			case(k_halt):
				fillOperations(C, operationStack);
				writeObj(C, HALT, 0);
				break;


			case(k_clr):
				writeObj(C, CLR, 0);
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
				if(!CO->parameterFlag) writeObj(C, APUSH, 0);
				break;


			case(k_cont):
				writeObj(C, CONT, 0);
				break;


			case(k_not):
				writeObj(C, NOT, 0);
				break;


			case(k_shift):
				stackPush(operationStack, SHIFT);
				break;


			case(k_is):
				fillOperations(C, operationStack);
				stackPush(operationStack, POP);
				break;

			case(k_isr):
				fillOperations(C, operationStack);
				stackPush(operationStack, RPOP);
				break;

			case(k_set):
				fillOperations(C, operationStack);
				stackPush(operationStack, BPOP);
				break;


			case(k_eq):
				fillOperations(C, operationStack);
				stackPush(operationStack, EQ);
				break;


			case(k_gt):
				fillOperations(C, operationStack);
				stackPush(operationStack, GT);
				break;


			case(k_lt):
				fillOperations(C, operationStack);
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
				stackPush(operationStack, ALOC);
				break;


			case(k_new):
				stackPush(operationStack, NEW);
				break;


			case(k_free):
				stackPush(operationStack, FREE);
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
				printf("LC: %lx\t_LC: %lx\n", C->LC, _C.LC);
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
				stackPush(operationStack, LOAD);
				break;

			case(k_imply):
				stackPush(operationStack, IMPL);
				break;

			case(k_create):
				stackPush(operationStack, CREATE);
				break;

			case(k_nativeFunction):
				compileNativeStructure(C, CO, tok);
				break;

			case(k_anon):
				CO->anonFlag = 1;
				compileAnonSet(C, CO, tok);
				CO->anonFlag = 0;
				break;

			default:
				compileAtom(C, CO, tok);
				break;
		}
	}
	
	stackFree(operationStack);
	if(tokVal == -1) writeObj(C, HALT, 0);

	return C->LC - begin;
}

/*
	writeAddressCalculation is a function that is used to write the address calculation of a user-defined symbol
	encountered by compileStatement. If this symbol has never been encountered before, it creates it as a new
	symbol in the current scope.
*/
long writeAddressCalculation(Compiler *C, Context *CO, char *tok) {
	//this function figures out what address a non-keyword token should correlate to
	//and writes that address to the output file
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);

	int acc = 0;
	Table *sym = tableLookup(CO->symbols, tok, &acc);
	
	if(sym == NULL) {	//does the symbol not exist yet?
		int offset = (CO->instructionFlag && !CO->literalFlag)? 5: 0;	//TODO: replace that 6 with a means of figuring out the grab offset per translation
		/*if(dst)*/ tableAddSymbol(CO->symbols, tok, C->LC + offset, CO); //you have to do this with no dst, otherwise fake-compiled sections will grab instead of rpush later
		sym = tableLookup(CO->symbols, tok, &acc);
		if(!CO->parameterFlag) {
			//This is an implicitly declared variable
			if(CO->anonFlag) {
				writeObj(C, AGET, 0);	//this will be changed to the correct parameter value the second time through (once the symbol table knows)
				stackPush(C->anonStack, sym);
			} else {
				if(C->dst && !CO->parameterFlag) printf("%d:\tImplicitly declared symbol: %s:%x, %d\n", C->lineCounter, sym->token, sym->val, sym->staticFlag);
				if(CO->publicFlag && C->dst && !CO->anonFlag) publicize(sym);
				if(!CO->literalFlag && CO->instructionFlag) writeObj(C, GRAB, 0);
				else writeObj(C, DATA, 0);
			}

		} else {
			//this is a parameter declaration, count it and carry on
			C->LC += WRDSZ;
		}
		return C->LC - begin;
	}

	int value = sym->val + acc;
	
	if(sym->parameterFlag) {
		writeObj(C, AGET, sym->val + WRDSZ);
	} else {
		if(!sym->staticFlag) {
			if(!acc) {
				//symbol is not being called for within a collection
				//if(dst) printf("%d:\tto child symbol %s. Val = %x, Offset = %x\n", *lineCount, sym->token, sym->val, sym->offset);
				if(!CO->literalFlag) {
					writeObj(C, RPUSH, value - C->LC - C->dictionary[RPUSH].length + 1 + sym->offset);
				} else {
					writeObj(C, DATA, value - C->LC + 1);
				}
			} else {
				//if(dst) printf("%d:\tto parent symbol %s. Val = %x, Offset = %x, Backset = %x\n", *lineCount, sym->token, sym->val, sym->offset, fakeLC);
				writeObj(C, RPUSH, - (C->LC + acc + C->dictionary[RPUSH].length) + 1);
				writeObj(C, PUSH, sym->val + sym->offset);
				writeObj(C, ADD, 0);
			}
		} else {
			//if(dst) printf("%d:\t%s to static from somewhere\n", *lineCount, sym->token);
			if(!CO->literalFlag) {
				writeObj(C, RPUSH, - (C->LC + C->dictionary[RPUSH].length) + 1);
				writeObj(C, PUSH, value);
				writeObj(C, ADD, 0);
			} else {
				writeObj(C, DATA, value);
			}
		}
	}
	return C->LC - begin;
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
	tableAddSymbol(ret, "{{", k_anonSet, &CO);
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
	tableAddSymbol(ret, "char", k_char, &CO);
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
	tableAddSymbol(ret, "<-", k_imply, &CO);
	tableAddSymbol(ret, "create", k_create, &CO);
	tableAddSymbol(ret, ":", k_anon, &CO);

	return ret;
}

/*
	prepareTranslation is a function that returns a translation dictionary between the Eesk Intermediate Representation
	and the machine code of the target platform. This dictionary is later used by a writer to do cross-assembly before
	writing to the output file.
*/
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
	translationAdd(ret, POPTO, c_popto, 3, 0);
	translationAdd(ret, RPUSH, c_rpush, 3, 1); 
	translationAdd(ret, GRAB, c_grab, 5, 0); 
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
	
	return ret;
}
