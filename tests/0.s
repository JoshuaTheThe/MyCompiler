	.section .text
	.global  _start
	.global  main
_start:
	andq $-16, %rsp
	call main
	movq %rax, %rdi
	movq $60, %rax
	syscall
1:	jmp 1b
main:
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	lea *main, %rbx
	call %rbx
	mov %rax, %rbx
	movq %rbx, %rax
	movq %rbp, %rsp
	popq %rbp
	retq
	movq %rbp, %rsp
	popq %rbp
	retq
