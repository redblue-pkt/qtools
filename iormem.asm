@ Applet for reading data from modem memory using command 0x11
@
@ Input:
@ R0 = start address of this code (label start)
@ R1 = address of the response buffer
@
@

.org    0	
.byte   0,0      @ alignment bytes - trimmed from the object module
.byte   0x11,0   @ command code 0x11 and an alignment byte - retained in the object module

start: 
    LDR    R3, srcadr    @ R3 = address of the data to be read from the modem memory
    LDR    R4, lenadr    @ R4 = size of the data to be written in bytes
    MOV    R0, #0x12     @ response code 
    STR    R0, [R1], #4  @ save it in the response buffer
    ADD    R0, R3, R4    @ R0 = end address of the data block

locloop:   
    LDR    R2, [R3], #4  
    STR    R2, [R1], #4
    CMP    R3, R0
    BCC    locloop
    ADD    R4, #4        @ response size - 4 bytes for code and data
    BX     LR

srcadr: .word 0
lenadr: .word 0
