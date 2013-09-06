//We'll say RBP contains the base of eesk's stack, ((RBP)) = eesk MEM space, (RBP+8) = native call function address
//(RBP+10) = activationStack, (RBP+18) = counterStack
void jmp() {
	asm volatile (	"popq %%rax\n\t"
			"jmp *%%rax\n\t"
			:::);
}

void hop() {
	asm volatile (	"popq %%rax\n\t"
			"call _nexthop\n\t"
			"_nexthop:popq %%rbx\n\t"
			"add %%rbx, %%rax\n\t"
			"jmp *%%rax\n\t"
			:::
			);
}

void brn() {
	asm volatile (	"popq %%rax\n\t"
			"popq %%rbx\n\t"
			"test %%rax, %%rax\n\t"
			"jz _skipbrn\n\t"
			"jmp *%%rbx\n\t"
			"_skipbrn:\n\t"
			:::
			);
}

void bne() {
	asm volatile (	"popq %%rax\n\t"
			"popq %%rbx\n\t"
			"test %%rax, %%rax\n\t"
			"jnz _skipbne\n\t"
			"jmp *%%rbx\n\t"
			"_skipbne:\n\t"
			:::
			);
}

void ntv() {
	asm volatile (	"popq %%rsi\n\t"
			"popq %%rdi\n\t"
			"movq 16(%%rbp), %%rdx\n\t"
			"mov (%%rbp), %%rax\n\t"
			"callq *(%%rax)\n\t"
			:::
			);
}

void loc() {
	asm volatile (	"movq (%%rbp), %%rax\n\t"
			"popq %%rbx\n\t"
			"lea (%%rax, %%rbx, 8), %%rax\n\t"
			"pushq %%rax\n\t"
			:::
			);
}

void dloc() {
	//this instruction is never used by the ee compiler
}

void push() {
	asm volatile (	"movq $0x0123456789abcdef, %%rax\n\t"
			"pushq %%rax\n\t"
			:::
			);
}

void rpush() {
	asm volatile (	"leaq 0x01234567(%%rip), %%rax\n\t"
			"pushq %%rax\n\t"
			:::
			);
}

void grab() {
	asm volatile (
			"callq _grab\n\t"
			//make sure there is a quadword-aligned 64-bits of data here by preceding callq with nops"
			"_grab:\n\t"
			:::
			);
}

void popto() {
	//this instruction is never used by the ee compiler
}

void pop() {
	asm volatile (
			"popq %%rbx\n\t"
			"popq %%rax\n\t"
			"movq %%rbx, (%%rax)\n\t"
			:::
			);
}

void bpop() {
	asm volatile (
			"popq %%rbx\n\t"
			"popq %%rax\n\t"
			"movb %%bl, (%%rax)\n\t"
			:::
			);
}

void cont() {
	asm volatile (
			"popq %%rax\n\t"
			"pushq (%%rax)\n\t"
			:::
			);
}

void clr() {
	asm volatile (
			"popq %%rax\n\t"
			:::
			);
}

void jsr() {
	asm volatile (
			//push activationStack onto counterStack, and grab numArgsPassed
			"movq %%rsp, %%r8\n\t"
			"movq 0x18(%%rbp), %%rsp\n\t"	//counterStack is the active stack
			"movq 0x10(%%rbp), %%rcx\n\t"	//activationStack in rcx
			"movq 0x8(%%rsp), %%rdx\n\t"	//past activationStack in rdx
			"pushq %%rcx\n\t"		//remember activationStack
			"subq %%rdx, %%rcx\n\t"		//rcx contains numArgsPassed
			"movq %%rsp, 0x18(%%rbp)\n\t"	//replace counterStack
			"movq %%r8, %%rsp\n\t"		//back on regular stack

			//grab call address and resolve activation stack
			"movq 0x10(%%rsp), %%rax\n\t"	//calling address
			"movq (%%rax), %%rbx\n\t"	//stack request
			"subq %%rcx, %%rbx\n\t"		//how many args weren't passed?
			"addq %%rbx, 0x10(%%rbp)\n\t"	//correct activationStack to reflect stack request
			"movq 0x18(%%rbp), %%rdx\n\t"	//grab counterStack
			"addq %%rbx, 0x8(%%rdx)\n\t"	//correct counterStack to reflect stack request

			//place call
			"jmp *%%rax\n\t"
			:::
			);
}

void rsr() {
	asm volatile (
			"popq %%rax\n\t"	//grab result
			"popq %%rbx\n\t"	//grab return address
			"popq %%rcx\n\t"	//remove call address from stack
			"pushq %%rax\n\t"	//push result
			"subq $0x8, 0x18(%%rbp)\n\t"	//pop counterStack
			"movq 0x18(%%rbp), %%rax\n\t"	//grab old activationStack
			"movq 0x8(%%rax), %%rax\n\t"		//find top address of counterStack
			"movq %%rax, 0x10(%%rbp)\n\t"	//replace activationStack
			"jmp *%%rcx\n\t"	//return
			:::
			);
}

void apush() {
	asm volatile (
			"popq %%rax\n\t"		//grab value
			"movq %%rsp, %%r8\n\t"		//set aside stack
			"movq 0x10(%%rbp), %%rsp\n\t"	//activationStack is active
			"pushq %%rax\n\t"		//push it
			"movq %%rsp, 0x10(%%rbp)\n\t"	//replace activationStack
			"movq %%r8, %%rsp\n\t"		//reactivate stack
			:::
			);
}

void aget() {
	asm volatile (
			"movq $0x0123456789abcdef, %%rax\n\t"	//grab parameter
			"movq 0x18(%%rbp), %%rdx\n\t"		//grab counterStack
			"subq 0x10(%%rdx), %%rax\n\t"		//find argument's address
			"pushq (%%rax)\n\t"			//push argument to stack
			:::
			);
}

void add() {
	asm volatile (
			"popq %%rax\n\t"
			"addq %%rax, 0x8(%%rsp)\n\t"
			:::
			);
}

void sub() {
	asm volatile (
			"popq %%rax\n\t"
			"subq %%rax, 0x8(%%rsp)\n\t"
			:::
			);
}

void mul() {
	asm volatile (
			"popq %%rbx\n\t"
			"mulq %%rbx\n\t"
			"pushq %%rax\n\t"
			:::
			);
}

void div() {
	asm volatile (
			"popq %%rbx\n\t"
			"popq %%rax\n\t"
			"idivq %%rbx\n\t"
			"pushq %%rax\n\t"
			:::
			);
}

void mod() {
	asm volatile (
			"popq %%rbx\n\t"
			"popq %%rax\n\t"
			"idivq %%rbx\n\t"
			"pushq %%rdx\n\t"
			:::
			);
}

void and() {
	asm volatile (
			"popq %%rax\n\t"
			"andq %%rax, 0x8(%%rsp)\n\t"
			:::
			);
}

void or() {
	asm volatile (
			"popq %%rax\n\t"
			"orq %%rax, 0x8(%%rsp)\n\t"
			:::
			);
}

void not() {
	asm volatile (
			"notq 0x8(%%rsp)\n\t"
			:::
			);
}

void gt() {
	asm volatile (
			"popq %%rax\n\t"
			"popq %%rbx\n\t"
			"cmpq %%rax, %%rbx\n\t"
			"jg _gt\n\t"
			"pushq $0x0\n\t"
			"jmp _gtn\n\t"
			"_gt:pushq $0x1\n\t"
			"_gtn:\n\t"
			:::
			);
}

void lt() {
	asm volatile (
			"popq %%rax\n\t"
			"popq %%rbx\n\t"
			"cmpq %%rax, %%rbx\n\t"
			"jl _lt\n\t"
			"pushq $0x0\n\t"
			"jmp _ltn\n\t"
			"_lt:pushq $0x1\n\t"
			"_ltn:\n\t"
			:::
			);
}

void eq() {
	asm volatile (
			"popq %%rax\n\t"
			"popq %%rbx\n\t"
			"cmpq %%rax, %%rbx\n\t"
			"je _eq\n\t"
			"pushq $0x0\n\t"
			"jmp _eqn\n\t"
			"_eq:pushq $0x1\n\t"
			"_eqn:\n\t"
			:::
			);
}
