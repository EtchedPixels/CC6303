;
;	A minimal crt0.s for now
;
;	TODO: work out whether we should use the provided stack or allocate
;	one
;
	.code

	.export _exit

start:
	ldaa #1
	staa @one
	deca
	staa @zero
	staa @zero+1
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
