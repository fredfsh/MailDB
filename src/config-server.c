/* Refer to the comments at the begining of "config-server.h".

   By fredfsh (fredfsh@gmail.com)
*/

#include "cfgsrv.h"
#include "config-server.h"
#include "conhash.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>

struct conhash_s *conhash_g;  // global consistent hashing instance

static ips result;

ips * get_hosts_by_key_1(char **key, CLIENT *client)
{
  
  result.ips_len = 0;
  result.ips_val = (u_int *) calloc(C, sizeof(u_int));

  conhash_lookup(conhash_g, *key, (int *) &result.ips_len,
      (struct in_addr *) &result.ips_val);

  return &result;
}

ips * get_hosts_by_key_1_svc(char **key, struct svc_req *svc)
{
  CLIENT *client;
  return get_hosts_by_key_1(key, client);
}

// Initializes consistent hashing ring and starts working.
// Server nodes information are read-in from a pre-defined config file.
void __conhash_init()
{
  FILE *fin;
  int rv;
  char line[MAX_LINE_LENGTH];
  struct addrinfo hints, *result;

  // Initializes global consistent hashing instance.
  conhash_g = conhash_init(NULL);  // default MD5 hash

  // Loads machine information from config file.
  // Adds nodes to the conhash instance.
  fin = fopen(CONFIG_FILE, "r");
  if (!fin) {
    printf("config-server.c: %s %s\n", "Error getting hosts by key.",
        "Config file for redis servers not found.");
    return;
  }
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  rv = fscanf(fin, "%s", line);
  while (!feof(fin)) {
    rv = getaddrinfo(line, NULL, &hints, &result);
    if (!rv && result) {
      conhash_add_node(conhash_g,
          ((struct sockaddr_in *) result->ai_addr)->sin_addr, REP_PER_NODE);
    }
    freeaddrinfo(result);
    rv = fscanf(fin, "%s", line);
  }
  fclose(fin);
}

// Initializes rpc server-side stub and listens for incoming call.
// Never @returns.
void __rpc_init()
{
  register SVCXPRT *transp;

  pmap_unset (CFGSRVPROG, CFGSRVVERS);

  transp = svcudp_create(RPC_ANYSOCK);
  if (transp == NULL) {
    fprintf (stderr, "%s", "cannot create udp service.");
    exit(1);
  }
  if (!svc_register(transp, CFGSRVPROG, CFGSRVVERS, cfgsrvprog_1,
        IPPROTO_UDP)) {
    fprintf (stderr, "%s", "unable to register (CFGSRVPROG, CFGSRVVERS, udp).");
    exit(1);
  }

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf (stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, CFGSRVPROG, CFGSRVVERS, cfgsrvprog_1,
        IPPROTO_TCP)) {
    fprintf (stderr, "%s", "unable to register (CFGSRVPROG, CFGSRVVERS, tcp).");
    exit(1);
  }

  printf("config-server.c: RPC routine registered. Start listening for "
      "incoming calls...\n");

  svc_run ();
  fprintf (stderr, "%s", "svc_run returned");
  exit (1);
  /* NOTREACHED */
}

int
main (int argc, char **argv)
{
  __conhash_init();
  __rpc_init();  // never returns

  exit(0);
  /* NOTREACHED */
}
