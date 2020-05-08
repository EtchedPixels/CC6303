
	.export storedpush
	.export pshtop
	.code
;
;	Store D and push it
;
storedpush:
	staa 3,x		; save D at top of stack (as seen by caller)
	stab 4,x
pshtop:
	ldx 1,x			; get return address
	stx @tmp		; patch it in
	tsx
	ldx 3,x			; get saved D
	stx 1,x			; overwrite return address
	jmp jmptmp		; to caller without adjusting stack


