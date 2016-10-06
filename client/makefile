TRIAL_C = $(wildcard trial*.c)
TRIAL_EXE = $(TRIAL_C:.c=)
C_SOURCES = $(filter-out $(TRIAL_C), $(wildcard *.c))
OBJS = $(C_SOURCES:.c=.o)



L_FLAGS=-g -Wall -lpthread -lcurl
B_FLAGS=-g -Wall -c

CC=gcc


all: $(TRIAL_EXE)

trial1: trial1.o $(OBJS)
	$(CC) $(L_FLAGS) $^ -o $@

trial2: trial2.o $(OBJS)
	$(CC) $(L_FLAGS) $^ -o $@

trial3: trial3.o $(OBJS)
	$(CC) $(L_FLAGS) $^ -o $@

%.o: %.c
	$(CC) $(B_FLAGS) $^ -o $@

clean:
	rm $(OBJS)
	rm $(TRIAL_EXE)
