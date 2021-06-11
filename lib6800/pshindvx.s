;
;	Push indirected via X
;
	.export pshindvx
	.export pshindvx1
	.export pshindvx2
	.export pshindvx3
	.export pshindvx4
	.export pshindvx5
	.export pshindvx6
	.export pshindvx7
	.code

pshindvx:
	pula
	pulb
	staa @tmp
	stab @tmp+1
	ldab $01,x
	ldaa $00,x
	pshb
	psha
	jmp @jmptmp
pshindvx1:
	pula
	pulb
	staa @tmp
	stab @tmp+1
	ldab $02,x
	ldaa $01,x
	pshb
	psha
	jmp @jmptmp
pshindvx2:
	pula
	pulb
	staa @tmp
	stab @tmp+1
	ldab $03,x
	ldaa $02,x
	pshb
	psha
	jmp @jmptmp
pshindvx3:
	pula
	pulb
	staa @tmp
	stab @tmp+1
	ldab $04,x
	ldaa $03,x
	pshb
	psha
	jmp @jmptmp
pshindvx4:
	pula
	pulb
	staa @tmp
	stab @tmp+1
	ldab $05,x
	ldaa $04,x
	pshb
	psha
	jmp @jmptmp
pshindvx5:
	pula
	pulb
	staa @tmp
	stab @tmp+1
	ldab $06,x
	ldaa $05,x
	pshb
	psha
	jmp @jmptmp
pshindvx6:
	pula
	pulb
	staa @tmp
	stab @tmp+1
	ldab $07,x
	ldaa $06,x
	pshb
	psha
	jmp @jmptmp
pshindvx7:
	pula
	pulb
	staa @tmp
	stab @tmp+1
	ldab $08,x
	ldaa $07,x
	pshb
	psha
	jmp @jmptmp
