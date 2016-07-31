CC = x86_64-zoidberg-gcc
CFLAGS = -ffreestanding ${INCLUDES}

default: mysh

mysh: lexer.o shell.o parser.o astree.o execute.o command.o
	$(CC) $(APPCFLAGS) parser.o lexer.o astree.o execute.o command.o shell.o -lc -lgcc -o mysh

command.o: command.c
	$(CC) $(CFLAGS) -c command.c
	
shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c
	
execute.o: execute.c execute.h
	$(CC) $(CFLAGS) -c execute.c
	
parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

lexer.o: lexer.h lexer.c
	$(CC) $(CFLAGS) -c lexer.c 
	
astree.o: astree.c astree.h
	$(CC) $(CFLAGS) -c astree.c 

clean: 
	rm *.o

