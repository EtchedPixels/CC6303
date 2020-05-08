;
;	6800 stack manipulation can get ugly
;

		.export des16
		.export des14
		.export des12
		.export des10
		.export des8
		.export des6

		.code
des16:
		tsx
		ldx 1,x
		stx @tmp
		des
		des
do14:
		des
		des
do12:
		des
		des
do10:
		des
		des
do8:
		des
		des
do6:
		des
		des
		des
		des
		des
		des
		jmp jmptmp
des14:
		tsx
		ldx 1,x
		stx @tmp
		bra do14
des12:
		tsx
		ldx 1,x
		stx @tmp
		bra do12
des10:
		tsx
		ldx 1,x
		stx @tmp
		bra do10
des8:
		tsx
		ldx 1,x
		stx @tmp
		bra do8
des6:
		tsx
		ldx 1,x
		stx @tmp
		bra do6
