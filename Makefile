all: trace

trace: LC4.o loader.o trace1.c
	clang -g LC4.o loader.o trace1.c -o trace

LC4.o:
	clang LC4.c -o LC4.o -c

loader.o: 
	clang loader.c -o loader.o -c

clean:
	rm -rf *.o

clobber: clean
	rm -rf trace
