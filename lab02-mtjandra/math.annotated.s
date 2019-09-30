/* This file contains x86-64 assembly-language implementations of three
 * basic, very common math operations.
 *
 * We are avoiding using labels and jumping to implement conditionals. This is
 * because conditional data transfers outperform code based on conditional control
 * transfers. The compiler would want to avoid the naive implementation with conditional
 * control transfers because compilers pipeline, which is overlapping steps of successive
 * instructions. This involves being able to determine the sequence of instructions to be
 * executed ahead of time. However, when a compiler sees a conditional jump, it won't be
 * able to tell which way the branch will go until the condition is evaluated. When, a
 * compiler mispredicts a jump, it has to discard much of its work for one branch and
 * instead fill the pipeline starting at the correct location.
 */

        .text

/*====================================================================
 * int f1(int x, int y)
 *
 * Finds the minimum of x and y. Subtracts x and y, and if x is greater than y,
 * y replaces x.
 */
.globl f1
f1:
        movl    %edi, %edx	# Moves the contents of edi (x)  into edx.	
        movl    %esi, %eax	# Moves the contents of esi (y)  into eax
        cmpl    %edx, %eax	# Subtracts the contents of eax (y) and edx (x) and updates flags.
        cmovg   %edx, %eax	# If eax is greater than edx, moves the contents of edx to eax
        ret


/*====================================================================
 * int f2(int x)
 * 
 * Takes the absolute value of x.
 */
.globl f2
f2:
        movl    %edi, %eax	# Moves the contents of edi (x)  into eax
        movl    %eax, %edx	# Moves the contents of eax into edx. now x is in edx.
        sarl    $31, %edx	# Shifts the contents of edx 31 to the right and preserves the sign of x.
	# The most significant bit of x (the signed bit) becomes the least significant bit.
        xorl    %edx, %eax	# Does XOR on the contents of edx and eax so that x is always positive.
        subl    %edx, %eax	# Subtracts eax from edx. 
	# This has the effect of adding 1 if x is a negative number, which is correct as there is one more negative
	# number than positive.
        ret


/*====================================================================
 * int f3(int x)
 *
 * Returns 1 is edx is positive and -1 if edx is zero or below. 
 */
.globl f3
f3:
        movl    %edi, %edx	# Moves the contents of edi (x)  into edx
        movl    %edx, %eax	# Moves the contents of edx into eax
        sarl    $31, %eax	# Shifts the contents of eax 31 to the right and preserves its sign.
	# The most significant bit of x (the signed bit) becomes the least significant bit.
	# If x is negative, eax would contain -1.
        testl   %edx, %edx	# Tests whether edx (x) is above, below, or equals 0
        movl    $1, %edx	# Sets edx to 1
        cmovg   %edx, %eax	# Sets eax to 1 if edx is greater than 0
        ret

