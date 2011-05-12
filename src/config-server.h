/* Config server.
   Eploys *libconhash* to implement consistent hashing with a red-black tree.
   The consistent hashing is instance independent. libconhash returns C ips
   for a specific key, each represents a distinct server. We sends back these
   ips to the client.
   TODO We don't support dynamically machine join and leaving, because data
   migration must be solved first.
   TODO We don't support different instances of consistent hashing yet.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef CONFIG_SERVER_H_
#define CONFIG_SERVER_H_

#include <rpc/rpc.h>
#include <netinet/in.h>

#define CONFIG_FILE "RedisServers.cfg"
#define MAX_LINE_LENGTH 0xFF

#define REP_PER_NODE 0x3FF

// Global conhash instance declaration. Definition in "cfgsrv.c".
extern struct conhash_s *conhash_g;

extern void cfgsrvprog_1(struct svc_req *rqstp, register SVCXPRT *transp);

#endif
