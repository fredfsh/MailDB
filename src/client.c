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
  for (i = 0; i < length; ++i) {
    key[i] = (char) (rand() % 26 + 'A');
    field[i] = (char) (rand() % 26 + 'a');
  }
  key[length] = '\0';
  field[length] = '\0';
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

void test(const char *testType, const char *testTimeStr) {
  int i;
  int type, threshold;
  int rv;
  int totalNum, successNum, failedNum, slowNum;
  int cnt;
  int opTime;
  long successTime;
  struct timeval start, end, last, current;
  int testTimeInSeconds;
  char *keys[TEST_SUITE_NUM];
  char *fields[TEST_SUITE_NUM];
  int valueLength, valueLengths[TEST_SUITE_NUM];
  unsigned char value[MAX_BLOB_LENGTH], *values[TEST_SUITE_NUM];
  FILE *fout;

  totalNum = 0;
  successNum = 0;
  failedNum = 0;
  slowNum = 0;
  successTime = 0;
  cnt = 0;
  testTimeInSeconds = atoi(testTimeStr);
  if (!strcmp(testType, "read")) {
    threshold = 100;
  } else if (!strcmp(testType, "write")) {
    threshold = 200;
  } else {
    threshold = 150;
  }

  printf("Preparing for data...\n");
  for (i = 0; i < TEST_SUITE_NUM; ++i) {
    keys[i] = (char *) malloc(MAX_KEY_LENGTH * sizeof(char));
    fields[i] = (char *) malloc(MAX_KEY_LENGTH * sizeof(char));
    values[i] =
        (unsigned char *) malloc(MAX_BLOB_LENGTH * sizeof(unsigned char));
    makeBlob(keys[i], fields[i], &valueLengths[i], values[i]);
  }

  printf("Starting to test...\n");
  gettimeofday(&start, NULL);
  gettimeofday(&end, NULL);
  while (ms(start, end) < 1000 * testTimeInSeconds) {
    i = cnt % TEST_SUITE_NUM;
    type = rand() % 11;
    gettimeofday(&last, NULL);
    if (!strcmp(testType, "read")) {
      rv = loadBlob(keys[i], fields[i], &valueLength, value);
    } else if (!strcmp(testType, "write")) {
      rv = saveBlob(keys[i], fields[i], valueLengths[i], values[i]);
    } else {
      if (type < 7) {
        rv = loadBlob(keys[i], fields[i], &valueLength, value);
      } else if (type < 10) {
        rv = saveBlob(keys[i], fields[i], valueLengths[i], values[i]);
      } else {
        rv = deleteBlob(keys[i], fields[i]);
      }
    }
    gettimeofday(&current, NULL);
    opTime = ms(last, current);
    //if (++cnt % 100 == 0) printf("opTime = %d\n", opTime);
    if (opTime > threshold) printf("opTime = %d\n", opTime);

    if (ms(start, end) >= (testTimeInSeconds - 60) * 1000) {
      ++totalNum;
      if (rv == API_OK) {
        ++successNum;
        successTime += opTime;
        if (opTime > threshold) ++slowNum;
      } else {
        ++failedNum;
      }
    }
    gettimeofday(&end, NULL);
  }
  fout = fopen(testTimeStr, "w");
  fprintf(fout, "%d %d %d %d %ld\n",
      totalNum, successNum, failedNum, slowNum, successTime);
  fclose(fout);
  printf("Done. Result written to %s.\n", testTimeStr);
}

void usage() {
  printf("Usage: ./client <type> <test_time_in_seconds>\n");
  printf("<type> is one of\"read\", \"write\", \"mixed\"\n");
}

int main(int argc, char *argv[]) {
  int rv;

  rv = apiInit();
  if (rv == API_OK) {
    printf("API init success.\n");
  } else if (rv == API_FAILED) {
    printf("API init failed.\n");
    exit(1);
  } else if (rv == API_ERR) {
    printf("API init error.\n");
    exit(1);
  }

  if (argc != 3) {
    usage();
  } else {
    test(argv[1], argv[2]);
  }

  apiDestroy();
  printf("API destroy.\n");

  return 0;
}
