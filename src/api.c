/* Refer to the comments at the begining of "api.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "api.h"
#include "def.h"
#include "router.h"
#include <stdio.h>

// Finishing function must be called after any operation.
void apiDestroy() { routerDestroy(); }

// Initial function must be called before any operation.
int apiInit() {
  int rv;

  rv = routerInit();
  if (rv == ROUTER_ERR) {
    printf("api.c: %s %s\n", "Failed to initialize api layer.",
        "Lower layer fails to.");
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
    printf("api.c: %s %s\n", "Failed to initialize api layer.",
        "Lower layer fails to.");
    return API_FAILED;
  }
  return API_OK;
}

// Creates a bucket.
int createBucket(const char *bucketId) {
  int rv;

  rv = routeCreateBucket(bucketId);
  if (rv == ROUTER_ERR) {
    printf("api.c: %s %s\n", "Failed to create bucket.",
        "Lower layer replies with error.");
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
    printf("api.c: %s %s\n", "Failed to create bucket.",
        "Lower layer fails to.");
    return API_FAILED;
  }
  return API_OK;
}

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

// Deletes a bucket.
int deleteBucket(const char *bucketId) {
  int rv;

  rv = routeDeleteBucket(bucketId);
  if (rv == ROUTER_ERR) {
    printf("api.c: %s %s\n", "Failed to delete bucket.",
        "Lower layer replies with error.");
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
    printf("api.c: %s %s\n", "Failed to delete bucket.",
        "Lower layer fails to.");
    return API_FAILED;
  }
  return API_OK;
}

// Determines whether a blob exists.
// @returns API_OK when existed.
int existBlob(const char *bucketId, const char *blobId) {
  int rv;

  rv = routeExistBlob(bucketId, blobId);
  if (rv == ROUTER_ERR) {
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
    return API_FAILED;
  }
  return API_OK;
}

// Determines whether a bucket exists.
// @returns API_OK when existed.
int existBucket(const char *bucketId) {
  int rv;

  rv = routeExistBucket(bucketId);
  if (rv == ROUTER_ERR) {
    return API_FAILED;
  } else if (rv == ROUTER_FAILED) {
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
