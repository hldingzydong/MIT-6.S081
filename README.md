## [MIT-6.S081 Fall 2020](https://pdos.csail.mit.edu/6.S081/2020/schedule.html)

## Reference Count For Each Page
- index the array with the page's physical address divided by 4096
- give the array a number of elements equal to highest physical address of any page placed on the free list by kinit() in kalloc.c
- kfree() should only place a page back on the free list if its reference count is zero

## Copy
- Modify uvmcopy() to map the parent's physical pages into the child, instead of allocating new pages. Clear PTE_W in the PTEs of both child and parent

## On Write
- Modify usertrap() to recognize page faults. When a page-fault occurs on a COW page, allocate a new page with kalloc(), copy the old page to the new page, and install the new page in the PTE with PTE_W set


## Other
- Modify copyout() to use the same scheme as page faults when it encounters a COW page
- It may be useful to have a way to record, for each PTE, whether it is a COW mapping. You can use the RSW (reserved for software) bits in the RISC-V PTE for this
- If a COW page fault occurs and there's no free memory, the process should be killed