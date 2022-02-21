;
;	Compare top of stack with D
;

		.export tosugeax
		.code
tosugeax:
		tsx
		cmpa 2,x
		bne noteq
		cmpb 3,x
noteq:		jsr boolule		; we did the comparison backwards
		jmp pop2flags
