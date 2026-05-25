.MODEL SMALL
.STACK 100h

DATA SEGMENT
    x DW 0  ; variable
DATA ENDS

CODE SEGMENT
    ASSUME CS:CODE, DS:DATA

; === ??????? ????????? ===

; === ???????: conditional ===
conditional PROC
        PUSH BP
        MOV BP, SP
        CALL ReadNumber
        MOV [x], AX
        MOV AX, [x]
        PUSH AX
        MOV AX, 0
        POP BX
        CMP BX, AX
        JLE L1
    ; print
        MOV AX, 1
        PUSH AX
        CALL PrintNumber
        POP AX

        JMP L0
L1:
    ; print
        MOV AX, 0
        PUSH AX
        CALL PrintNumber
        POP AX

L0:
_epilogue_conditional:
        POP BP
        RET
conditional ENDP
START:
        MOV AX, DATA
        MOV DS, AX

        CALL conditional

        MOV AH, 08h
        INT 21h

        MOV AH, 4Ch
        INT 21h


; === ????????? ?????? ?????? ????? (PrintNumber) ===
PrintNumber PROC
    PUSH AX
    PUSH BX
    PUSH CX
    PUSH DX
    PUSH SI
    MOV BX, 10
    MOV CX, 0
    MOV SI, SP
    CMP AX, 0
    JGE PrintNumber_positive
    NEG AX
    PUSH AX
    MOV AH, 02h
    MOV DL, '-'
    INT 21h
    POP AX
PrintNumber_positive:
PrintNumber_convert:
    XOR DX, DX
    DIV BX
    ADD DL, '0'
    PUSH DX
    INC CX
    CMP AX, 0
    JNE PrintNumber_convert
PrintNumber_print:
    POP DX
    MOV AH, 02h
    INT 21h
    LOOP PrintNumber_print
    POP SI
    POP DX
    POP CX
    POP BX
    POP AX
    RET
PrintNumber ENDP

; === ????????? ????? ?????? ????? (ReadNumber) ??? 8086 ===
ReadNumber PROC
    PUSH BX
    PUSH CX
    PUSH DX
    PUSH SI
    XOR AX, AX
    PUSH AX
    XOR SI, SI
ReadNumber_loop:
    MOV AH, 01h
    INT 21h
    CMP AL, 0Dh
    JE ReadNumber_done
    CMP AL, '-'
    JNE ReadNumber_digit
    MOV SI, 1
    JMP ReadNumber_loop
ReadNumber_digit:
    CMP AL, '0'
    JB ReadNumber_loop
    CMP AL, '9'
    JA ReadNumber_loop
    SUB AL, '0'
    XOR BX, BX
    MOV BL, AL
    POP AX
    MOV CX, 10
    MUL CX
    ADD AX, BX
    PUSH AX
    JMP ReadNumber_loop
ReadNumber_done:
    POP AX
    CMP SI, 0
    JE ReadNumber_exit
    NEG AX
ReadNumber_exit:
    POP SI
    POP DX
    POP CX
    POP BX
    RET
ReadNumber ENDP

CODE ENDS

END START