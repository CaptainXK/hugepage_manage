.PHONY:test,clean

test.app:main.o
	@gcc main.o -o test.app
test:test.app
	@$(EXEC) ./test.app
main.o:main.c
	@gcc -c main.c -g3
clean:
	@rm *.o test.app
