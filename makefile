.PHONY:test,clean

test.app:main.o sysfs_ops.o hugepage_memory.o hugepage_malloc.o runtime_info.o
	@gcc *.o -o test.app
test:test.app
	@$(EXEC) ./test.app
main.o:main.c
	@gcc -c main.c -g3
sysfs_ops.o:sysfs_ops.c
	@gcc -c sysfs_ops.c -g3
hugepage_memory.o:hugepage_memory.c
	@gcc -c hugepage_memory.c -g3
hugepage_malloc.o:hugepage_malloc.c
	@gcc -c hugepage_malloc.c -g3
runtime_info.o:runtime_info.c
	@gcc -c runtime_info.c -g3

clean:
	@rm *.o test.app
