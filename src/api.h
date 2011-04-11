/* Outmost wrapper of the desired APIs.
   This component links the desired APIs with our implementations. It servers
   as the entry point for upper layer who should include this file for
   invocation. This file is an ANSI C version of wrappers.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef API_H_
#define API_H_

#define API_OK 0
#define API_FAILED -1
#define API_ERR 1

// Creates a bucket.
int createBucket(const char *bucketId);
// Deletes a blob.
int deleteBlob(const char *bucketId, const char *blobId);
// Deletes a bucket.
int deleteBucket(const char *bucketId);
// Determines whether a blob exists.
int existBlob(const char *bucketId, const char *blobId);
// Determines whether a bucket exists.
int existBucket(const char *bucketId);
// Loads a blob.
//int loadBlob(const char *bucketId, const char *blobId, int *blobLength,
//    void *blob);
// Saves a blob.
int saveBlob(const char *bucketId, const char *blobId, const int blobLength,
    const void *blob);

#endif
