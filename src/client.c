#include "api.h"
#include "def.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  int rv;
  int blobLength;
  char blob[MAX_BLOB_LENGTH];
  char targetBucket[] = "amy";
  char targetField[] = "Russia";
  char targetBlob[] = "Moscow";

  rv = apiInit();
  if (rv == API_OK) {
    printf("API init success.\n");
  } else if (rv == API_FAILED) {
    printf("API init failed.\n");
  } else if (rv == API_ERR) {
    printf("API init error.\n");
  }

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

  rv = loadBlob(targetBucket, targetField, &blobLength, blob);
  if (rv == API_OK) {
    if (blobLength == 0) {
      printf("Load blob \"%s-%s-(nil)\"\n", targetBucket, targetField);
    } else {
      blob[blobLength == MAX_BLOB_LENGTH ? MAX_BLOB_LENGTH - 1 : blobLength]
          = '\0';  // for debug
      printf("Load blob \"%s-%s-%s\"\n", targetBucket, targetField, blob);
    }
  } else if (rv == API_FAILED) {
    printf("Load blob \"%s-%s\" failed.\n", targetBucket, targetField);
  } else if (rv == API_ERR) {
    printf("Load blob \"%s-%s\" error.\n", targetBucket, targetField);
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

  rv = existBlob(targetBucket, targetField);
  if (rv == API_OK) {
    printf("Blob \"%s-%s\" existed.\n", targetBucket, targetField);
  } else {
    printf("Blob \"%s-%s\" not existed.\n", targetBucket, targetField);
  }

  rv = loadBlob(targetBucket, targetField, &blobLength, blob);
  if (rv == API_OK) {
    if (blobLength == 0) {
      printf("Load blob \"%s-%s-(nil)\"\n", targetBucket, targetField);
    } else {
      blob[blobLength == MAX_BLOB_LENGTH ? MAX_BLOB_LENGTH - 1 : blobLength]
          = '\0';  // for debug
      printf("Load blob \"%s-%s-%s\"\n", targetBucket, targetField, blob);
    }
  } else if (rv == API_FAILED) {
    printf("Load blob \"%s-%s\" failed.\n", targetBucket, targetField);
  } else if (rv == API_ERR) {
    printf("Load blob \"%s-%s\" error.\n", targetBucket, targetField);
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

  apiDestroy();
  printf("API destroy.\n");

  return 0;
}
