// render_asm.S
// Supports ARM Cortex-M0+ (Thumb-2)

.syntax unified
.cpu cortex-m0plus
.thumb

.global tft_input

.global draw_wall_column_asm
.global draw_sprite_column_asm

.type draw_wall_column_asm, %function
.align 2

// void draw_wall_column_asm(
//     int x,               // r0 -> r4
//     int y_start,         // r1 -> r5
//     int y_end,           // r2 -> r6
//     uint32_t tex_pos,    // r3 -> r7
//     uint32_t step,       // [sp, #20]
//     const uint8_t *tex_data, // [sp, #24]
//     int tex_width,       // [sp, #28]
//     int tex_height,      // [sp, #32]
//     int tex_x,           // [sp, #36]
//     int shade_int,       // [sp, #40]
//     int x2               // [sp, #44]
// );

draw_wall_column_asm:
    push {r4, r5, r6, r7, lr}
    
    // Save arguments
    mov r4, r0  // x
    mov r5, r1  // y (current y)
    mov r6, r2  // y_end
    mov r7, r3  // tex_pos

    // Calculate ptr1
    // ptr1 = tft_input + (x * 240) + (y * 2)
    // Note: TFT_WIDTH=160 (x), TFT_HEIGHT=120 (y).
    // TFT_SWAP_XY=1 means tft_input[x][y].
    // Stride is 120 pixels * 2 bytes = 240 bytes.
    
    ldr r0, =tft_input
    ldr r0, [r0]       // r0 = buffer base address

    movs r1, #240
    muls r1, r4         // r1 = x * 240
    adds r0, r0, r1     // r0 = base + x*240
    
    lsls r1, r5, #1     // r1 = y * 2
    adds r0, r0, r1     // r0 = ptr1
    
    mov r4, r0         // r4 = ptr1

    // Calculate ptr2 (x2)
    ldr r3, [sp, #44]  // x2
    cmp r3, #160
    bge no_x2

    // ptr2 = tft_input + (x2 * 240) + (y * 2)
    ldr r0, =tft_input
    ldr r0, [r0]
    
    movs r1, #240
    muls r1, r3
    adds r0, r0, r1
    
    lsls r1, r5, #1
    adds r0, r0, r1     // r0 = ptr2
    
    mov r8, r0         // r8 = ptr2
    b loop

no_x2:
    movs r0, #0
    mov r8, r0         // r8 = 0 (invalid ptr)

loop:
    cmp r5, r6
    bgt end

    // Texture mapping
    // Calculate texY = (tex_pos >> 16) & (tex_height - 1)
    lsrs r0, r7, #16
    ldr r1, [sp, #32]  // tex_height
    subs r1, #1
    ands r0, r1        // r0 = texY
    
    ldr r1, [sp, #28]  // tex_width
    muls r0, r1         // texY * texWidth
    
    ldr r1, [sp, #36]  // tex_x
    adds r0, r0, r1     // index
    
    // index * 3
    lsls r1, r0, #1
    adds r0, r0, r1     // offset
    
    ldr r1, [sp, #24]  // tex_data
    adds r1, r1, r0     // pixel address
    
    ldrb r0, [r1, #0]  // R
    ldrb r2, [r1, #1]  // G
    ldrb r3, [r1, #2]  // B
    
    // Apply shade
    ldr r1, [sp, #40]  // shade
    muls r0, r1
    lsrs r0, r0, #8
    muls r2, r1
    lsrs r2, r2, #8
    muls r3, r1
    lsrs r3, r3, #8
    
    // RGB565 conversion
    lsrs r0, r0, #3
    lsls r0, r0, #11
    lsrs r2, r2, #2
    lsls r2, r2, #5
    lsrs r3, r3, #3
    orrs r0, r2
    orrs r0, r3     // r0 = color
    
    // Write ptr1
    strh r0, [r4]
    adds r4, #2         // Advance ptr1 (y+1)
    
    // Write ptr2 if valid
    mov r1, r8
    cmp r1, #0
    beq skip_x2
    strh r0, [r1]
    adds r1, #2         // Advance ptr2
    mov r8, r1         // Update r8
skip_x2:

    // Update
    adds r5, #1         // y++
    ldr r0, [sp, #20]  // step
    add r7, r7, r0     // tex_pos += step
    
    b loop

end:
    pop {r4, r5, r6, r7, pc}


.type draw_sprite_column_asm, %function
.align 2

// void draw_sprite_column_asm(
//     int x,               // r0 -> r4
//     int y_start,         // r1 -> r5
//     int y_end,           // r2 -> r6
//     uint32_t tex_pos,    // r3 -> r7
//     uint32_t step,       // [sp, #20]
//     const uint8_t *tex_data, // [sp, #24]
//     int tex_width,       // [sp, #28]
//     int tex_height,      // [sp, #32]
//     int tex_x,           // [sp, #36]
//     int shade_int,       // [sp, #40]
//     int is_hurt          // [sp, #44]
// );

draw_sprite_column_asm:
    push {r4, r5, r6, r7, lr}
    
    // Save arguments
    mov r4, r0  // x
    mov r5, r1  // y
    mov r6, r2  // y_end
    mov r7, r3  // tex_pos

    // Calculate ptr
    // ptr = tft_input + (x * 240) + (y * 2)
    ldr r0, =tft_input
    ldr r0, [r0]       // r0 = buffer base
    
    movs r1, #240
    muls r1, r4
    adds r0, r0, r1     // r0 = base + x*240
    
    lsls r1, r5, #1
    adds r0, r0, r1     // r0 = ptr
    
    mov r4, r0         // r4 = ptr

sprite_loop:
    cmp r5, r6
    bgt sprite_end

    // Calculate texY
    lsrs r0, r7, #16    // r0 = texY
    ldr r1, [sp, #32]  // tex_height
    
    cmp r0, r1
    blt tex_y_ok
    subs r0, r1, #1     // clamp
tex_y_ok:
    cmp r0, #0
    bge tex_y_pos
    movs r0, #0
tex_y_pos:

    // Calculate index
    ldr r1, [sp, #28]  // tex_width
    muls r0, r1         // texY * texWidth
    ldr r1, [sp, #36]  // tex_x
    adds r0, r0, r1
    
    lsls r1, r0, #1
    adds r0, r0, r1     // offset
    
    ldr r1, [sp, #24]  // tex_data
    adds r1, r1, r0
    
    ldrb r0, [r1, #0]  // R
    ldrb r2, [r1, #1]  // G
    ldrb r3, [r1, #2]  // B

    // Check transparency: R=255, G=60, B=255
    cmp r0, #255
    bne not_trans
    cmp r2, #60
    bne not_trans
    cmp r3, #255
    beq skip_pixel
not_trans:

    // Check is_hurt
    ldr r1, [sp, #44]  // is_hurt
    cmp r1, #0
    beq skip_hurt_effect
    
    // Apply hurt effect: R = min(R+100, 255), G/=2, B/=2
    adds r0, #100
    cmp r0, #255
    ble hurt_r_ok
    movs r0, #255
hurt_r_ok:
    lsrs r2, r2, #1
    lsrs r3, r3, #1
skip_hurt_effect:

    // Apply shade
    ldr r1, [sp, #40]  // shade_int
    muls r0, r1
    lsrs r0, r0, #8
    muls r2, r1
    lsrs r2, r2, #8
    muls r3, r1
    lsrs r3, r3, #8

    // RGB565 conversion
    lsrs r0, r0, #3
    lsls r0, r0, #11
    lsrs r2, r2, #2
    lsls r2, r2, #5
    lsrs r3, r3, #3
    orrs r0, r2
    orrs r0, r3     // r0 = color

    // Write pixel
    strh r0, [r4]

skip_pixel:
    // Update
    adds r4, #2         // ptr += 2
    adds r5, #1         // y++
    ldr r0, [sp, #20]  // step
    add r7, r7, r0     // tex_pos += step
    b sprite_loop

sprite_end:
    pop {r4, r5, r6, r7, pc}
