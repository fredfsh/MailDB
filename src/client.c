#include "api.h"
#include "client.h"
#include "def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
void makeBlob(char *key, char *field, int *valueLength, void *value) {
  int i;
  int length;
  unsigned char *p;

  length = rand() % (MAX_KEY_LENGTH - 1) + 1;
  //length = rand() % (10 - 1) + 1;
  for (i = 0; i < length; ++i) {
    key[i] = (char) (rand() % 26 + 'A');
    field[i] = (char) (rand() % 26 + 'a');
  }
  key[length] = '\0';
  field[length] = '\0';
  length = rand() % (MAX_BLOB_LENGTH - 1) + 1;
  //length = rand() % (10 - 1) + 1;
  p = (unsigned char *) value;
  for (i = 0; i < length; ++i) *(p++) = rand();
  *valueLength = length;
}

void testWrite(int minute) {
  int rv;
  int totalNum, successNum, failedNum, errorNum;
  int TPS;
  double successTime, genTime, averageTime;
  double percentage;
  int valueLength;
  time_t start, end;
  struct timeval last, current, genLast, genCurrent;
  char key[MAX_KEY_LENGTH];
  char field[MAX_KEY_LENGTH];
  unsigned char value[MAX_BLOB_LENGTH];

  totalNum = 0;
  successNum = 0;
  failedNum = 0;
  errorNum = 0;
  successTime = 0.0;
  genTime = 0.0;

  start = time(NULL);
  end = time(NULL);
  while (end - start < 60 * minute) {
    gettimeofday(&genLast, NULL);
    makeBlob(key, field, &valueLength, value);
    gettimeofday(&genCurrent, NULL);
    genTime +=
        (double) ((genCurrent.tv_sec - genLast.tv_sec) * 1000000 +
        genCurrent.tv_usec - genLast.tv_usec) / 1000.0;
    ++totalNum;
    gettimeofday(&last, NULL);
    rv = saveBlob(key, field, valueLength, value);
    gettimeofday(&current, NULL);
    if (rv == API_OK) {
      ++successNum;
      successTime +=
        (double) ((current.tv_sec - last.tv_sec) * 1000000 +
        current.tv_usec - last.tv_usec) / 1000.0;
    } else if (rv == API_FAILED) {
      ++failedNum;
    } else if (rv == API_ERR) {
      ++errorNum;
    }
    end = time(NULL);
  }
  printf("testTime = %dmin\n", minute);
  printf("totalNum = %d, successNum = %d, failedNum = %d, errorNum = %d\n",
      totalNum, successNum, failedNum, errorNum);
  percentage = 100.0 * (double) successNum / (double) totalNum;
  printf("successNum / totalNum = %.2f%%\n", percentage);
  averageTime = successTime / (double) successNum;
  printf("average write time = %.2fms\n", averageTime);
  TPS = totalNum / (end - start);
  printf("transactions per seconds = %d\n", TPS);
  printf("time used to generate test data = %.2fms\n", genTime);
}
*/

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

  //testWrite(atoi(argv[1]));

  /*
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
  */

  rv = createBucket(targetBucket);
  if (rv == API_OK) {
    printf("Create bucket \"%s\" success.\n", targetBucket);
  } else if (rv == API_FAILED) {
    printf("Create bucket \"%s\" failed.\n", targetBucket);
  } else if (rv == API_ERR) {
    printf("Create bucket \"%s\" error.\n", targetBucket);
  }

  /*
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
  */

  apiDestroy();
  printf("API destroy.\n");

  return 0;
}
