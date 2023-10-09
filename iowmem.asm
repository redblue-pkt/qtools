@ Applet for writing data to modem memory using command 0x11
@
@ Input:
@ R0 = start address of this code (label start)
@ R1 = address of the response buffer
@

.org    0   
.byte   0,0,0x11,0   @ command code 0x11 and alignment bytes
start: 
    ADD    R0, R0, #(param - start) @ address where data for writing begins
    LDR    R3, dstadr              @ address where data should be written in the modem
    LDR    R4, lenadr              @ size of the data to be written in bytes
    ADD    R4, R3, R4              @ end address of the data block

locloop:   
    LDR    R2, [R0], #4    
    STR    R2, [R3], #4
    CMP    R3, R4
    BCC    locloop
    MOV    R0, #0x12          @ response code 
    STRB   R0, [R1]           @ save it in the response buffer
    MOV    R4, #1             @ response size
    BX     LR

dstadr: .word 0
lenadr: .word 0
param:
