# The output function sets up argument calls according to the specified
# system-call convention and uses the syscall instruction to trap into the
# kernel and outputs the message with a call to write()
.globl output
output:
	# The address of the msg is initially in rdi
	# The size of the msg is initially in rsi
	movq	 $0x1, %rax  # System-call number of write is 1
	movq	 %rsi, %rdx  # Pass in the number of bytes to write as the
						 # third argument.
	movq	 %rdi, %rsi  # Pass in the address of the message to write out
						 # as the second argument.
	movq	 $0x1, %rdi  # Pass in 1 for the file descriptor as the first
						 # argument.
	syscall
	retq
	
	
