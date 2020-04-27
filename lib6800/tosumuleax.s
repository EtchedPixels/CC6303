;
;	Top of stack (1,x) * sreg:d
;
		.export tosumuleax

		.code
tosumuleax:
		tsx
		jsr umul32x32	; does the actual work
		jmp pop4	; and discard the argument
