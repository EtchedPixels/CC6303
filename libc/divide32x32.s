;	Workspace:
;	5-8,x	numerator		replaced with result
;	9-12,x	denominator
;	13-16,x	scratch			becomes remainder



div32x32:
		ldab #32
		stab @tmp
		ldd #0
		std 13,x		; Clear remainder/scratch
		std 15,x
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
		ldd 15,x
		subd 11,x
		std 15,x
		ldd 13,x
		sbcb 10,x
		sbca 9,x
		; Want to subtract
		bcc skip
		; No subtract, so put back the low 16bits we mushed
		ldd 15,x
		subd 11,x
		std 15,x
done:
		dec tmp
		bne loop
		rts
		; We do want to subtract - write back the other bits
skip:
		std 13,x
		dec tmp
		bne loop
		rts

