
		.export storetos
		.code

storetos:
		tsx
		staa $02,x
		stab $03,x
		inx
		inx
		rts
