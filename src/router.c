/* Refer to the comments at the beginning of "router.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "config-server.h"
#include "def.h"
#include "redis.h"
#include "router.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Cleans zombie children. (Ah... Horrible!)
void _cleanZombies() {
  while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

// Waits for enough children processes to exit.
//
// @returns ROUTER_OK if at least @threshold among @total children processes
// responses successfully.
int _waitChildren(const int total, const int threshold, const pid_t *pids) {
  int exitNum, successNum;
  int retry;
  int status;
  int i;
  int result;
  pid_t exitPid;

  exitNum = 0;
  successNum = 0;
  retry = WAIT_RETRY;
  while (exitNum < total && successNum < threshold && --retry >= 0) {
    usleep(WAIT_RETRY_INTERVAL);
    while ( (exitPid = waitpid(-1, &status, WNOHANG)) ) {
      if (exitPid < 0) {
        result = ROUTER_ERR;
        goto re;
      }
      printf("[debug]router.c: exitPid = %d\n", exitPid);
      for (i = 0; i < total; ++i) {
        if (exitPid == pids[i]) {
          if (status == REDIS_OK) ++successNum;
          ++exitNum;
          break;
        }
      }
      if (i < total) break;
    }
  }
  if (successNum < threshold) {
    result = ROUTER_FAILED;
    printf("[debug]router.c: successNum = %d\n", successNum);
    goto re;
  }
  result = ROUTER_OK;
  goto re;

re:
  for (i = 0; i < total; ++i) kill(pids[i], SIGTERM);
  return result;
}
// Creates a bucket at the database considering consistent hashing.
// @returns ROUTER_OK when at least W machines succeed.
//
// I choose N machines according to consistent hashing, and inserts a
// pre-defined blob into those machines. Hence if a bucket with a same
// @bucketId already exists, no bad effect would take place.
int routeCreateBucket(const char *bucketId) {
  int rv;
  int i;
  int ipNum;
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for blob saving.
  getHostsByKey(bucketId, &ipNum, ips);

  // Saves blob onto those redis servers.
  // Piggybacking zombies cleaing.
  _cleanZombies();
  if (ipNum > N) ipNum = N;
  for (i = 0; i < ipNum; ++i) {
    pid = fork();
    if (pid < 0) {
      printf("router.c: %s %s\n", "Error creating bucket.", "Error forking.");
      return ROUTER_ERR;
    }
    if (pid == 0) {  // child process
      rv = hSet(ips[i], bucketId, EXIST_BLOB_ID, strlen(EXIST_BLOB),
          EXIST_BLOB);
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, W, pids);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Error creating bucket.",
        "Error waiting for children processes to respond.");
    return ROUTER_ERR;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to create bucket.",
        "Failed to wait for children processes to respond.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}

// Deletes a blob from the database considering consistent hashing.
//
// I choose N machines according to consistent hashing, and inserts a
// pre-defined blob into those machines marking as deletion.
int routeDeleteBlob(const char *bucketId, const char *blobId) {
  int rv;
  int i;
  int ipNum;
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for blob saving.
  getHostsByKey(bucketId, &ipNum, ips);

  // Saves blob onto those redis servers.
  // Piggybacking zombies cleaing.
  _cleanZombies();
  if (ipNum > N) ipNum = N;
  for (i = 0; i < ipNum; ++i) {
    pid = fork();
    if (pid < 0) {
      printf("router.c: %s %s\n", "Error deleting blob.", "Error forking.");
      return ROUTER_ERR;
    }
    if (pid == 0) {  // child process
      rv = hSet(ips[i], bucketId, blobId, strlen(NIL_BLOB), NIL_BLOB);
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, W, pids);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Error deleting blob.",
        "Error waiting for children processes to respond.");
    return ROUTER_ERR;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to delete blob.",
        "Failed to wait for children processes to respond.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}

// Deletes a bucket from the database considering consistent hashing.
//
// I choose N machines according to consistent hashing, and inserts a
// pre-defined blob into those machines marking as deletion.
int routeDeleteBucket(const char *bucketId) {
  int rv;
  int sockfd;

  // Finds the Redis server for bucket deletion.
  sockfd = _chooseMachine(bucketId, 1);
  //printf("[debug]router.c: sockfd = %d\n", sockfd);  //debug
  if (sockfd == -1) return ROUTER_OK;

  // Deletes bucket from that Redis server.
  rv = del(sockfd, bucketId);
  close(sockfd);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Failed to delete bucket.",
        "Lower layer replies with error.");
    return ROUTER_FAILED;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to delete bucket.",
        "Lower layer fails to.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}

/*
// Determines whether a blob exists in the database considering consistent
// hashing.
//
// I choose a machine according to consistent hashing, and determines at that
// machine.
int routeExistBlob(const char *bucketId, const char *blobId, int *result) {
  int rv;
  int sockfd;

  // Finds the Redis server for blob existence determination.
  sockfd = _chooseMachine(bucketId, 1);
  if (sockfd == -1) {
    *result = 0;
    return ROUTER_OK;
  }

  // Determines existence of blob at that Redis server.
  rv = hExists(sockfd, bucketId, blobId, result);
  close(sockfd);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Failed to determine existence of blob.",
        "Lower layer replies with error.");
    return ROUTER_FAILED;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to determine existence of blob.",
        "Lower layer fails to.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}

// Determines whether a bucket exists in the database considering consistent
// hashing.
//
// I choose a machine according to consistent hashing, and determines at that
// machine.
int routeExistBucket(const char *bucketId, int *result) {
  int rv;
  int sockfd;

  // Finds the Redis server for bucket existence determination.
  sockfd = _chooseMachine(bucketId, 1);
  if (sockfd == -1) {
    *result = 0;
    return ROUTER_OK;
  }

  // Determines existence of bucket at that Redis server.
  rv = exists(sockfd, bucketId, result);
  close(sockfd);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Failed to determine existence of bucket.",
        "Lower layer replies with error.");
    return ROUTER_FAILED;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to determine existence of bucket.",
        "Lower layer fails to.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}

// Loads a blob from the database considering consistent hashing.
//
// I choose a machine according to consistent hashing, and loads the data from
// that machine. If no such machine is found, @blobLength is set to -1.
int routeLoadBlob(const char *bucketId, const char *blobId, int *blobLength,
    void *blob) {
  int rv;
  int sockfd;

  // Finds the Redis server for blob loading.
  sockfd = _chooseMachine(bucketId, 1);
  if (sockfd == -1) {
    *blobLength = -1;
    return ROUTER_OK;
  }

  // Loads blob from that Redis server.
  rv = hGet(sockfd, bucketId, blobId, blobLength, blob);
  close(sockfd);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Failed to load blob.",
        "Lower layer replies with error.");
    return ROUTER_FAILED;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to load blob.",
        "Lower layer fails to.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}
*/
// Saves a blob into the database considering consistent hashing.
// @returns ROUTER_OK when at least W machines succeed.
//
// I choose N machines according to consistent hashing, and save the data onto
// those machines. @returns ROUTER_OK when at least W machines succeed.
int routeSaveBlob(const char *bucketId, const char *blobId,
    const int blobLength, const void *blob) {
  int rv;
  int i;
  int ipNum;
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for blob saving.
  getHostsByKey(bucketId, &ipNum, ips);

  // Saves blob onto those redis servers.
  // Piggybacking zombies cleaing.
  _cleanZombies();
  if (ipNum > N) ipNum = N;
  for (i = 0; i < ipNum; ++i) {
    pid = fork();
    if (pid < 0) {
      printf("router.c: %s %s\n", "Error saving blob.", "Error forking.");
      return ROUTER_ERR;
    }
    if (pid == 0) {  // child process
      rv = hSet(&ips[i], bucketId, blobId, blobLength, blob);
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, W, pids);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Error saving blob.",
        "Error waiting for children processes to respond.");
    return ROUTER_ERR;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to save blob.",
        "Failed to wait for children processes to respond.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}
