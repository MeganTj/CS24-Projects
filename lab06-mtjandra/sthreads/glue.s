#============================================================================
# Keep a pointer to the main scheduler context.  This variable should be
# initialized to %rsp, which is done in the __sthread_start routine.
#
        .data
        .align 8
scheduler_context:      .quad   0


#============================================================================
# __sthread_switch is the main entry point for the thread scheduler.
# It has three parts:
#
#    1. Save the context of the current thread on the stack.
#       The context includes all of the integer registers and RFLAGS.
#
#    2. Call __sthread_scheduler (the C scheduler function), passing the
#       context as an argument.  The scheduler stack *must* be restored by
#       setting %rsp to the scheduler_context before __sthread_scheduler is
#       called.
#
#    3. __sthread_scheduler will return the context of a new thread.
#       Restore the context, and return to the thread.
#
        .text
        .align 8
        .globl __sthread_switch
__sthread_switch:

        # Save the process state onto its stack
     	pushq    %rax
	    pushq    %rbx
		pushq    %rcx
		pushq    %rdx
		pushq    %rsi
		pushq    %rdi
		pushq    %rbp
		pushq    %r8
		pushq    %r9
		pushq    %r10
		pushq    %r11
		pushq    %r12
		pushq    %r13
		pushq    %r14
		pushq    %r15
		pushf
	
        # Call the high-level scheduler with the current context as an argument
        movq    %rsp, %rdi
        movq    scheduler_context(%rip), %rsp
        call    __sthread_scheduler

        # The scheduler will return a context to start.
        # Restore the context to resume the thread.
__sthread_restore:
	    # Moves the address of the context stored in the return
	    # address to rsp.
	    mov %rax, %rsp
		popf
		popq   %r15
	    popq   %r14
		popq   %r13
		popq   %r12
		popq   %r11
		popq   %r10
		popq   %r9
	    popq   %r8
	    popq   %rbp
		popq   %rdi
		popq   %rsi
		popq   %rdx
		popq   %rcx
		popq   %rbx
		popq   %rax
        ret


#============================================================================
# Initialize a process context, given:
#    1. the stack for the process
#    2. the function to start
#    3. its argument
# The context should be consistent with that saved in the __sthread_switch
# routine.
#
# A pointer to the newly initialized context is returned to the caller.
# (This is the thread's stack-pointer after its context has been set up.)
#
# This function is described in more detail in sthread.c.
#
#
        .align 8
        .globl __sthread_initialize_context
__sthread_initialize_context:
	    # Saves the current stack pointer in a caller saved register, as
 	    # the current rsp points to the old context.
	    movq    %rsp, %rcx
	
		# Changes rsp to stackp, the first argument stored in rdi
        movq    %rdi, %rsp

	    # Sets the return address of f to sthread_finish by pushing the
	    # address of sthread_finish onto the stack
		leaq __sthread_finish(%rip), %rax   
        pushq   %rax	

		# The ret at the end of sthread_switch should execute the
    	# specified function f with the argument arg, so we push
	    # the address of f, which is stored in rsi, onto the stack 
	    pushq   %rsi
	
	    # Set the registers (with the exception of rdi) and the rflags
	    # on the stack to 0.
	    subq    $0x8, %rsp		    # Sets the memory on the stack
	    movq    $0x0, (%rsp)        # corresponding with rax to 0.

	    subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with rbx to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with rcx to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with rdx to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with rsi to 0.

		subq    $0x8, %rsp			# Set the argument of f to arg by moving
	    movq    %rdx, (%rsp)		# rdx to the memory on the stack
							     	# corresponding with rdi

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)		# corresponding with rbp to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with r8 to 0.		

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with r9 to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with r10 to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with r11 to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with r12 to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with r13 to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with r14 to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
		movq    $0x0, (%rsp)        # corresponding with r15 to 0.

		subq    $0x8, %rsp			# Sets the memory on the stack
	    movq    $0x0, (%rsp)        # corresponding with rflags to 0.

	    # Sets the return address to the top of the stack
        movq   %rsp, %rax

	    # Restores rsp to the address of the old context
     	movq   %rcx, %rsp 	    
        ret


#============================================================================
# The start routine initializes the scheduler_context variable, and calls
# the __sthread_scheduler with a NULL context.
#
# The scheduler will return a context to resume.
#
        .align 8
        .globl __sthread_start
__sthread_start:
        # Remember the context
        movq    %rsp, scheduler_context(%rip)

        # Call the scheduler with no context
        movl    $0, %edi  # Also clears upper 4 bytes of %rdi
        call    __sthread_scheduler

        # Restore the context returned by the scheduler
        jmp     __sthread_restore

