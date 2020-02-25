;
;	Runtime support
;

	.zp
	.export zero
	.export one
	.export tmp
	.export tmp1
	.export sreg

zero:
	.byte 0		; Patterns that form a word 0
one:
	.byte 0		; And a word 1
	.byte 0		; patched to 1 by runtime
;
;	These we need to switch on interrupt
;
tmp:
	.word 0		; Temporaries used by compiler
tmp1:
	.word 0
tmp2:			; Spare for now 2-4
	.word 0
tmp3:
	.word 0
tmp4:
	.word 0
sreg:			; Upper 16bits of working long
	.word 0
