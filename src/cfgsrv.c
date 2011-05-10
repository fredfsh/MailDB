/* Refer to the comments at the begining of "cfgsrv.h".

   By fredfsh (fredfsh@gmail.com)
*/

#include "cfgsrv.h"
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_FILE "RedisServers.cfg"
#define MAX_LINE_LENGTH 0xFF

void getHostsByKey(const char *key, int *num, struct in_addr *ips) {
  FILE *fin;
  int n;
  int rv;
  char line[MAX_LINE_LENGTH];
  struct addrinfo hints, *result;
  
  n = 0;
  fin = fopen(CONFIG_FILE, "r");
  if (!fin) {
    printf("config-server.c: %s %s\n", "Error getting hosts by key.",
        "Config file for redis servers not found.");
    *num = 0;
    return;
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  rv = fscanf(fin, "%s", line);
  while (!feof(fin)) {
    rv = getaddrinfo(line, NULL, &hints, &result);
    if (!rv && result) {
      memcpy(&ips[n++], &((struct sockaddr_in *) result->ai_addr)->sin_addr,
          sizeof(struct in_addr));
    }
    freeaddrinfo(result);
    rv = fscanf(fin, "%s", line);
  }
  fclose(fin);
  *num = n;
}
