;
;	top of stack arithmetic shift right by D
;
	.export tosasrax
	.code
tosasrax:
	tsx
asraxsh:
	asr 1,x
	ror 2,x
	decb
	bne asraxsh
	rts


