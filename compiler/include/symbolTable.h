/* symbolTable.h

	The data structure used by the compiler to keep track of user-defined symbols and their associated addresses.

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
#ifndef _symbolTable_h_
#define _symbolTable_h_ 

#include <string.h>
#include <definitions.h>

#define TOKSIZE 256


Table *tableCreate();
void publicize(Table *node);
Table *tableAddSymbol(Table *T, char *token, int val, Context *CO);
Table *tableAddLayer(Table *T, char isObject);
Table *tableRemoveLayer(Table *T);
Table *tableLookup(Table *T, char *token, long *accOff);
void tableDestroy(Table *T);

#endif
