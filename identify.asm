@  Applet reading the words identifying the modem chipset through command 11
@
@ At the entrance:
@ R0 = address of the beginning of this code (start point)
@ R1 = response buffer address
@
@

      .org    0	
      .byte   0,0 	  @ alignment bytes - cut off from the object module
      .byte   0x11,0      @ command code 11 and alignment byte - this remains in the object module

start: 
       PUSH	{R1}
       MOV	R0,LR		@ address of command handler 11 in the bootloader
       BIC	R0,#3           @ round at the word boundary
       ADD	R3,R0,#0xFF	@ pattern search boundary
       LDR	R1,=0xDEADBEEF  @ search pattern
floop:                          
       LDR	R2,[R0],#4      @ another word
       CMP	R2,R1           @ is this a sample?
       BEQ	found           @ yes - finally found
       CMP	R0,R3           @ have you reached the border?
       BCC	floop           @ no - we are looking further
@ Образец не найден             
       MOV	R0,#0           @ answer 0 - sample not found
       B	done            
@ Нашли образец
found:                          
       LDR	R0,[R0]         @ remove the chipset code
done:             
       POP	{R1}
       STRB	R0,[R1,#1]      @ save the code in response byte 1
       MOV	R0,#0xAA        @ AA - identification mode response code
       STRB	R0,[R1]         @ in byte 0
       MOV	R4,#2           @ response size - 2 bytes
       BX       LR              
                               
