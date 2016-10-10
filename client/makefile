TRIAL_C = $(wildcard trial*.c)
TRIAL_EXE = $(TRIAL_C:.c=)
C_SOURCES = $(filter-out $(TRIAL_C), $(wildcard *.c))
OBJS = $(C_SOURCES:.c=.o)

L_FLAGS=-g -L/usr/lib/x86_64-linux-gnu -Wall -pthread
B_FLAGS=-g  -L/usr/lib/x86_64-linux-gnu -Wall -c

CC=gcc


all: $(TRIAL_EXE)

trial1: trial1.o $(OBJS)
	$(CC) $(L_FLAGS) $^ -o $@ -lcurl

trial2: trial2.o $(OBJS)
	$(CC) $(L_FLAGS) $^ -o $@ -lcurl

trial3: trial3.o $(OBJS)
	$(CC) $(L_FLAGS) $^ -o $@ -lcurl

trial4: trial4.o $(OBJS)
	$(CC) $(L_FLAGS) $^ -o $@ -lcurl

%.o: %.c
	$(CC) $(B_FLAGS) $^ -o $@

clean:
	rm *.o
	rm $(TRIAL_EXE)
