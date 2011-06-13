# MailDB Makefile
#
# By fredfsh (fredfsh@gmail.com)

CC = cc
CFLAGS = -Wall -O2 -pthread
LD = cc
LDFLAGS = -lpthread
RPCGEN = rpcgen

CLISRC = api.c\
      connectionpool.c\
      client.c\
      redis.c\
      router.c\
      threadpool.c
CLIOBJ = $(CLISRC:%.c=%.o)
CFGSRVSRC = config-server.c\
      conhash.c\
      conhash_inter.c\
      conhash_util.c\
      md5.c\
      util_rbtree.c
CFGSRVOBJ = $(CFGSRVSRC:%.c=%.o)
OBJ = $(CLIOBJ) $(CFGSRVOBJ)

RPCPROG = cfgsrv
RPCSRC = $(RPCPROG).x
RPCHDR = $(RPCPROG).h
RPCGENSRC = $(addprefix $(RPCPROG)_,clnt.c svc.c xdr.c)
RPCTGT = $(RPCHDR) $(RPCGENSRC)
RPCOBJ = $(RPCGENSRC:%.c=%.o)

BIN = client config-server

.PHONY: all clean

all: $(BIN)

client: $(RPCPROG)_clnt.o $(RPCPROG)_xdr.o $(CLIOBJ) 
	$(LD) $(LDFLAGS) -o $@ $+

config-server: $(RPCPROG)_svc.o $(RPCPROG)_xdr.o $(CFGSRVOBJ) 
	$(LD) $(LDFLAGS) -o $@ $+

$(OBJ): %.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(RPCOBJ): %.o: %.c $(RPCHDR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(RPCTGT): $(RPCSRC)
	$(RPCGEN) -h -o $(RPCHDR) $(RPCSRC)
	$(RPCGEN) -l -o $(RPCPROG)_clnt.c $(RPCSRC)
	$(RPCGEN) -m -o $(RPCPROG)_svc.c $(RPCSRC)
	$(RPCGEN) -c -o $(RPCPROG)_xdr.c $(RPCSRC)

clean:
	rm -rf $(OBJ) $(BIN) $(RPCTGT) $(RPCOBJ)
