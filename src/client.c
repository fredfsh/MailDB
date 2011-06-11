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

void makeKeys(char *key, char *field) {
  int i;
  int length;

  length = rand() % (MAX_KEY_LENGTH - 1) + 1;
  for (i = 0; i < length; ++i) key[i] = (char) (rand() % 26 + 'A');
  key[length] = '\0';
  if (field) {
    for (i = 0; i < length; ++i) field[i] = (char) (rand() % 26 + 'a');
    field[length] = '\0';
  }
}

void makeBlob(char *key, char *field, int *valueLength, void *value) {
  int i;
  int length;
  unsigned char *p;

  makeKeys(key, field);

  length = rand() % (MAX_BLOB_LENGTH - 1) + 1;
  //length = rand() % (10 - 1) + 1;
  p = (unsigned char *) value;
  for (i = 0; i < length; ++i) *(p++) = rand();
  *valueLength = length;
}

void testRead(const char *arg) {
  int rv;
  int totalNum, successNum, failedNum, errorNum;
  int averageTime;
  int valueLength;
  int testTimeInSeconds;
  int opTime;
  int cnt;
  long successTime;
  long genTime;
  struct timeval start, end, last, current;
  struct timeval genLast, genCurrent;
  char key[MAX_KEY_LENGTH];
  char field[MAX_KEY_LENGTH];
  unsigned char value[MAX_BLOB_LENGTH];
  FILE *fout;

  totalNum = 0;
  successNum = 0;
  failedNum = 0;
  errorNum = 0;
  successTime = 0;
  genTime = 0;
  cnt = 0;

  gettimeofday(&start, NULL);
  gettimeofday(&end, NULL);
  testTimeInSeconds = atoi(arg);
  while (ms(start, end) < 1000 * testTimeInSeconds) {
    // make keys
    gettimeofday(&genLast, NULL);
    makeKeys(key, field);
    gettimeofday(&genCurrent, NULL);
    genTime += ms(genLast, genCurrent);

    // load blob
    gettimeofday(&last, NULL);
    rv = loadBlob(key, field, &valueLength, value);
    gettimeofday(&current, NULL);
    opTime = ms(last, current);
    if (++cnt % 1000 == 0) printf("opTime = %d\n", opTime);

    if (ms(start, end) >= (testTimeInSeconds - 30) * 1000) {
      ++totalNum;
      if (rv == API_OK) {
        ++successNum;
        successTime += opTime;
      } else if (rv == API_FAILED) {
        ++failedNum;
      } else if (rv == API_ERR) {
        ++errorNum;
      }
    }
    gettimeofday(&end, NULL);
  }
  fout = fopen(arg, "w");
  fprintf(fout, "%d %d %d %d\n", totalNum, successNum, failedNum, errorNum);
  averageTime = successTime / successNum;
  fprintf(fout, "%ld %ld %d\n", genTime, successTime, averageTime);
  fclose(fout);
  printf("Done. Result written to %s.\n", arg);
}

void testWrite(const char *arg) {
  int rv;
  int totalNum, successNum, failedNum, errorNum;
  int averageTime;
  int valueLength;
  int testTimeInSeconds;
  int opTime;
  int cnt;
  long successTime;
  long genTime;
  struct timeval start, end, last, current;
  struct timeval genLast, genCurrent;
  char key[MAX_KEY_LENGTH];
  char field[MAX_KEY_LENGTH];
  unsigned char value[MAX_BLOB_LENGTH];
  FILE *fout;

  totalNum = 0;
  successNum = 0;
  failedNum = 0;
  errorNum = 0;
  successTime = 0;
  genTime = 0;
  cnt = 0;

  gettimeofday(&start, NULL);
  gettimeofday(&end, NULL);
  testTimeInSeconds = atoi(arg);
  while (ms(start, end) < 1000 * testTimeInSeconds) {
    // make blob
    gettimeofday(&genLast, NULL);
    makeBlob(key, field, &valueLength, value);
    gettimeofday(&genCurrent, NULL);
    genTime += ms(genLast, genCurrent);

    // save blob
    gettimeofday(&last, NULL);
    rv = saveBlob(key, field, valueLength, value);
    gettimeofday(&current, NULL);
    opTime = ms(last, current);
    if (++cnt % 100 == 0) printf("opTime = %d\n", opTime);

    if (ms(start, end) >= (testTimeInSeconds - 30) * 1000) {
      ++totalNum;
      if (rv == API_OK) {
        ++successNum;
        successTime += opTime;
      } else if (rv == API_FAILED) {
        ++failedNum;
      } else if (rv == API_ERR) {
        ++errorNum;
      }
    }
    gettimeofday(&end, NULL);
  }
  fout = fopen(arg, "w");
  fprintf(fout, "%d %d %d %d\n", totalNum, successNum, failedNum, errorNum);
  averageTime = successTime / successNum;
  fprintf(fout, "%ld %ld %d\n", genTime, successTime, averageTime);
  fclose(fout);
  printf("Done. Result written to %s.\n", arg);
}

void testWriteThroughput(const char *arg) {
  int i;
  int rv;
  int totalNum, successNum, failedNum, errorNum;
  int testTimeInSeconds;
  int cnt;
  int valueLengths[TEST_SUITE_NUM];
  char *keys[TEST_SUITE_NUM];
  unsigned char *values[TEST_SUITE_NUM];
  struct timeval start, end;
  FILE *fout;

  totalNum = 0;
  successNum = 0;
  failedNum = 0;
  errorNum = 0;
  cnt = 0;

  printf("Preparing for data...\n");
  for (i = 0; i < TEST_SUITE_NUM; ++i) {
    keys[i] = (char *) malloc(MAX_KEY_LENGTH * sizeof(char));
    values[i] =
        (unsigned char *) malloc(MAX_BLOB_LENGTH * sizeof(unsigned char));
    makeBlob(keys[i], NULL, &valueLengths[i], values[i]);
  }

  printf("Starting to write...\n");
  gettimeofday(&start, NULL);
  gettimeofday(&end, NULL);
  testTimeInSeconds = atoi(arg);
  while (ms(start, end) <= 1000 * testTimeInSeconds) {
    // save blob
    rv = saveBlob(keys[cnt], keys[cnt], valueLengths[cnt], values[cnt]);
    if (++cnt % 100 == 0) printf("totalNum = %d\n", totalNum);

    gettimeofday(&end, NULL);
    if (ms(start, end) >= (testTimeInSeconds - 30) * 1000) {
      ++totalNum;
      if (rv == API_OK) {
        ++successNum;
      } else if (rv == API_FAILED) {
        ++failedNum;
      } else if (rv == API_ERR) {
        ++errorNum;
      }
    }
  }
  fout = fopen(arg, "w");
  fprintf(fout, "%d %d %d %d\n", totalNum, successNum, failedNum, errorNum);
  fclose(fout);
  printf("Done. Result written to %s.\n", arg);
}

int main(int argc, char *argv[]) {
  int rv;
  /*
  int blobLength;
  char blob[MAX_BLOB_LENGTH];
  char targetBucket[] = "amy";
  char targetField[] = "Russia";
  char targetBlob[] = "Moscow";
  */

  rv = apiInit();
  if (rv == API_OK) {
    printf("API init success.\n");
  } else if (rv == API_FAILED) {
    printf("API init failed.\n");
  } else if (rv == API_ERR) {
    printf("API init error.\n");
  }

  if (argc != 2) {
    printf("Usage: ./client process_num\n");
  } else {
    testWrite(argv[1]);
  }

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
  */

  apiDestroy();
  printf("API destroy.\n");

  return 0;
}
