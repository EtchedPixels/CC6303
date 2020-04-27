
	.export __cpu_to_le16
	.export __le16_to_cpu
	.export __be16_to_cpu
	.export __cpu_to_be16

	.setcpu 6803
	.code

__cpu_to_le16:
__le16_to_cpu:
	; The argument is top of stack 3,s-4,s
	tsx
	ldd 3,x
	stab 3,x
	staa 4,x
__cpu_to_be16:
__be16_to_cpu:
	rts
