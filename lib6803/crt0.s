;
;	A dummy minimal crt0.s for now
;
	.setcpu	6803

	.code

start:
	clra
	clrb
	std @zero
	inca
	staa @one+1
	ldx #__bss
	ldd #__bss_size
	beq nobss
clear_bss:
	clr ,x
	inx
	subd #1
	bne clear_bss
nobss:
	lds #stack
	ldx @zero
	pshx
	pshx
	jsr _main
	pulx
	pulx
	rts

	.data

	.ds 511
stack:
	.byte 0
