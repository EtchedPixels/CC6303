;
;	Compare 32bit with 0. Should preserve D ?
;
	.export tsteax
	.export utsteax

tsteax:		; sign doesn't matter for equality
utsteax:
	subd	@zero	; we don't have a 'tstd' but this is the same
	bne	done	; flag set and nothing touched
	pshb	; Push and pop it so we don't disturb the NE/EQ flags
	psha
	ldd	@sreg	; will set eq/ne for sreg being 0
	pula	; Restore without touching flags (ldd affects Z bit)
	pulb
done:
	rts
