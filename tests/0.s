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
	movq $3, %rax
	movq $3, %rbx
	addq %rbx, %rax
	movq $9, %rax
	movq $6, %rbx
	addq %rbx, %rax
	retq
