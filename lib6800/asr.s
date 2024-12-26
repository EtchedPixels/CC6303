;
;	top of stack arithmetic shift right by D
;
	.export tosasrax
	.code
tosasrax:
	tstb
	beq ret
	tsx
asraxsh:
	asr 2,x
	ror 3,x
	decb
	bne asraxsh
ret:
	jmp pop2get

