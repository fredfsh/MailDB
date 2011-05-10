# MailDB Makefile
#
# By fredfsh (fredfsh@gmail.com)

CC = cc
LD = cc
CFLAGS = -Wall -O2 -pthread
LDFLAGS = -lpthread

CLISRC = api.c\
      client.c\
      cfgsrv.c\
      redis.c\
      router.c\
      threadpool.c
CFGSRVSRC = config-server.c\
      conhash.c\
      conhash_inter.c\
      conhash_util.c\
      md5.c\
      util_rbtree.c
CLIOBJ = $(CLISRC:%.c=%.o)
CFGSRVOBJ = $(CFGSRVSRC:%.c=%.o)
OBJ = $(CLIOBJ) $(CFGSRVOBJ)

TARGET = client config-server

.PHONY: all clean

all: $(TARGET)

client: $(CLIOBJ)
	$(LD) $(LDFLAGS) -o client $(CLIOBJ)

config-server: $(CFGSRVOBJ)
	$(LD) $(LDFLAGS) -o config-server $(CFGSRVOBJ)

$(OBJ): %.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJ) $(TARGET)
