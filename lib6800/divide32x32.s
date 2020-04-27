;	Workspace:
;	5-8,x	numerator		replaced with result
;	9-12,x	denominator
;	13-16,x	scratch			becomes remainder

		.export div32x32

		.code
;
;	For speed we want to use sreg:@tmp1, probably also worth looking
;	for div32_16 cases and having a faster path.
;

div32x32:
		ldab #32
		stab @tmp
		clra
		clrb
		staa 13,x		; Clear remainder/scratch
		stab 14,x
		staa 15,x
		stab 16,x
loop:		; Shift the dividend
		lsl 8,x			; Shift 32bits left
		rol 7,x
		rol 6,x
		rol 5,x
		; Capture into the working register
		rol 16,x		; capturing low bit into scratch
		rol 15,x
		rol 14,x
		rol 13,x
		; Do a 32bit subtract but skip writing the high 16bits
		; back until we know the comparison
		ldaa 15,x
		ldab 16,x
		subb 12,x
		sbca 11,x
		staa 15,x
		stab 16,x
		ldaa 13,x
		ldab 14,x
		sbcb 10,x
		sbca 9,x
		; Want to subtract
		bcc skip
		; No subtract, so put back the low 16bits we mushed
		ldaa 15,x
		ldab 16,x
		subb 12,x
		sbca 11,x
		staa 15,x
		stab 16,x
done:
		dec tmp
		bne loop
		rts
		; We do want to subtract - write back the other bits
skip:
		staa 13,x
		stab 14,x
		dec tmp
		bne loop
		rts

