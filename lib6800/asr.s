;
;	top of stack arithmetic shift right by D
;
	.export tosasrax
	.code
tosasrax:
	cmpb #$8
	beq asrax8
	bcc asraxsh
	tab
	clra
	andb #7
asraxsh:
	tsx
	asr 1,x
	ror 2,x
	decb
	bne asraxsh
	rts
asrax8:
	tab
	clra
	rts


