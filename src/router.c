/* Refer to the comments at the beginning of "router.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "router.h"

typedef struct ConsistentHashingMap {
} ConsistentHashingMap;

// Saves a blob into the database considering consistent hashing.
//
// I choose a machine according to consistent hashing, and save the data onto
// that machine.
int redisHashSet(const char *bucketId, const char *blobId,
    const int streamLength, const void *inputStream) {
  int fd = _chooseMachine(bucketId);
}

// Chooses a machine according to consistent hashing, and returns the
// connection socket.
//
// At present, I just randomly pick a machine and try to make a connection. If
// the connection cannot be established, I ask the config server for an
// updated map. Then I continue to randomly pick a machine, until any machine
// is available.
int _chooseMachine(const char *key) {
}
