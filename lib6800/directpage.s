;
;	Runtime support
;

	.zp
	.export zero
	.export one
	.export tmp
	.export tmp1
	.export tmp2
	.export sreg
	.export fp
	.export reg

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
tmp2:
	.word 0
tmp3:
	.word 0
tmp4:
	.word 0
sreg:			; Upper 16bits of working long
	.word 0
fp:
	.word 0		; frame pointer for varargs
reg:
	.word 0		; For now allow for 3 words of registers
	.word 0
	.word 0
