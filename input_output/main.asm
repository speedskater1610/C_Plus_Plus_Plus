
section .data
   ; compiler varibles
   compilerVarible_format_ln db "%s", 10, 0
   compilerVarible_format_no_ln db "%s", 0
   
   ; user varibles 

	pointer_varible_Mname_println_1 dq userVarible_Mname_chars
   

	userVarible_Mname_init: db 'r', 'u', 's', 's', 'e', 'l', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
section .bss
   

	userVarible_Mname_main_var resb 100
section .text
    global main
main:
    push rbp
    mov rbp, rsp

	; Copy initial string for Mname
	mov rsi, userVarible_Mname_init
	mov rdi, userVarible_Mname_main_var
	mov rcx, 100
	rep movsb

	sub rsp, 40
	lea rcx, [rel compilerVarible_format_ln]
	mov rdx, qword [rel pointer_varible_Mname_println]
	xor rax, rax
	call printf 
	add rsp, 40

   
   mov eax, 0
   leave
   ret

extern printf
extern system
