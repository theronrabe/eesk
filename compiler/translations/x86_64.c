void main(){};

void halt() {
	asm volatile (
			"movq (%%r9), %%rax\n\t" //crash line
			"movq $0x0, %%rdi\n\t"
			"movq %%rsp, %%r13\n\t"
			"movq %%r15, %%rsp\n\t"
			"callq *%%r12\n\t"
			"movq %%rsp, %%r15\n\t"
			"movq %%r13, %%rsp\n\t"
			:::);
}

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

void ntv(){
	asm volatile (
			"movq $0x5, %%rdi\n\t"
			"movq %%rsp, %%r13\n\t"
			"movq %%r15, %%rsp\n\t"
			"callq *%%r12\n\t"
			//"movq %%rsp, %%r15\n\t"	//does this really need to be replaced?
			"movq %%r13, %%rsp\n\t"
			"addq $0x08, %%rsp\n\t"
			"movq (%%r15), %%r14\n\t"	//remove NTV arguments from activationStack
			:::
			);
}

void loc() {
	asm volatile (	"movq -0x8(%%rbp), %%rax\n\t"
			"popq %%rbx\n\t"
			//"lea (%%rax, %%rbx, 1), %%rax\n\t"
			"addq %%rbx, %%rax\n\t"
			"pushq %%rax\n\t"
			:::
			);
}

void dloc() {
	//this instruction is never used by the ee compiler
}

void prnt() {
	asm volatile (
			"movq $0x8, %%rdi\n\t"
			"movq %%rsp, %%r13\n\t"
			"movq %%r15, %%rsp\n\t"
			"callq *%%r12\n\t"
			"movq %%rsp, %%r15\n\t"
			"movq %%r13, %%rsp\n\t"
			"addq $0x8, %%rsp\n\t"
			:::);
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
			"nop;nop;nop;nop;  nop;nop;nop;nop\n\t"
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

void rpop() {
	asm volatile (
			"popq %%rax\n\t"
			"popq %%rbx\n\t"
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
			"movq %%rsp, %%r13\n\t"		//back up stack pointer
			"movq %%r15, %%rsp\n\t"		//counterStack is the active stack
			"movq %%r14, %%rdx\n\t"		//activationStack in rdx
			"movq (%%rsp), %%rcx\n\t"	//past activationStack in rcx
			"pushq %%rdx\n\t"		//remember activationStack
			"subq %%rdx, %%rcx\n\t"		//rcx contains numArgsPassed
			"movq %%rsp, %%r15\n\t"		//replace counterStack
			"movq %%r13, %%rsp\n\t"		//back on regular stack

			//grab call address and resolve activation stack
			"movq 0x8(%%rsp), %%rax\n\t"	//calling address
			"movq -0x8(%%rax), %%rbx\n\t"	//stack request
			"subq %%rcx, %%rbx\n\t"		//how many args weren't passed?
			"subq %%rbx, %%r14\n\t"		//correct activationStack to reflect stack request
			"popq %%rax\n\t"		//grab return address
			"movq %%rax, (%%r14)\n\t"	//put return address on activationStack
			"subq %%rbx, (%%r15)\n\t"	//correct counterStack to reflect stack request

			//place call
			"popq %%rax\n\t"		//grab calling address
				//"movq (%%r15), %%r15\n\t" //crash here
			"jmp *%%rax\n\t"
			:::
			);
}

void rsr() {
	asm volatile (
			/*
			"popq %%rax\n\t"	//grab result
			"popq %%rbx\n\t"	//grab return address
			"popq %%rcx\n\t"	//remove call address from stack
			"pushq %%rax\n\t"	//push result
			"subq $0x8, 0x18(%%rbp)\n\t"	//pop counterStack
			"movq 0x18(%%rbp), %%rax\n\t"	//grab old activationStack
			"movq 0x8(%%rax), %%rax\n\t"		//find top address of counterStack
			"movq %%rax, 0x10(%%rbp)\n\t"	//replace activationStack
			"jmp *%%rcx\n\t"	//return

			Things this needs to do: activationStack <- (counterStack), clear counterStack, return
			*/
			"movq %%rsp, %%r13\n\t"		//back up stack pointer
			"movq %%r15, %%rsp\n\t"		//counterStack is active stack
			"movq (%%r14), %%rax\n\t"	//grab return address
			"popq %%r14\n\t"		//activationStack resets
			"movq (%%rsp), %%r14\n\t"	//The actual correct activationStack
			"movq %%rsp, %%r15\n\t"		//replace counterStack
			"movq %%r13, %%rsp\n\t"		//back on regular stack
			"jmp *%%rax\n\t"		//return
			:::
			);
}

void apush() {
	asm volatile (
			"popq %%rax\n\t"		//grab value
			"movq %%rsp, %%r13\n\t"		//set aside stack
			"movq %%r14, %%rsp\n\t"	//activationStack is active
			"pushq %%rax\n\t"		//push it
			"movq %%rsp, %%r14\n\t"	//replace activationStack
			"movq %%r13, %%rsp\n\t"		//reactivate stack
			:::
			);
}

void aget() {
	asm volatile (
			"movq $0x0123456789abcdef, %%rax\n\t"	//grab offset parameter
			"movq 0x8(%%r15), %%rbx\n\t"		//grab base address (previous activationStack)
			"subq %%rax, %%rbx\n\t"			//combine base with offset
			"pushq %%rbx\n\t"		//push argument to stack
			:::
			);
}

void add() {
	asm volatile (
			"popq %%rax\n\t"
			"addq %%rax, (%%rsp)\n\t"
			:::
			);
}

void sub() {
	asm volatile (
			"popq %%rax\n\t"
			"subq %%rax, (%%rsp)\n\t"
			:::
			);
}

void mul() {
	asm volatile (
			"popq %%rax\n\t"
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
			"cqo\n\t"
			"idivq %%rbx\n\t"
			"pushq %%rax\n\t"
			:::
			);
}

void mod() {
	asm volatile (
			"popq %%rbx\n\t"
			"popq %%rax\n\t"
			"cqo\n\t"
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

void prts(){
	asm volatile (
			"movq $0x26, %%rdi\n\t"
			"movq %%rsp, %%r13\n\t"
			"movq %%r15, %%rsp\n\t"
			"callq *%%r12\n\t"
			"movq %%rsp, %%r15\n\t"
			"movq %%r13, %%rsp\n\t"
			"addq $0x08, %%rsp\n\t"
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

void aloc() {
	asm volatile (
			"movq $0x2a, %%rdi\n\t"
			"movq %%rsp, %%r13\n\t"
			"movq %%r15, %%rsp\n\t"
			"callq *%%r12\n\t"
			"movq %%rsp, %%r15\n\t"
			"movq %%r13, %%rsp\n\t"
			:::
			);
}

void FREE() {
	asm volatile (
			"movq $0x2c, %%rdi\n\t"
			"movq %%rsp, %%r13\n\t"
			"movq %%r15, %%rsp\n\t"
			"callq *%%r12\n\t"
			"movq %%rsp, %%r15\n\t"
			"movq %%r13, %%rsp\n\t"
			"addq $0x08, %%rsp\n\t"
			:::
			);
}

void new() {
	asm volatile (
			"movq $0x2b, %%rdi\n\t"
			"movq %%rsp, %%r13\n\t"
			"movq %%r15, %%rsp\n\t"
			"callq *%%r12\n\t"
			"movq %%rsp, %%r15\n\t"
			"movq %%r13, %%rsp\n\t"
			:::
			);
}

void load() {
	asm volatile (
			"movq $0x2d, %%rdi\n\t"
			"movq %%rsp, %%r13\n\t"
			"movq %%r15, %%rsp\n\t"
			"callq *%%r12\n\t"
			//"movq %%rsp, %%r15\n\t"
			"movq %%r13, %%rsp\n\t"
			:::);
}

void r14() {
	asm volatile (
			"pushq %%r14\n\t"
			:::);
}
