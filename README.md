# linux_lockfree

lock free list implementation
in linux kernel module


we made a simple singly circular list, and implemented push and pop operation with completly atomic functions __sync_bool_compare_and_swap()

using 4 threads,
insert time and delete time comparison is done

atomic operational lists seem to be 2x faster than original spinlock implemented lists

update: 12/07/2020
