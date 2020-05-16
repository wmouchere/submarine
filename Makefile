CC=gcc
CFLAGS=-std=gnu11 -O2
DEPS=cacheutils.h communication.h
OBJ=cacheutils.o communication.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: sender receiver test_sender test_receiver test_compare

sender: sender.o $(OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

receiver: receiver.o $(OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

test_sender: test_sender.o $(OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

test_receiver: test_receiver.o $(OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

test_compare: test_compare.o $(OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o sender receiver test_sender test_receiver
