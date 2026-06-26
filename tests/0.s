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
add:
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	mov a(%rip), %rbx
	mov b(%rip), %r11
	add %r11, %rbx
	movq %rbx, %rax
	jmp __ret
main:
	endbr64
	pushq %rbp
	movq %rsp, %rbp
	movq $2, %rbx
	lea a(%rip), %r11
	mov %rbx, (%r11)
	movq $3, %rbx
	lea b(%rip), %r11
	mov %rbx, (%r11)
	lea add(%rip), %rbx
	call *%rbx
	mov %rax, %rbx
	movq %rbx, %rax
	jmp __ret
	.section .bss
a: .space 8
b: .space 8
