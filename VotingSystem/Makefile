all: serwer komisja raport

serwer: serwer.o err.o
	gcc -Wall -pthread -lrt err.o serwer.o -o serwer
	
komisja: komisja.o err.o
	gcc -Wall err.o komisja.o -o komisja

raport: raport.o err.o
	gcc -Wall err.o raport.o -o raport

serwer.o: serwer.c
	gcc -Wall -pthread -lrt -c serwer.c

komisja.o: komisja.c
	gcc -Wall -c komisja.c

raport.o: raport.c
	gcc -c raport.c

err.o: err.c
	gcc -Wall -c err.c

clean:
	rm -f *.o
	
.PHONY: all clean