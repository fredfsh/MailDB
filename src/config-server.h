/* Config server.
   Interface file between client and config server. Client ask config server
   for hosts, which are owners of a key.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef CONFIG_SERVER_H_
#define CONFIG_SERVER_H_

#include <netinet/in.h>

// @returns host ips for @key.
void getHostsByKey(const char *key, int *num, struct in_addr *ips);

#endif
