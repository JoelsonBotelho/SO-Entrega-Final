CFLAGS = -Wall -g -pedantic

all: banco banco-terminal

banco-terminal: commandlinereader.o banco-terminal.o
	gcc -o banco-terminal commandlinereader.o banco-terminal.o

banco: commandlinereader.o contas.o banco.o
	gcc -o banco -pthread commandlinereader.o contas.o banco.o

commandlinereader.o: commandlinereader.c
	gcc $(CFLAGS) -c commandlinereader.c
	
banco-terminal.o: banco-terminal.c commandlinereader.h
	gcc $(CFLAGS) -c banco-terminal.c

banco.o: banco.c commandlinereader.h contas.h
	gcc $(CFLAGS) -c banco.c

contas.o: contas.c contas.h
	gcc $(CFLAGS) -pthread -c contas.c

clean:
	rm -f *.o banco
	rm -f *.o banco-terminal
	rm -f banco-pipe
	rm -f log.txt
	rm -f pipe-*
	rm -f banco-sim-*

run:
	./banco
