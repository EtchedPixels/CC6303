;
;	32bit unsigned divide. Used as the core for the actual C library
;	division routines. It expects to be called with the parameters
;	offsets from X, and uses tmp/tmp2/tmp3.
;
;	tmp2/tmp3 end up holding the remainder
;

DIVID		.equ	1
;		5/6,x are a return address and it's easier to just leave
;		the gap
DENOM		.equ	7

		.setcpu 6803

		.export div32x32
		.code

div32x32:
		ldab #32		; 32 iterations for 32 bits
		stab @tmp
		ldd @zero
		std @tmp2		; Clear remainder/scratch
		std @tmp3
loop:		; Shift the dividend
		lsl DIVID+3,x			; Shift 32bits left
		rol DIVID+2,x
		rol DIVID+1,x
		rol DIVID,x
		; Capture into the working register
		rol @tmp3+1		; capturing low bit into scratch
		rol @tmp3
		rol @tmp2+1
		rol @tmp2
		; Do a 32bit subtract but skip writing the high 16bits
		; back until we know the comparison
		ldd @tmp3
		subd DENOM+2,x
		std @tmp3
		ldd @tmp2
		sbcb DENOM+1,x
		sbca DENOM,x
		; Want to subtract
		bcc skip
		; No subtract, so put back the low 16bits we mushed
		ldd @tmp3
		addd DENOM+2,x
		std @tmp3
done:
		dec tmp
		bne loop
		rts
		; We do want to subtract - write back the other bits
skip:
		std @tmp2
		dec tmp
		bne loop
		rts
