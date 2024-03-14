.code

run_shellcode proc
    mov rax, [rsp].Context.Rax
    mov rcx, [rsp].Context.Rcx
    mov rdx, [rsp].Context.Rdx
    mov rbx, [rsp].Context.Rbx
    mov rbp, [rsp].Context.Rbp
    mov rsi, [rsp].Context.Rsi
    mov rdi, [rsp].Context.Rdi
    mov r8,  [rsp].Context.R8
    mov r9,  [rsp].Context.R9
    mov r10, [rsp].Context.R10
    mov r11, [rsp].Context.R11
    mov r12, [rsp].Context.R12
    mov r13, [rsp].Context.R13
    mov r14, [rsp].Context.R14
    mov r15, [rsp].Context.R15
    mov rsp, [rsp].Context.Rsp

    ; vectored exception handler can redirect out RIP here
    ; i cant believe this is real
    int 3

    ret
run_shellcode endp

end