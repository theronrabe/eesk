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
	CO->symbols = tableAddLayer(CO->symbols, tok, 0);
	_C.dst = NULL;
	long conditionLength = compileStatement(&_C, &_CO, tok);
	long clauseLength = compileStatement(&_C, &_CO, tok);
	long elseLength = compileStatement(&_C, &_CO, tok);
	CO->symbols = tableRemoveLayer(CO->symbols);

	long offset = C->dictionary[BNE].length + C->dictionary[RPUSH].length + C->dictionary[JMP].length + 1;
	writeObj(C, RPUSH, conditionLength+clauseLength+offset);	//else address
	compileStatement(C, CO, tok);					//compiled clause
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
	writeObj(C, RPUSH, condL+loopL+offset);	//end address

	//DC[0] = compileStatement(keyWords, symbols, dictionary, src, SC, dst, LC, &subContext, (dst)?lineCount:&i);		//compiled condtion
	compileStatement(C, CO, tok);

	writeObj(C, BNE, 0);	//decide

	compileStatement(C, CO, tok);	//compiled loop

	offset = C->dictionary[RPUSH].length;
	writeObj(C, RPUSH, nameAddr - C->LC - offset + 1);	//begin address
	writeObj(C, JMP, 0);					//iterate

	return C->LC - begin;
}

long compileSet(Compiler *C, Context *CO, char *tok, char anonymous) {
	char name[256];
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);
	long nameAddr = begin;

	//in a new namespace
	getToken(C, tok);
	strcpy(name, tok);
	CO->symbols = tableAddSymbol(CO->symbols, name, nameAddr, CO->staticFlag, CO->parameterFlag);	//change to this scope
	if(CO->publicFlag) { publicize(CO->symbols); }
	CO->symbols = tableAddLayer(CO->symbols, name, 1);

	//count length of parameters, data, and statement sections
		_C.LC = 0;
		_C.SC = C->SC;
		_CO.parameterFlag = 1;
		_CO.instructionFlag = 0;
		_C.dst = NULL;
		_CO.symbols = tableAddLayer(_CO.symbols, name, 1);
	long paramL = compileStatement(&_C, &_CO, tok);	//param length
		_CO.parameterFlag = 0;
		_C.LC = 0;	//because parameters don't increment location counter
		_CO.instructionFlag = 1;
	long bodyL = compileStatement(&_C, &_CO, tok);	//body length
		_CO.symbols = tableRemoveLayer(_CO.symbols);
	long offset;
	
	if(CO->instructionFlag) {
		offset = C->dictionary[JMP].length + C->dictionary[DATA].length*2 + C->dictionary[RSR].length + 1;
		writeObj(C, RPUSH, bodyL+offset);	//this used to be a PUSH
		writeObj(C, JMP, 0);		//Hop over the definition	//this used to be a HOP
	}

	//reset calling address
	nameAddr = C->LC + WRDSZ*2;	//an extra word for total length (for newing), and an extra word for stack request
	tableAddSymbol(CO->symbols->parent->layerRoot, name, nameAddr, CO->staticFlag, CO->parameterFlag);		//overwrite previous definition
	if(CO->publicFlag) { publicize(CO->symbols->parent); }

		_C.LC = 0;
		_C.SC = C->SC;
		_CO.parameterFlag = 1;
		_CO.instructionFlag = 0;
		_C.dst = C->dst;
	paramL = compileStatement(&_C, &_CO, tok);	//compile parameters
		_CO.parameterFlag = 0;
		_CO.instructionFlag = 1;

	writeObj(C, DATA, WRDSZ*2 + bodyL + C->dictionary[RPUSH].length + C->dictionary[RSR].length);	//a word containing total function length
	writeObj(C, DATA, paramL+WRDSZ);	//write argument number at callAddress-1, extra word for return address
	//reset calling address to right here

		_C.LC = 0;
		_C.dst = C->dst;
	bodyL = compileStatement(&_C, CO, tok);	//compiled body, relatively addressed
		C->SC = _C.SC;
		C->LC += _C.LC;
		_CO.instructionFlag = CO->instructionFlag;
	writeObj(C, RSR, 0);

	//instructionFlagged functions should jump to right here
	
	if(CO->instructionFlag) {
		//put our calling address atop the stack
		writeObj(C, RPUSH, nameAddr - C->LC+1 - WRDSZ);
	}

	//no longer in this namespace
	CO->symbols = tableRemoveLayer(CO->symbols);

	return C->LC - begin;
}

long compileCall(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);

	_CO.instructionFlag = 1;
	_CO.symbols = tableAddLayer(_CO.symbols, tok, 0);
	_C.dst = NULL;
	long argL = compileStatement(&_C, &_CO, tok);	//find argument length
	_CO.symbols = tableRemoveLayer(_CO.symbols);

	//push return address of call
	writeObj(C, RPUSH, argL+C->dictionary[JSR].length+1);			//push return address

	//compile arguments
	compileStatement(C, &_CO, tok);
	_CO.instructionFlag = CO->instructionFlag;

	//make call
	writeObj(C, JSR, 0);

	//return to right here
	return C->LC - begin;
}

long compileNative(Compiler *C, Context *CO, char *tok) {
	long begin = C->LC;
	Compiler _C; subCompiler(C, &_C);
	Context _CO; subContext(CO, &_CO);
	//compiled argument section
	_CO.instructionFlag = 1;
	compileStatement(C, &_CO, tok);
	_CO.instructionFlag = CO->instructionFlag;

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
			tempTable = tableAddSymbol(CO->symbols, tok, (C->LC)-len-1, CO->staticFlag, CO->parameterFlag);
		}
	} else {
		//create a symbol and a word
		tempTable = tableAddSymbol(CO->symbols, tok, (C->LC) + ((CO->instructionFlag)?5:0), CO->staticFlag, CO->parameterFlag);
		if(!CO->parameterFlag) {
			if(CO->publicFlag) publicize(tempTable);
			if(CO->instructionFlag) writeObj(C, GRAB, 0);
			writeObj(C, DATA, 0);
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
		if(numeric(tok[0])) {
			val = atoi(tok);
		} else {
			printf("%d:\tImplicitly declared offset: %s.\n", C->lineCounter, tok);
			val = 0;
		}
	} else {
		val = tempTable->val;
	}
	writeObj(C, PUSH, val);
	writeObj(C, ADD, 0);
				
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
			if(!CO->literalFlag) writeObj(C, PUSH, atol(tok));
			else writeObj(C, DATA, atol(tok));	//push its value
		}
	} else {
		writeAddressCalculation(C, CO, tok);
	}
	
	return C->LC - begin;
}
