/* Router layer implementing data replication.

   This component asks the config server for Redis servers associated with each
   key. Then it forks to N children, where each child executes the same command
   to a redis server.

   If W children succeed for a write operation, the write operation is regarded
   as successful.

   If R children succeed for a read operation, the read operation is regarded
   as successful. If different versions of data is returned. Router should
   resolve the conflictions according to the version metadata of the blob.
   (TODO: Modification to Redis code to attach version metadata of blob.
    Current: Random choice.)

   For deletion, we require strong consistency. Only if N children succeed
   for a delete operation, the delete operation is regarded as successful.

   If data exists on any child, the exist operation is regarded as successful.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef ROUTER_H_
#define ROUTER_H_

#define ROUTER_OK 0
#define ROUTER_FAILED -1
#define ROUTER_ERR 1

#define MAX_HASH_LENGTH 0xFF
#define MAX_LINE_LENGTH 0xFF
#define MAX_PORT_LENGTH 0xFF
#define MAX_SNAPSHOT_LENGTH 0xFFFF

#define WAIT_RETRY 100
#define WAIT_RETRY_INTERVAL 10 * 1000  // in microseconds

// Finishing function must be called after any operation.
void routerDestroy();
// Initial function must be called before any operation.
int routerInit();

// Creates a bucket considering consistent hashing.
int routeCreateBucket(const char *bucketId);
// Deletes a blob considering consistent hashing.
int routeDeleteBlob(const char *bucketId, const char *blobId);
// Deletes a bucket considering consistent hashing.
int routeDeleteBucket(const char *bucketId);
// Determines whether a blob exists considering consistent hashing.
int routeExistBlob(const char *bucketId, const char *blobId);
// Determines whether a bucket exists considering consistent hashing.
int routeExistBucket(const char *bucketId);
// Loads a blob considering consistent hashing.
int routeLoadBlob(const char *bucketId, const char *blobId, int *blobLength,
    void *blob);
// Saves a blob considering consistent hashing.
int routeSaveBlob(const char *bucketId, const char *blobId,
    const int blobLength, const void *blob);

#endif
