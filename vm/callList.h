/*
callList.h

	A data structure to manage call lists for the virtual machine's varying state.

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

typedef struct CallList {
	int address;
	struct CallList *next;
} CallList;

void callFree(CallList *cl) {
	if(!cl->next) {
		free(cl);
		return;
	} else {
		callFree(cl->next);
		free(cl);
		return;
	}
}

CallList *callAdd(CallList *cl, int address) {
	CallList *ret = malloc(sizeof(CallList));
	
	ret->address = address;
	ret->next = cl;

	return ret;
}
