// x86_64 implementation supporting Windows (MS ABI) and Linux (System V ABI)

#if defined(_WIN32) || defined(_WIN64)
    // Windows ABI
    // Args: RCX, RDX, R8, R9, Stack...
    #define LOAD_ARGS \
        movl %ecx, %r10d; \
        movl %edx, %eax; \
        movl %r8d, %ebx; \
        movl %r9d, %r11d; \
        movq 48(%rbp), %r12; \
        movq 56(%rbp), %r13; \
        movq 64(%rbp), %r14; \
        movq 72(%rbp), %r15;
    
    #define OFFSET_TEX_X 80(%rbp)
    #define OFFSET_SHADE 88(%rbp)
    #define OFFSET_LAST 96(%rbp)

#else
    // Linux / System V ABI
    // Args: RDI, RSI, RDX, RCX, R8, R9, Stack...
    #define LOAD_ARGS \
        movl %edi, %r10d; \
        movl %esi, %eax; \
        movl %edx, %ebx; \
        movl %ecx, %r11d; \
        movq %r8, %r12; \
        movq %r9, %r13; \
        movq 16(%rbp), %r14; \
        movq 24(%rbp), %r15;

    #define OFFSET_TEX_X 32(%rbp)
    #define OFFSET_SHADE 40(%rbp)
    #define OFFSET_LAST 48(%rbp)
#endif

.global draw_wall_column_asm
.global draw_sprite_column_asm

// Note: tft_input is defined in tft.c

draw_wall_column_asm:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %rsi
    pushq %rdi
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    // Unified argument loading
    // r10d = x
    // eax = y (start)
    // ebx = y_end
    // r11d = tex_pos
    // r12 = step
    // r13 = tex_data
    // r14 = tex_width
    // r15 = tex_height
    LOAD_ARGS

    // Calculate ptr1 = tft_input + x*240 + y*2
    movq tft_input(%rip), %rsi // rsi = base
    
    // x * 240
    movl %r10d, %edi
    imull $240, %edi, %edi
    addq %rdi, %rsi         // rsi = base + x*240

    // y * 2
    movl %eax, %edi
    shll $1, %edi
    addq %rdi, %rsi         // rsi = ptr1

    // Calculate ptr2
    movl OFFSET_LAST, %ecx     // x2
    cmpq $160, %rcx
    jge .no_x2
    
    // ptr2 = tft_input + x2*240 + y*2
    movq tft_input(%rip), %rdi
    imull $240, %ecx, %ecx
    addq %rcx, %rdi
    movl %eax, %ecx
    shll $1, %ecx
    addq %rcx, %rdi         // rdi = ptr2
    jmp .loop_start

.no_x2:
    xorq %rdi, %rdi         // rdi = 0

.loop_start:
    cmpl %ebx, %eax         // cmp y, y_end
    jg .end_loop

    // texY = (tex_pos >> 16) & (tex_height - 1)
    movl %r11d, %ecx
    shrl $16, %ecx
    movl %r15d, %edx
    decl %edx
    andl %edx, %ecx         // ecx = texY

    // index = texY * tex_width + tex_x
    imull %r14d, %ecx       // texY * tex_width
    addl OFFSET_TEX_X, %ecx     // + tex_x

    // offset = index * 3
    movl %ecx, %edx
    shll $1, %edx
    addl %ecx, %edx         // edx = offset

    // pixel = tex_data[offset]
    movzbl (%r13, %rdx), %ecx // R
    movzbl 1(%r13, %rdx), %r8d // G
    movzbl 2(%r13, %rdx), %r9d // B

    // shade
    movl OFFSET_SHADE, %edx     // shade
    imull %edx, %ecx
    shrl $8, %ecx
    imull %edx, %r8d
    shrl $8, %r8d
    imull %edx, %r9d
    shrl $8, %r9d

    // RGB565
    shrl $3, %ecx
    shll $11, %ecx
    shrl $2, %r8d
    shll $5, %r8d
    shrl $3, %r9d
    orl %r8d, %ecx
    orl %r9d, %ecx          // ecx = color

    // Store ptr1
    movw %cx, (%rsi)
    addq $2, %rsi

    // Store ptr2
    testq %rdi, %rdi
    jz .skip_x2
    movw %cx, (%rdi)
    addq $2, %rdi
.skip_x2:

    // Update
    incl %eax               // y++
    addl %r12d, %r11d       // tex_pos += step
    jmp .loop_start

.end_loop:
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rdi
    popq %rsi
    popq %rbx
    popq %rbp
    ret


draw_sprite_column_asm:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rbx
    pushq %rsi
    pushq %rdi
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    // Unified argument loading
    LOAD_ARGS

    // Calculate ptr = tft_input + x*240 + y*2
    movq tft_input(%rip), %rsi // rsi = base
    
    // x * 240
    movl %r10d, %edi
    imull $240, %edi, %edi
    addq %rdi, %rsi         // rsi = base + x*240

    // y * 2
    movl %eax, %edi
    shll $1, %edi
    addq %rdi, %rsi         // rsi = ptr

.sprite_loop:
    cmpl %ebx, %eax
    jg .sprite_end

    // texY = tex_pos >> 16
    movl %r11d, %ecx
    shrl $16, %ecx
    
    // Clamp texY < tex_height
    cmpl %r15d, %ecx
    jl .tex_ok
    movl %r15d, %ecx
    decl %ecx
.tex_ok:
    testl %ecx, %ecx
    jge .tex_pos_ok
    xorl %ecx, %ecx
.tex_pos_ok:

    // index = texY * tex_width + tex_x
    imull %r14d, %ecx
    addl OFFSET_TEX_X, %ecx

    // offset = index * 3
    movl %ecx, %edx
    shll $1, %edx
    addl %ecx, %edx

    // pixel
    movzbl (%r13, %rdx), %ecx // R
    movzbl 1(%r13, %rdx), %r8d // G
    movzbl 2(%r13, %rdx), %r9d // B

    // Transparency: 255, 60, 255
    cmpl $255, %ecx
    jne .not_trans
    cmpl $60, %r8d
    jne .not_trans
    cmpl $255, %r9d
    je .skip_sprite_pixel

.not_trans:
    // is_hurt
    cmpl $0, OFFSET_LAST
    je .no_hurt
    
    addl $100, %ecx
    cmpl $255, %ecx
    jle .hurt_r_ok
    movl $255, %ecx
.hurt_r_ok:
    shrl $1, %r8d
    shrl $1, %r9d

.no_hurt:
    // shade
    movl OFFSET_SHADE, %edx
    imull %edx, %ecx
    shrl $8, %ecx
    imull %edx, %r8d
    shrl $8, %r8d
    imull %edx, %r9d
    shrl $8, %r9d

    // RGB565
    shrl $3, %ecx
    shll $11, %ecx
    shrl $2, %r8d
    shll $5, %r8d
    shrl $3, %r9d
    orl %r8d, %ecx
    orl %r9d, %ecx

    movw %cx, (%rsi)

.skip_sprite_pixel:
    addq $2, %rsi
    incl %eax
    addl %r12d, %r11d
    jmp .sprite_loop

.sprite_end:
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rdi
    popq %rsi
    popq %rbx
    popq %rbp
    ret
