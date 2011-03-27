/* Refer to the comments at the begining of "redis.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "api.h"
#include "router.h"

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

// Saves a blob into the database.
//
// I simply use a Redis hash to implement this. The bucket ID is the key, the
// blob ID is the field, while the blob content is the value. I also invoke
// APIs from the router layer, which is responsible for data distribution.
int saveBlob(const char *bucketId, const char *blobId, const int streamLength,
    const void *inputStream) {
  int rv = redisHashSet(bucketId, blobId, streamLength, inputStream);
  if (rv == ROUTER_OK) return API_OK;
  if (rv == ROUTER_FAILED) return API_FAILED;
  return API_ERR;
}

// Reads the content of a blob from the database.
//void *loadBlob(const char *bucket_id, const char *blobId, int *streamLength);

// Tests whether a blob exists.
//int ifBlobExist(const char *bucketId, const char *blobId);

// Deletes a blob.
//int deleteBlob(const char *bucketId, const char *blobId);
