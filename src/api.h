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
//int createBucket(const char *bucketId);
// Tests whether a bucket exists.
//int ifBucketExist(const char *bucketId);
// Deletes a bucket.
//int deleteBucket(const char *bucketId);
// Saves a blob into the database.
int saveBlob(const char *bucketId, const char *blobId, const int streamLength,
    const void *inputStream);
// Reads the content of a blob from the database.
//void *loadBlob(const char *bucket_id, const char *blobId, int *streamLength);
// Tests whether a blob exists.
//int ifBlobExist(const char *bucketId, const char *blobId);
// Deletes a blob.
//int deleteBlob(const char *bucketId, const char *blobId);

#endif
