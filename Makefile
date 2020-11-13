CC = gcc
SOURCES = dataManager.c idGenerator.c message.c neighbour.c neighbourManager.c tlv.c info.c inputReader.c
CFLAGS = -Wall -g
LIBS = -lm -lpthread
OBJS = $(SOURCES:%.c=%.o)

all: p2pchat

p2pchat : p2pchat.c $(OBJS)
		$(CC) $(CFLAGS) -o $@ p2pchat.c $(OBJS) $(LIBS)

%.o : %.c
		gcc -c $(CFLAGS) $<

clean :
		rm -f  p2pchat *.o
