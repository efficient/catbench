.globl cpuid
# %rdi : cpuid_t *buf
# %esi : uint32_t leaf
# %edx : uint32_t subleaf
cpuid:
	pushq %rbx

	movl  %esi, %eax
	movl  %edx, %ecx
	cpuid
	movl  %eax,  0(%rdi)
	movl  %ebx,  4(%rdi)
	movl  %ecx,  8(%rdi)
	movl  %edx, 12(%rdi)

	popq  %rbx
	ret
