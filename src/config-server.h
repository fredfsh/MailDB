/* Config server.
   Config server maintains all the information about the liveness and death of
   each Redis server in the whole system. The info is updated according to the
   basic heartbeat mechanism, and it responses to the client, who connects to
   this config server, the latest snapshot. The info is sent with a sequence of
   serialized "in_addr" sequence.
   Config server also distinguish failures between Redis process failure and
   machine unreachable failure, by Redis PING command and socket connection.
   The difference is not told to client but logged locally, for administrator
   to take action properly.
   After administrator adds machines to the cluster and setups of Redis, he
   modifies "RedisServers.cfg" at config server. Then he signals config server
   process with SIGUSR1 and the server will reload the config file update its
   liveness list.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef CONFIG_SERVER_H_
#define CONFIG_SERVER_H_

#define CONFSRV_OK 0
#define CONFSRV_FAILED -1
#define CONFSRV_ERR 1

#define REDISSRV_CONFIG_FILE "RedisServers.cfg"

#define MAX_LINE_LENGTH 0xFF
#define MAX_LOG_LENGTH 0xFF
#define MAX_SNAPSHOT_LENGTH 0xFFFF

#define HEARTBEAT_RETRY 3 
#define HEARTBEAT_INTERVAL 30  // in seconds
//#define HEARTBEAT_INTERVAL 3  // debug
#define PING_RETRY 5
#define PING_INTERVAL 500 * 1000  // in microseconds

#define CONFIG_SERVER_PORT 6380

#endif
