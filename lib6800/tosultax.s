;
;	Compare top of stack with D
;
;	True if TOS < D. We actually check this as D > TOS
;
		.export tosultax
		.code
tosultax:
		tsx
		cmpa 2,x
		bne noteq
		cmpb 3,x
noteq:		jsr boolugt		; we did the comparison backwards
		jmp pop2flags		; so do the bool generation backwards
