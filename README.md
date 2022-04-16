# [MIT-6.S081 Fall 2020](https://pdos.csail.mit.edu/6.S081/2020/schedule.html)

## Question: why need to backup whole trapframe
### Debug the reason of backup trapframe by break into:
```c
uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  *(p->trapframe) = *(p->backuptrapframe); // set breakpoint here
  p->prevfinished = 1;
  return 0;
}
```

#### p->trapframe
```
(gdb) p p->trapframe
$2 = (struct trapframe *) 0x87f62000
```
```
(gdb) p *(struct trapframe *)0x87f62000
$3 = {kernel_satp = 9223372036855332863, 
  kernel_sp = 274877882368, kernel_trap = 2147494002, 
  epc = 1762, kernel_hartid = 0, ra = 50, sp = 12160, 
  gp = 361700864190383365, tp = 361700864190383365, 
  t0 = 361700864190383365, t1 = 361700864190383365, 
  t2 = 361700864190383365, s0 = 12176, s1 = 11100490, a0 = 1, 
  a1 = 11919, a2 = 1, a3 = 16032, a4 = 4114, a5 = 10, 
  a6 = 17592186044416, a7 = 23, s2 = 1000000, s3 = 500000000, 
  s4 = 3352, s5 = 2992, s6 = 361700864190383365, 
  s7 = 361700864190383365, s8 = 361700864190383365, 
  s9 = 361700864190383365, s10 = 361700864190383365, 
  s11 = 361700864190383365, t3 = 361700864190383365, 
  t4 = 361700864190383365, t5 = 361700864190383365, 
  t6 = 361700864190383365}
```


#### p->backuptrapframe
```
(gdb) p p->backuptrapframe
$4 = (struct trapframe *) 0x87f73000
```
```
(gdb) p *(struct trapframe *)0x87f73000
$5 = {kernel_satp = 9223372036855332863, 
  kernel_sp = 274877882368, kernel_trap = 2147494002, 
  epc = 290, kernel_hartid = 0, ra = 316, sp = 12176, 
  gp = 361700864190383365, tp = 361700864190383365, 
  t0 = 361700864190383365, t1 = 361700864190383365, 
  t2 = 361700864190383365, s0 = 12240, s1 = 11100490, a0 = 1, 
  a1 = 2992, a2 = 1, a3 = 16032, a4 = 5120, a5 = 0, 
  a6 = 17592186044416, a7 = 16, s2 = 1000000, s3 = 500000000, 
  s4 = 3352, s5 = 2992, s6 = 361700864190383365, 
  s7 = 361700864190383365, s8 = 361700864190383365, 
  s9 = 361700864190383365, s10 = 361700864190383365, 
  s11 = 361700864190383365, t3 = 361700864190383365, 
  t4 = 361700864190383365, t5 = 361700864190383365, 
  t6 = 361700864190383365}
```

#### Diff Registers
- epc
- ra
- sp
- s0
- a1
- a4
- a5
- a7