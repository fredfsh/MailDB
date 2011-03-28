/* Refer to the comments at the begining of "redis.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "api.h"
#include "def.h"
#include "router.h"
#include <stdio.h>

// Creates a bucket.
//
// Not supported.
// Bucket creation could simply be done by maintaining a special Redis list,
// the value of which is IDs of buckets.
//int createBucket(const char *bucketId);

// Tests whether a bucket exists.
//
// Not supported.
//int ifBucketExist(const char *bucketId);

// Deletes a bucket.
//
// Not supported.
//int deleteBucket(const char *bucketId);

// Deletes a blob.
int deleteBlob(const char *bucketId, const char *blobId) {
  int rv;

  rv = routeDeleteBlob(bucketId, blobId);
  if (rv == ROUTER_ERR) {
    printf("api.c: %s %s\n", "Failed to delete blob.",
        "Lower layer replies with error.");
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
    printf("api.c: %s %s\n", "Failed to delete blob.", "Lower layer fails to.");
    return API_FAILED;
  }
  return API_OK;
}

// Tests whether a blob exists.
int existBlob(const char *bucketId, const char *blobId, int *result) {
  int rv;

  rv = routeExistBlob(bucketId, blobId, result);
  if (rv == ROUTER_ERR) {
    printf("api.c: %s %s\n", "Failed to determine existence of blob.",
        "Lower layer replies with error.");
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
    printf("api.c: %s %s\n", "Failed to determine existence of blob.",
        "Lower layer fails to.");
    return API_FAILED;
  }
  return API_OK;
}

// Loads a blob.
int loadBlob(const char *bucketId, const char *blobId, int *blobLength,
    void *blob) {
  int rv;

  rv = routeLoadBlob(bucketId, blobId, blobLength, blob);
  if (rv == ROUTER_ERR) {
    printf("api.c: %s %s\n", "Failed to load blob.",
        "Lower layer replies with error.");
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
    printf("api.c: %s %s\n", "Failed to load blob.", "Lower layer fails to.");
    return API_FAILED;
  }
  return API_OK;
}

// Saves a blob.
int saveBlob(const char *bucketId, const char *blobId, const int blobLength,
    const void *blob) {
  int rv;

  rv = routeSaveBlob(bucketId, blobId, blobLength, blob);
  if (rv == ROUTER_ERR) {
    printf("api.c: %s %s\n", "Failed to save blob.",
        "Lower layer replies with error.");
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
    printf("api.c: %s %s\n", "Failed to save blob.", "Lower layer fails to.");
    return API_FAILED;
  }
  return API_OK;
}
