;
;	Stack unwind helpers
;
	.export pop4
	.export tpop4
	.export swap32pop4
	.export pop4flags

	.setcpu 6803
	.code

swap32pop4:
	std @sreg
tpop4:
	ldd @tmp
pop4:
	pulx
pop4h:
	ins
	ins
	ins
	ins
	jmp ,x

pop4flags:
	pulx
	tstb
	bra pop4h
