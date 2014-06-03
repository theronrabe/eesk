/*
writer.h
	Provides an output interface.

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
#ifndef _writer_h_
#define _writer_h_

#include <stdio.h>
#include <compiler.h>

void writeVals(FILE *file, char *vals, int length);		//writes length number of bytes from vals[]
void writeObj(Compiler *C, long instr, long param);			//writes an eesk instruction
void writeStr(FILE *fn, char *str, long *LC);			//writes an eesk string
#endif
