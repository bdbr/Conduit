CC = gcc
CFLAGS = -g -O3 -ffast-math -Wall -pthread
LDLIBS = -lm

all: test_conduit test_pipe
		
test_pipe: test_pipe.c conduct.c 
	$(CC) $(CFLAGS) -o test_pipe test_pipe.c conduct.c 
	
test_conduit: test_conduit.c conduct.c 
	$(CC) $(CFLAGS) -o test_conduit test_conduit.c conduct.c 

clean:
	rm -rf julia conduct  test_pipe test_conduit  *~ *.o
