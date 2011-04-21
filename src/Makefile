# MailDB Makefile
#
# By fredfsh (fredfsh@gmail.com)

CC = cc
LD = cc
CFLAGS = -Wall -O2 -pthread
LDFLAGS = -lpthread
SRC = api.c\
      client.c\
      config-server.c\
      redis.c\
      router.c\
      threadpool.c
OBJ = $(SRC:%.c=%.o)
TARGET = client
CLIOBJ = $(OBJ)


all: $(TARGET)

client: $(CLIOBJ)
	$(LD) $(LDFLAGS) -o client $(CLIOBJ)

$(OBJ): %.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJ) $(TARGET)
