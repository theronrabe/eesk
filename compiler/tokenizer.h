/*
Theron Rabe
tokenizer.h
2/11/2013

	This acts as the tokenizer for a compiler. While ignoring whitespace delimeters, it groups characters into alphabetics, numerics, and symbols,
	then returns that token.
*/
#include <stdio.h>
#include <string.h>

void trimComments(char *src) {
	int i;
	int comment = 0;

	for(i=0;i<strlen(src);i++) {
		if(src[i] == '$') { src[i] = ' '; comment = 1; }
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
		} else if(numeric(src[*loc+ws])) {
			while(numeric(src[*loc+ws+i])) {
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
