;
;	A dummy minimal crt0.s for now
;
	.code
start:
	ldx #__bss_size
	beq nobss
	ldx #__bss
	ldab #<__bss
	ldaa #>__bss
	addb #<__bss_size
	adca #>__bss_size
	stab @tmp+1
	staa @tmp
	clrb
	bra clear_bss_1
clear_bss:
	stab 0,x
	inx
clear_bss_1:
	cpx @tmp
	bne clear_bss
nobss:
	sts exitsp
	lds #$7fff
	;
	; Runtime DP constants
	;
	clra
	staa @zero
	staa @zero+1
	inca
	staa @one+1
	clra
	psha
	psha
	psha
	psha
	jsr _main
;
;	No atexit support yet
;
doexit:
	lds exitsp
	stab $FEFF
	rts
	.bss
exitsp:
	.word 0
