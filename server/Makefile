CC=$(CROSS_COMPILE)gcc
CUR_PATH=$(dir $(abspath $(MAKEFILE_LIST)))
CFLAGS= -g 
#-fsanitize=address,leak
LDLIBS= -lm -lpthread -lrt
#-lfile_manipulate -lsig_func
SRC=$(wildcard $(CUR_PATH)/*.c)
OBJS=$(SRC:.c=.o)

.PHONY: all aesdsocket clean

.DEFAULT_GOAL: all

all: aesdsocket

aesdsocket: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) aesdsocket $(wildcard *.o) 
