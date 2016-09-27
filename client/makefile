C_SOURCES = $(wildcard *.c)
OBJS = $(C_SOURCES:.c=.o)
EXE = main

L_FLAGS=-g -Wall
B_FLAGS=-g -Wall -c

CC=gcc


all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(L_FLAGS) $^ -o $@

%.o: %.c
	$(CC) $(B_FLAGS) $^ -o $@

clean:
	rm $(OBJS)
	rm $(EXE)


