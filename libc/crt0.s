;
;	A dummy minimal crt0.s for now
;
	.code

	.export _exit

start:
	ldaa #1
	staa @one
	deca
	staa @zero
	staa @zero+1
	ldx #__bss
	ldaa #>__bss_size
	orab #<__bss_size
	beq nobss
	ldaa #>__bss_size
	ldab #<__bss_size
clear_bss:
	clr ,x
	inx
	decb
	bcc clear_bss
	deca
	bcc clear_bss
nobss:
	sts exitsp
	psha
	psha
	psha
	psha
	ldab #4
	jsr _main
	bra doexit

_exit:
	tsx
	ldaa 3,x
	ldab 4,x
doexit:
;
;	No atexit support yet
;
	lds exitsp
	rts

	.bss
exitsp:
	.word 0
