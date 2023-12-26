CC ?= $(CROSS_COMPILE)gcc

HFILES = 
CFILES = aesdsocket.c

SRCS = ${HFILES} ${CFILES}
OBJS = ${CFILES:.c=.o}

CFLAGS ?= -Wall -Werror
LDFLAGS ?= -lpthread 

all: aesdsocket

clean:
	-rm -f *.o *.d
	-rm -f aesdsocket

aesdsocket: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o aesdsocket $(LDFLAGS)
