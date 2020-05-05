
		.export _putchar
		.code

_putchar:
		tsx
		ldab 4,x
		jmp __putc

