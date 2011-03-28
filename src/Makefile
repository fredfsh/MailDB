# MailDB Makefile
#
# By fredfsh (fredfsh@gmail.com)

CC = g++
CFLAGS = -Wall -O2
SRCS = api.c\
       debug.c\
       md5.c\
       redis.c\
       router.c
OBJS = $(SRCS:%.c=%.o)

debug: $(OBJS)
	$(CC) $(CFLAGS) -o debug $(OBJS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf *.o
