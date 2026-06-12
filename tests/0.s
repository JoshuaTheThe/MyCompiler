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
__ret:
	movq %rbp, %rsp
	popq %rbp
	retq
main:
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq $32, %rbx
	subq $8, %rsp
	mov %rbx, -8(%rbp)
	mov -8(%rbp), %rbx
	movq %rbx, %rax
	jmp __ret
	.section .bss
k: .space 8
