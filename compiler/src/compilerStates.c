/*
compilerStates.c

	This file contains implementations of different "states" the compiler FSM can reach. Each state function
	takes a Compiler and a Context and uses them to read Eesk code and write its corresponding eeskIR to the
	output file.

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

#include <compilerStates.h>
#include <eeskIR.h>
#include <writer.h>
#include <ffi.h>
#include <symbolTable.h>
#include <tokenizer.h>

long compileIf(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Context _CO; subContext(CO, &_CO);
	Compiler _C; subCompiler(C, &_C);
	long nameAddr = begin;

	//get length of component statements
	_CO.symbols = tableAddLayer(_CO.symbols, tok, 0);
	_C.dst = NULL;
	long conditionLength = compileStatement(&_C, &_CO, tok);
	long clauseLength = compileStatement(&_C, &_CO, tok);
	long elseLength = compileStatement(&_C, &_CO, tok);
	_CO.symbols = tableRemoveLayer(_CO.symbols);

	long offset = C->dictionary[BNE].length + C->dictionary[RPUSH].length + C->dictionary[JMP].length + 1;
	compileStatement(C, CO, tok);					//compiled clause
	writeObj(C, RPUSH, clauseLength+offset);	//else address
	writeObj(C, BNE, 0);						//decide

	compileStatement(C, CO, tok);										//compiled statement
	offset = C->dictionary[JMP].length + 1;
	writeObj(C, RPUSH, elseLength+offset);						//push end address
	writeObj(C, JMP, 0);							//jump to end

	//compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled else
	compileStatement(C, CO, tok);

	return C->LC - begin;
}

long compileWhile(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);
	long nameAddr = begin;

	//get section lengths
	_C.LC = 0;
	_C.dst = NULL;
	_CO.symbols = tableAddLayer(_CO.symbols, tok, 0);
	long condL = compileStatement(&_C, &_CO, tok);	//condition length
	long loopL = compileStatement(&_C, &_CO, tok);	//loop length
	_CO.symbols = tableRemoveLayer(_CO.symbols);

	long offset = C->dictionary[BNE].length + C->dictionary[RPUSH].length + C->dictionary[JMP].length + 1;
	//writeObj(C, RPUSH, condL+loopL+offset);	//end address

	//DC[0] = compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled condtion
	condL = compileStatement(C, CO, tok);

	writeObj(C, RPUSH, loopL + offset);

	writeObj(C, BNE, 0);	//decide

	loopL = compileStatement(C, CO, tok);	//compiled loop

	offset = C->dictionary[RPUSH].length;
	writeObj(C, RPUSH, nameAddr - C->LC - offset + 1);	//begin address
	writeObj(C, JMP, 0);					//iterate

	return C->LC - begin;
}

long compileSet(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	char name[256];
	Compiler _C;
	Context _CO;
	long nameAddr = 1; //begin + 2*WRDSZ;

	//
	//Set up a new namespace
	//
	getToken(C, tok);
	strcpy(name, tok);
	Table *nameSym = tableAddSymbol(CO->symbols, name, nameAddr, CO);	//change to this scope
	CO->symbols = nameSym;
	nameSym->type = 1;							//make sure this symbol correlates to type set
	if(CO->publicFlag) { publicize(CO->symbols); }
	CO->symbols = tableAddLayer(CO->symbols, "this", 1);
	Table *depSym = tableAddSymbol(CO->symbols, "this.dependency", 0, CO);		//add a placeholder for this symbol
	depSym->type = 1;							//dependency set is also of type set
	int oldAnon = C->anonStack->sp;
	long depSC = C->SC;

	//
	//Find symbol names and count body length
	//
		subCompiler(C, &_C);
		subContext(CO, &_CO);
		_CO.parameterFlag = 1;
		_CO.anonFlag = 1;
		_C.dst = NULL;
		_CO.symbols = tableAddLayer(_CO.symbols, "temp", 1);	//Create temporoary symbol layer to catch declarations
			tableAddSymbol(_CO.symbols, depSym->token, depSym->val, CO);	//Make sure this reference isn't accumulated from temp layer
	compileStatement(&_C, &_CO, tok);	//dependency set
	long paramL = (_C.anonStack->sp - oldAnon);		//number of symbols in dependency set
		_CO.parameterFlag = 0;
		_C.LC = 0;	//because parameters don't increment location counter
		_CO.instructionFlag = 1;
		_CO.anonFlag = 0;
	long bodySC = _C.SC;
	long bodyL = compileStatement(&_C, &_CO, tok);	//body length
	//printf("bodyL first = %lx\n", bodyL);
		//CO->expectedLength = _C.LC;
		depSym->val = bodyL + C->dictionary[RSR].length + C->dictionary[RPUSH].length*2 + C->dictionary[JMP].length + 2*WRDSZ;
	long offset;

	//
	//Prepare argument symbols
	//
	_CO.parameterFlag = 1;
	Table *sym;
	int i;
		//place argument symbols into proper layer
	for(i=0; i < paramL; i++) {
		sym = C->anonStack->array[oldAnon+i];
		sym->val = i * WRDSZ;
		tableAddSymbol(CO->symbols, sym->token, sym->val, &_CO);
		//printf("ARGSYM: %s is argument #%x:%x\n", sym->token, i, sym->val);
	}
	_CO.symbols = tableRemoveLayer(_CO.symbols);		//Remove temporary symbol layer
	_C.anonStack->sp = oldAnon;				//Retract stack pointer

	//
	//Skip over dependency source
	//
	C->SC = bodySC;

	//
	//Jump over Set definition
	//
	if(CO->instructionFlag) {
		offset = C->dictionary[JMP].length + C->dictionary[DATA].length*2 + C->dictionary[RSR].length + 1;
		writeObj(C, RPUSH, bodyL+offset);	//this used to be a PUSH
		writeObj(C, JMP, 0);		//Hop over the definition	//this used to be a HOP
	}

	//
	//Write Set header
	//
	long totalLength = WRDSZ*2 + bodyL + C->dictionary[RPUSH].length + C->dictionary[RSR].length;
	//tableAddSymbol(CO->symbols, "this.dependency", totalLength+(2*WRDSZ), CO);
	writeObj(C, DATA, totalLength);	//a word containing total function length
	writeObj(C, DATA, paramL*WRDSZ);	//write argument number at callAddress-1, extra word for return address no longer needed (kept on r15)
	//reset calling address to right here

	//
	//Reset calling address
	//
	nameAddr = C->LC;	//an extra word for total length (for newing), and an extra word for stack request
	//tableAddSymbol(CO->symbols->parent->layerRoot, name, nameAddr, CO);		//overwrite previous definition
	nameSym->val = nameAddr;
	//CO->symbols->val = nameAddr;
	//printf("SYMBOLS: %s %lx\n", CO->symbols->token, CO->symbols->val);
	if(CO->publicFlag) { publicize(nameSym); }

	//
	//Write Set body
	//
//printf("%lx: compiling body %lx\n", C->LC, C->SC);
		subCompiler(C, &_C);
		CO->expectedLength = totalLength;
		_C.LC = 0;
		_C.dst = C->dst;
	bodyL = compileStatement(&_C, CO, tok);	//compiled body, relatively addressed
	//printf("bodyL next = %lx\n", bodyL);
		C->SC = _C.SC;
		C->LC += _C.LC;
		CO->expectedLength = 0;
		_CO.instructionFlag = CO->instructionFlag;
	writeObj(C, RSR, 0);

	if(CO->instructionFlag) {
		//put our calling address atop the stack
		writeObj(C, RPUSH, nameAddr - C->LC - WRDSZ + 1);
	}

	//
	//Prepare dependency Set
	//
	//tableAddSymbol(CO->symbols->parent->layerRoot, "this.dependency", C->LC+2*WRDSZ, CO);		//reset symbol value
//printf("compiling depset %lx\n", depSC);
		subCompiler(C, &_C);
		_C.SC = depSC;
	long depL = compileAnonSet(&_C, CO, tok);
		C->LC += depL;
		writeObj(C, CLR, 0);

//printf("continuing...\n");
	//
	//no longer in this namespace
	//
	CO->symbols = tableRemoveLayer(CO->symbols);
	if(CO->typingFlag) writeObj(C, TPUSH, 1);
	return C->LC - begin;
}

long compileAnonSet(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C;
	Context _CO;

	//new namespace
	long nameAddr = C->LC + C->dictionary[RPUSH].length + C->dictionary[JMP].length + 2*WRDSZ;
	CO->symbols = tableAddLayer(CO->symbols, "this", 0);
	CO->symbols = tableAddSymbol(CO->symbols, "this", nameAddr, CO);

	//remember sp of anonStack
	int oldAnon = C->anonStack->sp;

	//count length
		subCompiler(C, &_C);
		_C.dst = NULL;
		_C.LC = 0;
		subContext(CO, &_CO);
		//_CO.symbols = tableAddLayer(_CO.symbols, tok, 1);
		_CO.anonFlag = 1;
	long bodyL = compileStatement(&_C, &_CO, tok);
		//_CO.symbols = tableRemoveLayer(_CO.symbols);
	
	//jump to end
	long end = C->dictionary[JMP].length + (2*C->dictionary[DATA].length + bodyL + C->dictionary[RSR].length) + 1;
	long apushes = C->dictionary[NPUSH].length * (C->anonStack->sp - oldAnon);
		end += apushes;
	writeObj(C, RPUSH, end);
	writeObj(C, JMP, 0);

	//set calling address

	//write Set data
	writeObj(C, DATA, 2*WRDSZ + apushes + bodyL + C->dictionary[RSR].length);
	writeObj(C, DATA, 0); //WRDSZ * (C->anonStack->sp -  oldAnon));

	//set calling address
	nameAddr = C->LC;

	//write , operators for declared symbols
	Table *sym;
	int i, j = 0;
	for(i=oldAnon; i < C->anonStack->sp; i++) {
		sym = C->anonStack->array[C->anonStack->sp - j - 1];
		j++;
		sym->parameterFlag = 1;
		sym->val = (i-oldAnon) * WRDSZ;
		writeObj(C, NPUSH, 0);
		//printf("AnonSym %s %d\n", sym->token, sym->val);
	}
	//printf("\n");
	C->anonStack->sp = oldAnon;

	//write Set body
		//subCompiler(C, &_C);
		//_C.LC = 0;
	bodyL = compileStatement(C, CO, tok);
		//C->SC = _C.SC;
		//C->LC += _C.LC;
	writeObj(C, RSR, 0);

	//push beginning
	writeObj(C, RPUSH, nameAddr - C->LC + 1 - WRDSZ);

	//return namespace
	CO->symbols = tableRemoveLayer(CO->symbols);
	if(CO->typingFlag) writeObj(C, TPUSH, 1);
	return C->LC - begin;
}

long compileCall(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);

	_CO.instructionFlag = 1;
	if(!CO->anonFlag) _CO.symbols = tableAddLayer(_CO.symbols, tok, 0);	//because anonymous symbols exist in a flat layer
	_C.dst = NULL;
	long argL = compileStatement(&_C, &_CO, tok);	//find argument length
	if(!CO->anonFlag) _CO.symbols = tableRemoveLayer(_CO.symbols);

	//push return address of call
	//if(CO->typingFlag) writeObj(C, TDUP, 0);
	long distance;
	if(CO->swapFlag) {
		distance = argL + C->dictionary[SWAP].length + C->dictionary[RESWAP].length + C->dictionary[JSR].length + 1;
	} else {
		distance = argL + C->dictionary[JSR].length + 1;
	}
	writeObj(C, RPUSH, distance);			//push return address

	//swap stacks
	if(CO->swapFlag) {
		writeObj(C, SWAP, 0);
		++(CO->swapDepth);
	}

	//compile arguments
	subContext(CO, &_CO);
	compileStatement(C, &_CO, tok);
	_CO.instructionFlag = CO->instructionFlag;

	//swap back
	if(CO->swapFlag) {
		writeObj(C, RESWAP, 0);
		--(CO->swapDepth);
	}

	//make call
	writeObj(C, JSR, 0);

	//return to right here
	return C->LC - begin;
}

long compileNative(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);
	
	//swap stacks
	if(CO->swapFlag) {
		writeObj(C, SWAP, 0);
		++(_CO.swapDepth);
	}

	//compiled argument section
	_CO.instructionFlag = 1;
	compileStatement(C, &_CO, tok);
	_CO.instructionFlag = CO->instructionFlag;

	//swap back
	if(CO->swapFlag) {
		writeObj(C, RESWAP, 0);
		--(_CO.swapDepth);
	}

	//make call
	writeObj(C, NTV, 0);
	return C->LC - begin;
}

long compileQuote(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;

	long quoteL = getQuote(C, tok);
	if(!CO->literalFlag) {
		writeObj(C, RPUSH, C->dictionary[RPUSH].length + C->dictionary[JMP].length+1);	//push start of string
		writeObj(C, RPUSH, quoteL + C->dictionary[JMP].length+1);	//push end of string
		writeObj(C, JMP, 0);						//skip over string leaving it on stack
	}
	writeStr(C->dst, tok, &(C->LC));

	return C->LC - begin;
}

long compileDeclaration(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Table *tempTable;

	getToken(C, tok);
	if(numeric(tok[0])) {		//is this an array?
		//create a symbol and some memory
		int len = atoi(tok);
		writeObj(C, DATA, C->LC);
		for(int i=0; i<len; i++)
			writeObj(C, DATA, i*WRDSZ);
		getToken(C, tok);
		if(C->dst) {
			tempTable = tableAddSymbol(CO->symbols, tok, (C->LC)-len-1, CO);
		}
	} else {
		//create a symbol and a word
		tempTable = tableAddSymbol(CO->symbols, tok, (C->LC) + ((CO->instructionFlag)?5:0), CO);
		if(!CO->parameterFlag) {
			if(CO->publicFlag) { publicize(tempTable); }
			if(CO->instructionFlag) { writeObj(C, GRAB, 0); if(CO->typingFlag) writeObj(C, TSYM, 0); }
			//writeObj(C, DATA, 0);
		} else {
			(C->LC) += WRDSZ;
		}
	}
	return C->LC - begin;
}

long compileRedirection(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);
	long val;
	Table *tempTable;

	writeObj(C, CONT, 0);
	getToken(C, tok);
	tempTable = tableLookup(CO->symbols, tok, &(_C.LC));
	if(!tempTable) {
		if(numeric(tok[0]) || (tok[0] == '-' && numeric(tok[1]))) {
			val = atoi(tok);
		} else {
			/*if(C->dst)*/ printf("%d:\tImplicitly declared offset: %s\n", C->lineCounter, tok);
			val = 0;
		}
	} else {
		val = tempTable->val;
	}
	writeObj(C, PUSH, val);
	writeObj(C, ADD, 0);
				
	return C->LC - begin;
}

long compileBackset(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	long val;
	Table *tempTable;

	getToken(C, tok);		//get backset symbol
	tempTable = tableLookup(CO->symbols, tok, &val);
	if(!tempTable) {
		if(numeric(tok[0]) || (tok[0] == '-' && numeric(tok[1]))) {
			val = atoi(tok);
		} else {
			printf("%d:\tImplicitly declared backset: %s.\n", C->lineCounter, tok);
			val = 0;
		}
	} else {
		val = tempTable->backset;
	}

	writeObj(C, CONT, val);
	writeObj(C, BKSET, val);

	return C->LC - begin;
}

long compileNativeStructure(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);
	char string[256];
	/*builds a struct like this:
		function pointer;
		return type;
		argument count;
		argument types
		...
		function name;
	*/

	long offset = C->dictionary[RPUSH].length + C->dictionary[JMP].length + 1;
	getQuote(C, tok);		//once first, to accommodate for opening "
	getQuote(C, tok);
	strcpy(string, tok);

	char *functionName = strtok(string, "():");	//grab function name
	char *returnType = strtok(NULL, "():");		//grab return type
	char *argTypes = strtok(NULL, "():");		//grab argument types

	writeObj(C, RPUSH, offset);		//push Native structure
	writeObj(C, RPUSH, 3*WRDSZ + strlen(argTypes)*WRDSZ + C->dictionary[JMP].length + strlen(functionName) + 2);	//hop over definition
	writeObj(C, JMP, 0);

	writeObj(C, DATA, 0);		//space for function pointer

	//return type
	writeObj(C, DATA, returnType[0]);

	//argument information
	writeObj(C, DATA, strlen(argTypes));	//write argument count
				
	for(int i=0;i<strlen(argTypes);i++) {
		writeObj(C, DATA, (long) argTypes[i]);
	}

	//write function name
	writeStr(C->dst, functionName, &C->LC);
				
	return C->LC - begin;
}

long compileAtom(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);
	double tempFloat;
	//this is either an intended symbol or a number

	if(numeric(tok[0]) || (tok[0] == '-' && numeric(tok[1]))) {
		//this is a numeric literal
		if(isFloat(tok)) {
			tempFloat = atof(tok);
			if(!CO->literalFlag) {
				writeObj(C, PUSH, *(long *) &tempFloat);
			} else {
				if(C->dst) {
					writeObj(C, DATA, *(long *) &tempFloat);
				}
			}
		} else {
			if(!CO->literalFlag) { writeObj(C, PUSH, atol(tok));}
			else writeObj(C, DATA, atol(tok));	//push its value
		}
		if(!CO->literalFlag && CO->typingFlag) writeObj(C, TPUSH, 0);
	} else {
		writeAddressCalculation(C, CO, tok);
	}
	
	return C->LC - begin;
}
