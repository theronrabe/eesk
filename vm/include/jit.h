/*
jit.h

	This is the header file for the Eesk just-in-time compiler. Someday, it may be more extravagant.

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

#ifndef _jit_h_
#define _jit_h_

#include <assembler.h>
long *jitSet(int count, long *values);
void writeJit(long *SC, int eeskIR, long arg);
void jitInit(translation *dictionary);
void jitEnd();
void *jitCompile(char *src);
#endif
