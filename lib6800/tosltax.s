;
;	Compare top of stack with D
;

		.export tosltax
		.code
tosltax:
		tsx
		cmpa 2,x
		bne noteq
		cmpb 3,x
noteq:		jsr boolgt		; as we compare backwardss

		jmp pop2flags
