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

#define ROUTER_OK 0
#define ROUTER_FAILED -1
#define ROUTER_ERR 1

#define CONFIG_FILE "RedisServers.cfg"

#define EXIST_BLOB_ID "#ExistBlobId#"
#define EXIST_BLOB "#ExistBlob#"

#define MAX_HASH_LENGTH 0xFF
#define MAX_LINE_LENGTH 0xFF

#include <netinet/in.h>

typedef struct ConsistentHashingVirtualNode {
  unsigned char hash[MAX_HASH_LENGTH];
  struct ConsistentHashingVirtualNode *next;
} chVirtualNode;

typedef struct ConsistentHashingNode {
  in_addr addr;
  chVirtualNode *list;
  int nVirtualNode;
  struct ConsistentHashingNode *next;
} chNode;

typedef struct ConsistentHashingMap {
  chNode *list;
  int nNode;
} chMap;

// Creates a ConsistentHashingNode and add it to a ConsistentHashingMap.
// @returns that node.
chNode *addNode(const sockaddr *addr, chMap *map);
// Creates a ConsistentHashingVirtualNode and add it to a ConsistentHashingNode.
// @returns that virtual node.
chVirtualNode *addVirtualNode(const char *key, chMap *map);

// Creates a bucket considering consistent hashing.
int routeCreateBucket(const char *bucketId);
// Deletes a blob considering consistent hashing.
int routeDeleteBlob(const char *bucketId, const char *blobId);
// Deletes a bucket considering consistent hashing.
int routeDeleteBucket(const char *bucketId);
// Determines whether a blob exists considering consistent hashing.
int routeExistBlob(const char *bucketId, const char *blobId, int *result);
// Determines whether a bucket exists considering consistent hashing.
int routeExistBucket(const char *bucketId, int *result);
// Loads a blob considering consistent hashing.
int routeLoadBlob(const char *bucketId, const char *blobId, int *blobLength,
    void *blob);
// Saves a blob considering consistent hashing.
int routeSaveBlob(const char *bucketId, const char *blobId,
    const int blobLength, const void *blob);

#endif
