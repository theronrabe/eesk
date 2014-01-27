/*
options.c
	Provides an interface for compiler command-line options.

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
#include <options.h>
#include <definitions.h>

void contextSetOptions(int argc, char **argv, Context *CO) {
	CO->typingFlag = 0;
	CO->displaySymbols = 0;
	CO->verboseFlag = 0;
	CO->swapFlag = 1;

	if(argc > 2) {
		int i = 2;
		for(;i < argc; i++) {
			if(argv[i][0] == '-') {
				switch (argv[i][1]) {
					case ('t'):
						CO->typingFlag = 1;
						break;
					case ('d'):
						CO->displaySymbols = 1;
						break;
					case ('v'):
						CO->verboseFlag = 1;
						break;
					case ('s'):
						CO->swapFlag = 0;
						break;
				}
			}
		}
	}
}
