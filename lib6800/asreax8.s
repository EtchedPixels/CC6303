;
;	Right shift the 32bit working register
;
	.export asreax8

	.code

asreax8:
	tab
	ldaa @sreg+1
	psha
	ldaa @sreg
	staa @sreg+1
	; Now the sign 
	bpl signok
	ldaa #$FF
	staa @sreg
	pula
	rts
signok:
	clr @sreg
	pula
	rts
