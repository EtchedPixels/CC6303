;
;	top of stack arithmetic shift right by D
;
	.export tosasrax
	.code
tosasrax:
	tsx
asraxsh:
	asr 2,x
	ror 3,x
	decb
	bne asraxsh
	jmp pop2get

