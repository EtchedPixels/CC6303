;
;	Top of stack (1,x) * sreg:d
;
		.export tosmuleax
		.export tosumuleax

		.code
tosumuleax:
tosmuleax:
		tsx
		jsr umul32x32	; does the actual work
		jmp pop4	; and discard the argument
