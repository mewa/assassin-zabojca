CC=mpicc
CFLAGS=-Wall

build: obj
	$(CC) $(CFLAGS) *.o -o zabojcy

obj:
	$(CC) $(CFLAGS) -c $(wildcard *.c)

clean:
	rm -f $(wildcard *.o *~)
