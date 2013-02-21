/*
Theron Rabe
fileIO.h
2/11/2013

	Does some basic file IO work that I seem to use too often to rewrite.
*/
#include <stdio.h>

char *loadFile(char *fn) {
	FILE *fp;
	char *ret = NULL;
	int i=0;

	fp = fopen(fn, "rt");
	fseek(fp, 0, SEEK_END);
	i = ftell(fp);
	rewind(fp);
	ret = (char *)malloc(sizeof(char) * (i));
	fread(ret,sizeof(char),i,fp);
	fclose(fp);
	return ret;
}
