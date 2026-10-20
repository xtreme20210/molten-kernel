.intel_syntax noprefix
.text
.global kernelHandoff
kernelHandoff:
cli
mov rsp, rdi
sub rsp, 8
mov rdi, rdx
jmp rsi