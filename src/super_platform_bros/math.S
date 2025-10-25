    .syntax unified
    .thumb
    .text
    .align 2
    .global fast_add
    .thumb_func

// int fast_add(int a, int b)
fast_add:
    adds r0, r0, r1
    bx lr

