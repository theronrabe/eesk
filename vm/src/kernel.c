/*
kernel.c

	This file contains the eesk kernel. The kernel is a function that takes an eeskir instruction
	and a return address. It then performs the correlating actions at a higher level than the machine 
	code can easily do, and in a safer environment. Process control is then returned to the return
	address. The kernel's calling address is remembered at eesk startup for easy calling at any time.

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

#include <vm.h>
#include <eeskIR.h>
#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <jit.h>
#include <string.h>
#include <sys/mman.h>

void kernel(long eeskir) {
	long **rsp, *rbp;
	void *res;
	long *aStack, *cStack, *tStack, *otStack;

	asm volatile (
			"movq %%r13, %0\n\t"
			"movq %%rbp, %1\n\t"
			"movq %%r14, %2\n\t"
			"movq %%r15, %3\n\t"
			"movq %%r11, %4\n\t"
			"movq %%r10, %5\n\t"
			:"=m" (rsp), "=m" (rbp), "=m" (aStack), "=m" (cStack), "=m" (tStack), "=m" (otStack)
			:
			:"memory"
			);

	switch(eeskir) {
		case(HALT):
			quit((long *)rsp, rbp, tStack);
			break;
		case(PRNT):
			printf("%lx\n", *rsp);
			break;
		case(PRTS):
			printf("%s", *rsp);
			break;
		case(PRTC):
			printf("%c", *(char *) rsp);
			break;
		case(PRTF):
			printf("%f", *(double *)rsp);
			break;
		case(ALOC):
			*rsp = malloc((int)*rsp);
			break;
		case(NEW):
			newCollection(rsp);
			break;
		case(FREE):
			munmap((*rsp)-2, *((*rsp)-2));
			break;
		case(NTV):
			*(long *)(rsp+1) = nativeCall(*(rsp+1), *rsp, aStack);
			break;
		case(LOAD):
			loadLib((char **)rsp);
			break;
		case(CREATE):
			create(rsp, (long **)aStack);
			break;
		case(EVAL):
			res = jitCompile((char *)*rsp);	//compile and load
			printf("2 %p = lx\n", res);
			*rsp = res;
			break;
			
	}
	//printf("kernel returning to address %lx\n", *aStack);

	asm volatile (
			"movq %0, %%r11\n\t"
			"movq %1, %%r10\n\t"
			:
			:"r" (tStack), "r" (otStack)
			:
			);
}
