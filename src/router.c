/* Refer to the comments at the beginning of "router.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "cfgsrv.h"
#include "def.h"
#include "redis.h"
#include "router.h"
#include "threadpool.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

CLIENT *client = NULL;

// Gets N hosts for the specific @key.
// @returns false when not enough servers available.
int __getHostsByKey(const char *key, struct in_addr *res_ips)
{
  char *rpc_key;
  ips *rpc_result;

  rpc_key = (char *) malloc(MAX_COMMAND_LENGTH * sizeof(char));
  strcpy(rpc_key, key);
  rpc_result = get_hosts_by_key_1(&rpc_key, client);
  if (!rpc_result) {
    printf("router.c: %s %s\n", "Error getting hosts by key.",
        "Failed to get redis server info from config server through rpc.");
    return ROUTER_ERR;
  }

  if (rpc_result->ips_len < N) {
    printf("router.c: %s %s\n", "Failed to get hosts by key.",
        "Not enough redis server available.");
    return ROUTER_FAILED;
  }

  memcpy(res_ips, rpc_result->ips_val, sizeof(struct in_addr) * N);
  return ROUTER_OK;
}

// Creates a bucket at the database considering consistent hashing.
// @returns ROUTER_OK when at least W machines succeed.
//
// I choose N machines according to consistent hashing, and inserts a
// pre-defined blob into those machines. Hence if a bucket with a same
// @bucketId already exists, no bad effect would take place.
int routeCreateBucket(const char *bucketId) {
  int rv;
  struct in_addr ips[N];
  RedisCommand *redisCommand;

  // Finds N redis servers for bucket creation.
  rv = __getHostsByKey(bucketId, ips);
  if (rv != ROUTER_OK) {
    printf("router.c: %s %s\n", "Failed to create bucket.",
        "Unable to get hosts by key.");
    return ROUTER_FAILED;
  }

  // Constructs redis command structure.
  redisCommand = newRedisCommand(hSet, bucketId, EXIST_BLOB_ID,
      strlen(EXIST_BLOB), EXIST_BLOB);

  // Executes redis command.
  execute(N, ips, redisCommand);

  // Reads result.
  rv = readResult(W, redisCommand, NULL, NULL);
  if (rv != THREADPOOL_OK) {
    rv = ROUTER_FAILED;
  } else {
    rv = ROUTER_OK;
  }
  return rv;
}

// Deletes a blob from the database considering consistent hashing.
//
// I choose N machines according to consistent hashing, and deletes the blob
// on all of them.
int routeDeleteBlob(const char *bucketId, const char *blobId) {
  int rv;
  struct in_addr ips[N];
  RedisCommand *redisCommand;

  // Finds N redis servers for blob deletion.
  rv = __getHostsByKey(bucketId, ips);
  if (rv != ROUTER_OK) {
    printf("router.c: %s %s\n", "Failed to delete blob.",
        "Unable to get hosts by key.");
    return ROUTER_FAILED;
  }

  // Constructs redis command structure.
  redisCommand = newRedisCommand(hDel, bucketId, blobId, 0, NULL);

  // Executes redis command.
  execute(N, ips, redisCommand);

  // Reads result.
  rv = readResult(N, redisCommand, NULL, NULL);
  if (rv != THREADPOOL_OK) {
    rv = ROUTER_FAILED;
  } else {
    rv = ROUTER_OK;
  }
  return rv;
}

// Deletes a bucket from the database considering consistent hashing.
//
// I choose N machines according to consistent hashing, and deletes the bucket
// on all of them.
int routeDeleteBucket(const char *bucketId) {
  int rv;
  struct in_addr ips[N];
  RedisCommand *redisCommand;

  // Finds N redis servers for bucket deletion.
  rv = __getHostsByKey(bucketId, ips);
  if (rv != ROUTER_OK) {
    printf("router.c: %s %s\n", "Failed to delete bucket.",
        "Unable to get hosts by key.");
    return ROUTER_FAILED;
  }

  // Constructs redis command structure.
  redisCommand = newRedisCommand(del, bucketId, NULL, 0, NULL);

  // Executes redis command.
  execute(N, ips, redisCommand);

  // Reads result.
  rv = readResult(N, redisCommand, NULL, NULL);
  if (rv != THREADPOOL_OK) {
    rv = ROUTER_FAILED;
  } else {
    rv = ROUTER_OK;
  }
  return rv;
}

// Determines whether a blob exists in the database considering consistent
// hashing.
// @returns ROUTER_OK when existed.
//
// I choose N machines according to consistent hashing. If any of them
// contains the target, the blob is regarded as existed.
int routeExistBlob(const char *bucketId, const char *blobId) {
  int rv;
  int exist;
  struct in_addr ips[N];
  RedisCommand *redisCommand;

  // Finds N redis servers for blob existence determination.
  rv = __getHostsByKey(bucketId, ips);
  if (rv != ROUTER_OK) {
    printf("router.c: %s %s\n", "Failed to determine existence of blob.",
        "Unable to get hosts by key.");
    return ROUTER_FAILED;
  }

  // Constructs redis command structure.
  redisCommand = newRedisCommand(hExists, bucketId, blobId, 0, NULL);

  // Executes redis command.
  execute(N, ips, redisCommand);

  // Reads result.
  rv = readResult(1, redisCommand, &exist, NULL);
  if (rv != THREADPOOL_OK || !exist) {
    rv = ROUTER_FAILED;
  } else {
    rv = ROUTER_OK;
  }
  return rv;
}

// Determines whether a bucket exists in the database considering consistent
// hashing.
// @returns ROUTER_OK when existed.
//
// I choose N machines according to consistent hashing. If any of them
// contains the target, the bucket is regarded as existed.
int routeExistBucket(const char *bucketId) {
  int rv;
  int exist;
  struct in_addr ips[N];
  RedisCommand *redisCommand;

  // Finds N redis servers for bucket existence determination.
  rv = __getHostsByKey(bucketId, ips);
  if (rv != ROUTER_OK) {
    printf("router.c: %s %s\n", "Failed to determine existence of bucket.",
        "Unable to get hosts by key.");
    return ROUTER_FAILED;
  }

  // Constructs redis command structure.
  redisCommand = newRedisCommand(exists, bucketId, NULL, 0, NULL);

  // Executes redis command.
  execute(N, ips, redisCommand);

  // Reads result.
  rv = readResult(1, redisCommand, &exist, NULL);
  if (rv != THREADPOOL_OK || !exist) {
    rv = ROUTER_FAILED;
  } else {
    rv = ROUTER_OK;
  }
  return rv;
}

// Loads a blob from the database considering consistent hashing.
// @returns ROUTER_OK when at least R machines succeed.
// ( TODO: Conflict resolution. At present, if different version of data is
//   received, chosen randomly. )
//
// I choose N machines according to consistent hashing, and load the data from
// those machines.
int routeLoadBlob(const char *bucketId, const char *blobId, int *blobLength,
    void *blob) {
  int rv;
  int rblobLength;
  struct in_addr ips[N];
  RedisCommand *redisCommand;
  unsigned char rblob[MAX_BLOB_LENGTH];

  // Finds N redis servers for blob loading.
  rv = __getHostsByKey(bucketId, ips);
  if (rv != ROUTER_OK) {
    printf("router.c: %s %s\n", "Failed to load blob.",
        "Unable to get hosts by key.");
    return ROUTER_FAILED;
  }

  // Constructs redis command structure.
  redisCommand = newRedisCommand(hGet, bucketId, blobId, 0, NULL);

  // Executes redis command.
  execute(N, ips, redisCommand);

  // Reads result.
  rv = readResult(R, redisCommand, NULL, rblob);
  if (rv != THREADPOOL_OK) {
    rv = ROUTER_FAILED;
  } else {
    memcpy(&rblobLength, rblob, sizeof(int));
    rblobLength -= sizeof(int);
    *blobLength = rblobLength;
    memcpy(blob, &((int *) rblob)[1], rblobLength);
    rv = ROUTER_OK;
  }
  return rv;
}

// Saves a blob into the database considering consistent hashing.
// @returns ROUTER_OK when at least W machines succeed.
//
// I choose N machines according to consistent hashing, and save the data onto
// those machines.
int routeSaveBlob(const char *bucketId, const char *blobId, 
    const int blobLength, const void *blob) {
  int rv;
  struct in_addr ips[N];
  RedisCommand *redisCommand;

  // Finds N redis servers for blob saving.
  rv = __getHostsByKey(bucketId, ips);
  if (rv != ROUTER_OK) {
    printf("router.c: %s %s\n", "Failed to save blob.",
        "Unable to get hosts by key.");
    return ROUTER_FAILED;
  }

  // Constructs redis command structure.
  redisCommand = newRedisCommand(hSet, bucketId, blobId, blobLength, blob);

  // Executes redis command.
  execute(N, ips, redisCommand);

  // Reads result.
  rv = readResult(W, redisCommand, NULL, NULL);
  if (rv != THREADPOOL_OK) {
    printf("[debug]router.c: Cause by %x, %x, %x\n",
        *((unsigned int *) &ips[0]),
        *((unsigned int *) &ips[1]),
        *((unsigned int *) &ips[2]));
    rv = ROUTER_FAILED;
  } else {
    rv = ROUTER_OK;
  }
  return rv;
}

// Finishing function must be called after any operation.
void routerDestroy() {
  threadPoolDestroy();
}

// Initial function must be called before any operation.
int routerInit() {
  int rv;

  client = clnt_create(CFGSRV_HOST, CFGSRVPROG, CFGSRVVERS, "tcp");
  printf("[debug]router.c: RPC client init success.\n");

  rv = threadPoolInit();
  if (rv == THREADPOOL_ERR) {
    printf("router.c: %s %s\n", "Failed to initialize router layer.",
        "Error initializing thread pool.");
    return ROUTER_FAILED;
  } else if (rv == THREADPOOL_FAILED) {
    printf("router.c: %s %s\n", "Failed to initialize router layer.",
        "Failed to initialize thread pool.");
    return ROUTER_FAILED;
  }

  printf("[debug]router.c: Client threadpool init success.\n");
  return ROUTER_OK;
}
