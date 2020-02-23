;
;	A dummy minimal crt0.s for now
;
	.setcpu	6803

	.export zero
	.export one
	.export tmp

	.code

start:
	ldaa #1
	staa @onel
	ldx @zero
	pshx
	pshx
	jsr _main
	pulx
	pulx
	rts

	.zp
zero:
	.byte 0
one:
	.byte 0
onel:
	.byte 0
tmp:
	.word 0


