#include "api.h"
#include "def.h"
#include "redis.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  int rv;
  int blobLength;
  char blob[MAX_BLOB_LENGTH];
  char targetBucket[] = "amy";
  char targetField[] = "Russia";
  char targetBlob[] = "Moscow";

  rv = deleteBucket(targetBucket);
  if (rv == API_OK) {
    printf("Delete bucket \"%s\" success.\n", targetBucket);
  } else if (rv == API_FAILED) {
    printf("Delete bucket \"%s\" failed.\n", targetBucket);
  } else if (rv == API_ERR) {
    printf("Delete bucket \"%s\" error.\n", targetBucket);
  }

  rv = existBucket(targetBucket);
  if (rv == API_OK) {
    printf("Bucket \"%s\" existed.\n", targetBucket);
  } else {
    printf("Bucket \"%s\" not existed.\n", targetBucket);
  }

  rv = createBucket(targetBucket);
  if (rv == API_OK) {
    printf("Create bucket \"%s\" success.\n", targetBucket);
  } else if (rv == API_FAILED) {
    printf("Create bucket \"%s\" failed.\n", targetBucket);
  } else if (rv == API_ERR) {
    printf("Create bucket \"%s\" error.\n", targetBucket);
  }

  rv = existBucket(targetBucket);
  if (rv == API_OK) {
    printf("Bucket \"%s\" existed.\n", targetBucket);
  } else {
    printf("Bucket \"%s\" not existed.\n", targetBucket);
  }

  rv = deleteBlob(targetBucket, targetField);
  if (rv == API_OK) {
    printf("Delete blob \"%s-%s\" success.\n", targetBucket, targetField);
  } else if (rv == API_FAILED) {
    printf("Delete blob \"%s-%s\" failed.\n", targetBucket, targetField);
  } else if (rv == API_ERR) {
    printf("Delete blob \"%s-%s\" error.\n", targetBucket, targetField);
  }

  rv = existBlob(targetBucket, targetField);
  if (rv == API_OK) {
    printf("Blob \"%s-%s\" existed.\n", targetBucket, targetField);
  } else {
    printf("Blob \"%s-%s\" not existed.\n", targetBucket, targetField);
  }

  rv = saveBlob(targetBucket, targetField, strlen(targetBlob), targetBlob);
  if (rv == API_OK) {
    printf("Save blob \"%s-%s-%s\" success.\n", targetBucket, targetField,
        targetBlob);
  } else if (rv == API_FAILED) {
    printf("Save blob \"%s-%s-%s\" failed.\n", targetBucket, targetField,
        targetBlob);
  } else if (rv == API_ERR) {
    printf("Save blob \"%s-%s-%s\" error.\n", targetBucket, targetField,
        targetBlob);
  }

  rv = existBucket(targetBucket);
  if (rv == API_OK) {
    printf("Bucket \"%s\" existed.\n", targetBucket);
  } else {
    printf("Bucket \"%s\" not existed.\n", targetBucket);
  }

  rv = deleteBlob(targetBucket, targetField);
  if (rv == API_OK) {
    printf("Delete blob \"%s-%s\" success.\n", targetBucket, targetField);
  } else if (rv == API_FAILED) {
    printf("Delete blob \"%s-%s\" failed.\n", targetBucket, targetField);
  } else if (rv == API_ERR) {
    printf("Delete blob \"%s-%s\" error.\n", targetBucket, targetField);
  }

  rv = existBlob(targetBucket, targetField);
  if (rv == API_OK) {
    printf("Blob \"%s-%s\" existed.\n", targetBucket, targetField);
  } else {
    printf("Blob \"%s-%s\" not existed.\n", targetBucket, targetField);
  }


  /*
  rv = loadBlob("fredfsh", "four", &blobLength, blob);
  if (rv == API_OK) {
    if (blobLength == - 1) {
      printf("(nil)\n");
    } else {
      blob[blobLength == MAX_BLOB_LENGTH ? MAX_BLOB_LENGTH - 1 : blobLength]
          = '\0';  // for debug
      printf("%s\n", blob);
    }
  } else if (rv == API_FAILED) {
    printf("Failed.\n");
  } else if (rv == API_ERR) {
    printf("Error.\n");
  }
*/
  return 0;
}
