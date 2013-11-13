/*
compilerStates.h

	This file contains all the functions that the compiler FSM needs. Finally some modularity...

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
#ifndef _compilerStates.h_
#define _compilerStates.h_

#include <compiler.h>
long compileIf(Compiler *C, Context *context, char *tok);
long compileWhile(Compiler *C, Context *context, char *tok);
long compileSet(Compiler *C, Context *context, char *tok);
long compileAnonSet(Compiler *C, Context *CO, char *tok);
long compileCall(Compiler *C, Context *context, char *tok);
long compileQuote(Compiler *C, Context *context, char *tok);
long compileDeclaration(Compiler *C, Context *context, char *tok);
long compileRedirection(Compiler *C, Context *context, char *tok);
long compileNative(Compiler *C, Context *context, char *tok);
long compileNativeStructure(Compiler *C, Context *CO, char *tok);
long compileAtom(Compiler *C, Context *context, char *tok);
long compileSymbol(Compiler *C, Context *context, char *tok);
#endif
