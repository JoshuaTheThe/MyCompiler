	.section .text
	.global  _start
_start:
	andq $-16, %rsp
	call 2f
	movq %rax, %rdi
	movq $60, %rax
	syscall
1:	jmp 1b
2:
	movq $1, %rbx
	movq $1, %rcx
	cmpq %rcx, %rbx
	setne %bl
	movzx %bl, %rbx
	movq %rbx, %rax
	retq
