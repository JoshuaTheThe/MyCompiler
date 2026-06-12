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
	movq $800, %rbx
	movq $0, %rcx
	shl $2, %rcx
	add %rcx, %rbx
	movq (%rbx), %rbx
	movq %rbx, %rax
	retq
