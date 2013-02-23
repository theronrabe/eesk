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
#include <stdio.h>
#include <string.h>

void trimComments(char *src) {
	int i;
	int comment = 0;

	for(i=0;i<strlen(src);i++) {
		if(src[i] == '#') { src[i] = ' '; comment = 1; }
		if(src[i] == '\n') { comment = 0; }

		if(comment) {
			src[i] = ' ';
		}
	}
}

int isFloat(char *tok) {
	int max = strlen(tok);
	int i;

	for(i=0;i<max;i++) {
		if(tok[i] == '.') return 1;
	}
	return 0;
}

int symbolic(char c) {
	char *symbols = "<>+-*/=";
	int i;
	for(i=0;i<strlen(symbols);i++) {
		if(c == symbols[i]) return 1;
	}

	return 0;
}

int alphabetic(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int numeric(char c) {
	return (c >= '0' && c <= '9') || (c == '.');
}

int getToken(char *token, char *src, int *loc) {
	//places src's next token (starting at location loc) into token, then returns it's final location.

	int i=0;
	int ws = 0;
	
	if(*loc < strlen(src)) {
		//trim whitespace
		while(src[*loc+ws]==' '||src[*loc+ws]=='\t'||src[*loc+ws]=='\n') ++ws;
		
		//Get token
		token[i++] = src[*loc+ws];
	
		if(alphabetic(src[*loc+ws])) {
			while(alphabetic(src[*loc+ws+i])) {
				token[i] = src[*loc+ws+i];
				++i;
			}
		} else if(numeric(src[*loc+ws]) || (src[*loc+ws] == '-' && numeric(src[*loc+ws+1]))) {
			while(numeric(src[*loc+ws+i])) {
			/////////////FIXING NEGATIVE NUMBER TOKENS
				token[i] = src[*loc+ws+i];
				++i;
			}
		} else if(symbolic(src[*loc+ws+i])) {
			while(symbolic(src[*loc+ws+i])) {
				token[i] = src[*loc+ws+i];
				++i;
			}
		}
	
		token[i] = '\0';
		//printf("%s\n", token);
		*loc += i+ws;
		return i+ws;
	} else {
		return -1;
	}
}

int getQuote(char *tok, char *src, int *SC) {
	int i = 0;
	while(src[*SC+i] != '\"') {
		tok[i] = src[*SC+i];
		++i;
	}
	tok[i++] = '\0';

	return i;
}
