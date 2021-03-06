/*
kernel.h

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

#ifndef _kernel_h_
#define _kernel_h_

void kernel(long eeskir);
#endif
