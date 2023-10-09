@  Applet for checking bootloader functionality
@
@ Starting:
@ R0 = address of the beginning of this code (start point)
@ R1 = response buffer address
@
@ Returns the response 12
@

      .org    0	
      .byte   0,0 	  @ alignment bytes - cut off from the object module
      .byte   0x11,0      @ command code 11 and alignment byte - this remains in the object module

start: 
       MOV	R0,#0x12;
       STRB	R0,[R1]
       MOV	R4,#1
       BX       LR
