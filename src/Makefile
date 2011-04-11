# MailDB Makefile
#
# By fredfsh (fredfsh@gmail.com)

CC = g++
LD = g++
CFLAGS = -Wall -O2
SRC = api.c\
      client.c\
      config-server.c\
      redis.c\
      router.c
OBJ = $(SRC:%.c=%.o)
TARGET = client
CLIOBJ = $(OBJ)


all: $(TARGET)

client: $(CLIOBJ)
	$(LD) -o client $(CLIOBJ)

$(OBJ): %.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJ) $(TARGET)
