/*
tokenizer.h

	This acts as the tokenizer for a compiler. While ignoring whitespace delimeters, it groups characters into alphabetics, numerics, and symbols,
	then returns that token.

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
#ifndef _tokenizer.h_
#define _tokenizer.h_

void trimComments(char *src);
int isFloat(char *tok);
int symbolic(char c);
int alphabetic(char c);
int getQuote(char *tok, char *src, int *SC);
int numeric(char c);
int getToken(char *token, char *src, int *loc, int *lineCount);

#endif
