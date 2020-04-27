
	.export __cpu_to_le32
	.export __le32_to_cpu
	.export __be32_to_cpu
	.export __cpu_to_be32

	.setcpu 6803
	.code

__cpu_to_le32:
__le32_to_cpu:
	; The argument is top of stack 3,s-6,s
	tsx
	ldx 3,x
	pshx
	ldd 5,x
	stab 3,x
	staa 4,x
	pulb
	pula
	std 5,x
__cpu_to_be32:
__be32_to_cpu:
	rts
