ls: ls.o
	gcc -o ls ls.o

ls.o: ls.c
	gcc -c ls.c

clean:
	rm -f *.o ls
