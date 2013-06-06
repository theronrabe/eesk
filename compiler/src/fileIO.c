/*
fileIO.c

	Does some basic file IO work that I seem to use too often to rewrite.

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
#include <fileIO.h>
#include <stdio.h>
#include <stdlib.h>

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
