/* Config server.
   Eploys *libconhash* to implement consistent hashing with a red-black tree.
   The consistent hashing is instance independent. libconhash returns C ips
   for a specific key, each represents a distinct server. We sends back these
   ips to the client.
   TODO We don't support dynamically machine join and leaving, because data
   migration must be solved first.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef CONFIG_SERVER_H_
#define CONFIG_SERVER_H_

#include <netinet/in.h>

// @returns host ips for @key.
void getHostsByKey(const char *key, int *num, struct in_addr *ips);

#endif
