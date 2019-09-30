	# Compares the value of *target, in edi, to old, in esi. If they are
	# the  same it stores new, in edx, into target and returns 1.
	# Otherwise, the function does not modify *target and returns 0.
	.globl compare_and_swap
compare_and_swap:
	movl   %esi, %eax                 # First move old_value into eax
	lock
	cmpxchgl   %edx,  (%rdi) # Then compare old_value and target, target
                       # being the destination and new_value being the source
	jz zero        # If zero flag is set, return 1. Otherwise, return 0
	movl  $0x0, %eax
	ret
zero:	
	movl  $0x1, %eax
	ret
	

