	.export loadtos
	.export savetos
	.export addtotos
	.export addtotosb

	.code

loadtos:
	tsx
	ldab $03,x
	ldaa $02,x
	inx			; so X is as the caller expects it
	inx
	rts

savetos:
	tsx
	stab $03,x
	staa $02,x
	inx			; so X is as the caller expects it
	inx
	rts

addtotosb:
	clra
addtotos:
	tsx
	addb $03,x
	adca $02,x
	stab $03,x
	staa $02,x
	inx
	inx
	rts
