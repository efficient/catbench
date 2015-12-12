.globl movntq4
# %rdi : uint64_t *dest
# %rsi : uint64_t a
# %rdx : uint64_t b
# %rcx : uint64_t c
# %r8  : uint64_t d
movntq4:
	movnti %rsi,  0(%rdi)
	movnti %rdx,  8(%rdi)
	movnti %rcx, 16(%rdi)
	movnti %r8,  24(%rdi)
	ret
