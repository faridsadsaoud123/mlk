all: mlk shell.so foo.so bar.so

mlk : my_litle_kernel.c scc.so syscall.h
	gcc my_litle_kernel.c scc.so -ldl -o mlk
scc.so: scc.o
	gcc -shared -o scc.so scc.o
%.so: %.o
	gcc -shared -o $@ $< scc.so
%.o: %.c
	gcc -fPIC -c $<

