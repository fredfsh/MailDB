/* Config server lib on client side.
   Interface file between client and config server. Client asks config server
   for hosts, which are owners of a key.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef CFGSRV_H_
#define CFGSRV_H_

#include <netinet/in.h>

// @returns host ips for @key.
void getHostsByKey(const char *key, int *num, struct in_addr *ips);

#endif
