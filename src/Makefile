# MailDB Makefile
#
# By fredfsh (fredfsh@gmail.com)

CC = g++
LD = g++
CFLAGS = -Wall -O2
SRC = api.c\
       config-server.c\
       debug.c\
       md5.c\
       redis.c\
       router.c
OBJ = $(SRC:%.c=%.o)
TARGET = config-server debug
CONFSRVOBJ = config-server.o md5.o redis.o router.o
DBGOBJ = api.o debug.o md5.o redis.o router.o


all: $(TARGET)

config-server: $(CONFSRVOBJ)
	$(LD) -o config-server $(CONFSRVOBJ)

debug: $(DBGOBJ)
	$(LD) -o debug $(DBGOBJ)

$(OBJ): %.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJ) $(TARGET)
