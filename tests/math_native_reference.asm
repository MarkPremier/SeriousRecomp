; Hardware reference only. Preserve the caller's complete FP/SSE environment.
.code
native_math_reference PROC FRAME
    sub rsp, 218h
    .allocstack 218h
    .endprolog
    fxsave [rsp]
    fninit
    fld1
    fxam                           ; seed C2=1 as the captured failure did
    fstp st(0)
    cmp ecx, 4
    je scale_case
    fld qword ptr [rdx]
    cmp ecx, 0
    je sine_case
    cmp ecx, 1
    je cosine_case
    cmp ecx, 2
    je sincos_case
    fptan
    jmp save_result
sine_case:
    fsin
    jmp save_result
cosine_case:
    fcos
    jmp save_result
sincos_case:
    fsincos
    jmp save_result
scale_case:
    fld qword ptr [rdx+8]
    fld qword ptr [rdx]
    fscale
save_result:
    fnstsw ax
    mov word ptr [r9], ax
    fstp qword ptr [r8]
    cmp ecx, 2
    jb done
    cmp ecx, 3
    ja done
    test ax, 400h
    jnz done
    fstp qword ptr [r8+8]
done:
    fxrstor [rsp]
    add rsp, 218h
    ret
native_math_reference ENDP
END
