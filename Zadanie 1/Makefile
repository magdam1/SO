all: Pascal W

Pascal: Pascal.o err.o
	gcc -Wall Pascal.o err.o -o Pascal
	
W: W.o err.o
	gcc -Wall W.o err.o -o W

Pascal.o: Pascal.c
	gcc -Wall -c Pascal.c -o Pascal.o

W.o: W.c
	gcc -Wall -c W.c -o W.o

	
err.o: err.c err.h
	gcc -Wall -c err.c -c err.h

clean:
	rm -f *.o *.gch
	
.PHONY: all clean