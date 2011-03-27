/* Router layer implementing consistent hashing for data distribution.
   This component asks the config server for information about machine liveness
   and death. It maintains a map from hashes of keys to host names of server
   machines, according to the consistent hashing algorithm. This map is cached
   locally. If connection with the target host could not be established, this
   component asks the config server for the latest snapshot of server states,
   and modifies the map relationship. Then it informs related server to move
   data around for load balance.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef ROUTER_H_
#define ROUTER_H_

int redisHashSet(const char *bucketId, const char *blobId,
    const int streamLength, const void *inputStream);

#endif
