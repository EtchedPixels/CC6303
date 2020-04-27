;
;	A dummy minimal crt0.s for now
;
	.code

start:
	ldaa #1
	staa @one
	clra
	clrb
	psha
	pshb
	psha
	pshb
	jsr _main
	ins
	ins
	ins
	ins
	rts

