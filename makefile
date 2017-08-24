.PHONY:test,clean

test.app:main.o sysfs_ops.o hugepage_memory.o
	@gcc *.o -o test.app
test:test.app
	@$(EXEC) ./test.app
main.o:main.c
	@gcc -c main.c -g3
sysfs_ops.o:sysfs_ops.c
	@gcc -c sysfs_ops.c -g3
hugepage_memory.o:hugepage_memory.c
	@gcc -c hugepage_memory.c -g3

clean:
	@rm *.o test.app
