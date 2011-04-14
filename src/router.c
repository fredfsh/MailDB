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

// Reads blob.
int _readBlob(const int fd, void *blob) {
  int rv;
  int blobLength;
  unsigned char buffer[MAX_BLOB_LENGTH];

  // Reads length metadata.
  rv = read(fd, &blobLength, sizeof(int));
  if (rv == -1) return ROUTER_ERR;
  if (rv != sizeof(int)) return ROUTER_FAILED;

  // Reads real blob content.
  rv = read(fd, buffer, blobLength - sizeof(int));
  if (rv == -1) return ROUTER_ERR;
  if (rv != blobLength - (int) sizeof(int)) return ROUTER_FAILED;

  memcpy(blob, &blobLength, sizeof(int));
  memcpy(&((int *) blob)[1], buffer, blobLength - sizeof(int));
  return ROUTER_OK;
}

// Waits for enough children processes to exit.
// @returns ROUTER_OK if at least @threshold among @total children processes
// responses successfully. @returns ROUTER_ERR immediately once
// REDIS_FATAL_ERR is received.
int _waitChildren(const int total, const int threshold, const pid_t *pids) {
  int exitNum, successNum;
  int retry;
  int status;
  int i;
  pid_t exitPid;

  exitNum = 0;
  successNum = 0;
  retry = WAIT_RETRY;
  while (exitNum < total && successNum < threshold && --retry >= 0) {
    usleep(WAIT_RETRY_INTERVAL);
    while ( (exitPid = waitpid(-1, &status, WNOHANG)) ) {
      if (exitPid < 0) return ROUTER_ERR;
      //printf("[debug]router.c: exitPid = %d\n", exitPid);  // debug
      for (i = 0; i < total; ++i) {
        if (exitPid == pids[i]) {
          //printf("[debug]router.c: status = %d\n", status);  // debug
          if (status == REDIS_FATAL_ERR) {
            printf("router.c: %s\n", "Fatal error received from child!");
            return ROUTER_ERR;
          }
          if (status == REDIS_OK) ++successNum;
          ++exitNum;
          break;
        }
      }
      if (i < total) break;
    }
  }
  if (successNum < threshold) return ROUTER_FAILED;
  return ROUTER_OK;
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
  int blobLength;
  unsigned char blob[MAX_BLOB_LENGTH];
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for blob saving.
  getHostsByKey(bucketId, &ipNum, ips);

  // Saves blob onto those redis servers.
  blobLength = strlen(EXIST_BLOB) + sizeof(int);
  memcpy(blob, &blobLength, sizeof(int));
  memcpy(&blob[sizeof(int)], EXIST_BLOB, strlen(EXIST_BLOB));
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
      rv = hSet(&ips[i], bucketId, EXIST_BLOB_ID, blob);
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, W, pids);
  if (rv == ROUTER_ERR) {
    printf("router.c: %s %s\n", "Error creating bucket.",
        "Error waiting for children processes to respond.");
  } else if (rv == ROUTER_FAILED) {
    printf("router.c: %s %s\n", "Failed to create bucket.",
        "Failed to wait for children processes to respond.");
  }
  return rv;
}

// Deletes a blob from the database considering consistent hashing.
//
// I choose N machines according to consistent hashing, and deletes the blob
// on all of them.
int routeDeleteBlob(const char *bucketId, const char *blobId) {
  int rv;
  int i;
  int ipNum;
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for blob saving.
  getHostsByKey(bucketId, &ipNum, ips);

  // Deletes blob from those redis servers.
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
      rv = hDel(&ips[i], bucketId, blobId);
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, N, pids);
  if (rv == ROUTER_ERR) {
    printf("router.c: %s %s\n", "Error deleting blob.",
        "Error waiting for children processes to respond.");
  } else if (rv == ROUTER_FAILED) {
    printf("router.c: %s %s\n", "Failed to delete blob.",
        "Failed to wait for children processes to respond.");
  }
  return rv;
}

// Deletes a bucket from the database considering consistent hashing.
//
// I choose N machines according to consistent hashing, and deletes the bucket
// on all of them.
int routeDeleteBucket(const char *bucketId) {
  int rv;
  int i;
  int ipNum;
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for blob saving.
  getHostsByKey(bucketId, &ipNum, ips);

  // Deletes bucket from those redis servers.
  // Piggybacking zombies cleaing.
  _cleanZombies();
  if (ipNum > N) ipNum = N;
  for (i = 0; i < ipNum; ++i) {
    pid = fork();
    if (pid < 0) {
      printf("router.c: %s %s\n", "Error deleting bucket.", "Error forking.");
      return ROUTER_ERR;
    }
    if (pid == 0) {  // child process
      rv = del(&ips[i], bucketId);
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, N, pids);
  if (rv == ROUTER_ERR) {
    printf("router.c: %s %s\n", "Error deleting bucket.",
        "Error waiting for children processes to respond.");
  } else if (rv == ROUTER_FAILED) {
    printf("router.c: %s %s\n", "Failed to delete bucket.",
        "Failed to wait for children processes to respond.");
  }
  return rv;
}

// Determines whether a blob exists in the database considering consistent
// hashing.
// @returns ROUTER_OK when existed.
//
// I choose N machines according to consistent hashing. If any of them
// contains the target, the blob is regarded as existed.
int routeExistBlob(const char *bucketId, const char *blobId) {
  int rv;
  int i;
  int ipNum;
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for blob existence determination.
  getHostsByKey(bucketId, &ipNum, ips);

  // Determining existence of blob on those redis servers.
  // Piggybacking zombies cleaing.
  _cleanZombies();
  if (ipNum > N) ipNum = N;
  for (i = 0; i < ipNum; ++i) {
    pid = fork();
    if (pid < 0) {
      printf("router.c: %s %s\n", "Error determining existence of blob.",
          "Error forking.");
      return ROUTER_ERR;
    }
    if (pid == 0) {  // child process
      rv = hExists(&ips[i], bucketId, blobId);
      //printf("[debug]router.c: rv = %d\n", rv);  // debug
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, 1, pids);
  if (rv == ROUTER_ERR) {
    printf("router.c: %s %s\n", "Error determining existence of blob.",
        "Error waiting for children processes to respond.");
  }
  return rv;
}

// Determines whether a bucket exists in the database considering consistent
// hashing.
// @returns ROUTER_OK when existed.
//
// I choose N machines according to consistent hashing. If any of them
// contains the target, the bucket is regarded as existed.
int routeExistBucket(const char *bucketId) {
  int rv;
  int i;
  int ipNum;
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for bucket existence determination.
  getHostsByKey(bucketId, &ipNum, ips);

  // Determining existence of bucket on those redis servers.
  // Piggybacking zombies cleaing.
  _cleanZombies();
  if (ipNum > N) ipNum = N;
  for (i = 0; i < ipNum; ++i) {
    pid = fork();
    if (pid < 0) {
      printf("router.c: %s %s\n", "Error determining existence of bucket.",
          "Error forking.");
      return ROUTER_ERR;
    }
    if (pid == 0) {  // child process
      rv = exists(&ips[i], bucketId);
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, 1, pids);
  if (rv == ROUTER_ERR) {
    printf("router.c: %s %s\n", "Error determining existence of bucket.",
        "Error waiting for children processes to respond.");
  }
  return rv;
}

// Loads a blob from the database considering consistent hashing.
// @returns ROUTER_OK when at least R machines succeed.
// ( TODO: Conflict resolution. At present, if different version of data is
//   received, chosen randomly. )
//
// I choose N machines according to consistent hashing, and load the data from
// those machines.
int routeLoadBlob(const char *bucketId, const char *blobId, void *blob) {
  int rv, wrv;
  int i;
  int ipNum;
  int pipefd[2];
  int blobLength;
  int chosen;
  unsigned char rblob[MAX_BLOB_LENGTH];
  in_addr ips[C];
  pid_t pid, pids[N];

  // Finds N redis servers for blob loading.
  getHostsByKey(bucketId, &ipNum, ips);

  // Opens pipes for data transfer between parent and children.
  rv = pipe(pipefd);
  if (rv) {
    printf("router.c: %s %s\n", "Error loading blob.",
        "Error opening pipe for data transfer.");
    return ROUTER_ERR;
  }

  // Loads blob from those redis servers.
  // Piggybacking zombies cleaing.
  _cleanZombies();
  if (ipNum > N) ipNum = N;
  for (i = 0; i < ipNum; ++i) {
    pid = fork();
    if (pid < 0) {
      printf("router.c: %s %s\n", "Error loading blob.", "Error forking.");
      close(pipefd[0]);
      close(pipefd[1]);
      return ROUTER_ERR;
    }
    if (pid == 0) {  // child process
      // Child closes read end of pipe.\n
      close(pipefd[0]);
      rv = hGet(&ips[i], bucketId, blobId, rblob);
      if (rv == REDIS_OK) {
        memcpy(&blobLength, rblob, sizeof(int));
        // Child writes data to parent through pipe.
        wrv = write(pipefd[1], rblob, blobLength);
        if (wrv == -1) {
          perror("1");
          printf("router.c: %s %s\n", "Error loading blob.",
              "Child unable to write blob to parent through pipe.");
          rv = REDIS_ERR;
        } else if (wrv != blobLength) {
          printf("router.c: %s %s\n", "Fatal error!",
              "Child unable to write blob to parent through pipe.");
          rv = REDIS_FATAL_ERR;
        }
      }
      // Child closes write end of pipe.\n
      close(pipefd[1]);
      _exit(rv);
    }
    pids[i] = pid;
  }
  // Parent closes write end of pipe.\n
  close(pipefd[1]);

  // Collects responses from children processes.
  rv = _waitChildren(N, R, pids);
  if (rv == ROUTER_ERR) {
    printf("router.c: %s %s\n", "Error loading blob.",
        "Error waiting for children processes to respond.");
    close(pipefd[0]);
    return ROUTER_ERR;
  } else if (rv == ROUTER_FAILED) {
    printf("router.c: %s %s\n", "Failed to load blob.",
        "Failed to wait for children processes to respond.");
    close(pipefd[0]);
    return ROUTER_FAILED;
  }

  // Randomly chooses a version of data.
  chosen = (int) random() % R;
  for (int i = 0; i <= chosen; ++i) {
    rv = _readBlob(pipefd[0], rblob);
    if (rv == ROUTER_ERR) {
      printf("router.c: %s %s\n", "Error loading blob.",
          "Error reading children's response.");
      close(pipefd[0]);
      return ROUTER_ERR;
    } else if (rv == ROUTER_FAILED) {
      printf("router.c: %s %s\n", "Failed to load blob.",
          "Failed to read children's response.");
      close(pipefd[0]);
      return ROUTER_FAILED;
    }
  }
  close(pipefd[0]);
  memcpy(&blobLength, rblob, sizeof(int));
  memcpy(blob, rblob, blobLength);
  return ROUTER_OK;
}

// Saves a blob into the database considering consistent hashing.
// @returns ROUTER_OK when at least W machines succeed.
//
// I choose N machines according to consistent hashing, and save the data onto
// those machines.
int routeSaveBlob(const char *bucketId, const char *blobId, const void *blob) {
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
      rv = hSet(&ips[i], bucketId, blobId, blob);
      _exit(rv);
    }
    pids[i] = pid;
  }

  // Collects responses from children processes.
  rv = _waitChildren(N, W, pids);
  if (rv == ROUTER_ERR) {
    printf("router.c: %s %s\n", "Error saving blob.",
        "Error waiting for children processes to respond.");
  } else if (rv == ROUTER_FAILED) {
    printf("router.c: %s %s\n", "Failed to save blob.",
        "Failed to wait for children processes to respond.");
  }
  return rv;
}
