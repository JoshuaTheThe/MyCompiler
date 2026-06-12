	.section .text
	.global  _start
_start:
	andq $-16, %rsp
	call 2f
	movq $60, %rax
	syscall
1:	jmp 1b
2:
	movq $3, %rbx
	movq $3, %rcx
	addq %rcx, %rbx
	movq %rbx, %rdi
	retq
