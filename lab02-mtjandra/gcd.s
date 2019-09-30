	# The gcd function recursively computes the GCD of two non-negative
	# numbers using Euclid's algorithm. Takes in two arguments in rdi and
	# rsi. rdi contains the greater argument and rsi the lower.
	.globl gcd
gcd:
	orl $0, %esi		# Sets zero flag if lower argument is 0
	jnz gcd_continue	# Divide greater and lower argument if lower is nonzero
	jmp gcd_return

gcd_continue:
	movl	%edi, %eax	# Move the greater argument into eax
	cqto
	divl	%esi		# Divides the greater by the lower and stores remainder.
	movl	%esi, %edi	# The lower number becomes the greater number 
	movl	%edx, %esi  	# The remainder becomes the lower number
	call	gcd
	
gcd_return:
	movl	%edi, %eax
	ret			# All done

