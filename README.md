# [MIT-6.S081 Fall 2021](https://pdos.csail.mit.edu/6.S081/2021/schedule.html)

## [Tools](https://pdos.csail.mit.edu/6.S081/2021/tools.html)

## [Debug](https://pdos.csail.mit.edu/6.S081/2021/lec/gdb_slides.pdf)
1. Use *make* to start QEMU with gdb
```
make CPUS=1 qemu-gdb
```

2. In another termial to connect to QEMU
```
riscv64-unknown-elf-gdb
```

3. Set break point to _entry
```
b _entry
```

## [Extended Asm - Assembler Instructions with C Expression Operands](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html)
1. [Output Operands](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#OutputOperands)
2. [Input Operands](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#InputOperands)