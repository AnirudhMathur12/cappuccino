#ifndef CAPPUCCINO_STDLIB_H
#define CAPPUCCINO_STDLIB_H

#include <string>

const std::string STDLIB_ASM = R"(
.section __TEXT,__text,regular,pure_instructions

// ==========================================
//              I/O Functions
// ==========================================

// --- print_s(string) ---
.globl _print_s
.p2align 2
_print_s:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    bl _puts                ; Use puts() - safer than printf for plain strings
    ldp x29, x30, [sp], #16
    ret

// --- print(int) ---
.globl _print
.p2align 2
_print:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    
    // printf("%ld\n", val)
    mov x1, x0              ; Standard: Arg 2 in x1
    str x0, [sp, #-16]!     ; Apple: Arg 2 on stack
    
    adrp x0, l_fmt_int@PAGE
    add x0, x0, l_fmt_int@PAGEOFF
    
    bl _printf
    
    add sp, sp, #16         ; Cleanup stack
    ldp x29, x30, [sp], #16
    ret

// --- print_f(float) ---
.globl _print_f
.p2align 2
_print_f:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    
    str d0, [sp, #-16]!     ; Apple: Float arg on stack
    
    adrp x0, l_fmt_flt@PAGE
    add x0, x0, l_fmt_flt@PAGEOFF
    
    bl _printf
    
    add sp, sp, #16
    ldp x29, x30, [sp], #16
    ret

// --- input_f() -> float ---
.globl _input_f
.p2align 2
_input_f:
    sub sp, sp, #32         ; Alloc result slot + frame
    stp x29, x30, [sp, #16]
    add x29, sp, #16
    
    add x1, sp, #8          ; Standard: Pointer in x1
    str x1, [sp]            ; Apple: Pointer on stack
    
    adrp x0, l_fmt_scan_flt@PAGE
    add x0, x0, l_fmt_scan_flt@PAGEOFF
    
    bl _scanf

    ldr d0, [sp, #8]
    
    ldp x29, x30, [sp, #16]
    add sp, sp, #32
    ret

// --- input_i() -> int ---
.globl _input_i
.p2align 2
_input_i:
    sub sp, sp, #32
    stp x29, x30, [sp, #16]
    add x29, sp, #16

    // Scanf
    add x1, sp, #8
    str x1, [sp]
    
    adrp x0, l_fmt_scan_int@PAGE
    add x0, x0, l_fmt_scan_int@PAGEOFF
    
    bl _scanf

    ldr x0, [sp, #8]
    ldp x29, x30, [sp, #16]
    add sp, sp, #32
    ret

// ==========================================
//              Math Wrappers
// ==========================================
.globl _sqrt_f
_sqrt_f:
    b _sqrt
.globl _sin_f
_sin_f:
    b _sin
.globl _cos_f
_cos_f:
    b _cos
.globl _tan_f
_tan_f:
    b _tan
.globl _abs_f
_abs_f:
    fabs d0, d0
    ret

// ==========================================
//              Data Section
// ==========================================
.section __TEXT,__cstring,cstring_literals
l_fmt_int:      .asciz "%ld\n"
l_fmt_flt:      .asciz "%f\n"
l_fmt_scan_int: .asciz "%ld"
l_fmt_scan_flt: .asciz "%lf"
)";

#endif
