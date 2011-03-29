# MailDB Makefile
#
# By fredfsh (fredfsh@gmail.com)

CC = g++
CFLAGS = -Wall -O2
SRCS = api.c\
       md5.c\
       redis.c\
       router.c
OBJS = $(SRCS:%.c=%.o)

#all: config-server debug

#config-server: config-server.c
#$(CC) $(CFLAGS) -o config-server config-server.c

debug: debug.c $(OBJS)
	$(CC) $(CFLAGS) -o debug debug.c $(OBJS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf *.o
