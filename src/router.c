/* Refer to the comments at the beginning of "router.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "def.h"
#include "md5.h"
#include "redis.h"
#include "router.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Global consistent hashing map.
ConsistentHashingMap map;

// Creates a ConsistentHashingNode and add it to a ConsistentHashingMap.
// @returns that node.
chNode *addNode(const sockaddr *addr, chMap *map) {
  chNode *node, *p;

  if (!addr || !map) return 0;

  // Creates a new node.
  node = (chNode *) malloc(sizeof(chNode));
  if (!node) {
    printf("router.c: %s %s\n", "Failed to create node.",
        "Insufficient memory.");
    return 0;
  }
  memset(node, 0, sizeof(chNode));
  memcpy(&node->addr, &((sockaddr_in *) addr)->sin_addr, sizeof(in_addr));

  // Adds that node to the map.
  if (!map->nNode) {
    map->list = node;
  } else {
    p = map->list;
    while (p->next) p = p->next;
    p->next = node;
  }
  ++ map->nNode;
  return node;
}

// Creates a ConsistentHashingVirtualNode and add it to a ConsistentHashingNode.
// @returns that virtual node.
chVirtualNode *addVirtualNode(const char *key, chNode *node) {
  chVirtualNode *virtualNode, *p;

  if (!node) return 0;

  // Creates a new virtual node.
  virtualNode = (chVirtualNode *) malloc(sizeof(virtualNode));
  if (!virtualNode) {
    printf("router.c: %s %s\n", "Failed to create virtual node.",
        "Insufficient memory.");
    return 0;
  }
  md5(key, virtualNode->hash);
  virtualNode->next = 0;

  // Adds that virtual node to the node.
  if (!node->nVirtualNode) {
    node->list = virtualNode;
  } else {
    p = node->list;
    while (p->next) p = p->next;
    p->next = virtualNode;
  }
  ++ node->nVirtualNode;
  return virtualNode;
}

// Frees all the memory pre-allocated by the consistent hashing map.
void _freeNodes(chMap *map) {
  chNode *node, *p;
  chVirtualNode *virtualNode, *q;

  if (!map || !map->nNode) return;

  node = map->list;
  while (node) {
    p = node;
    node = node->next;
    virtualNode = p->list;
    while (virtualNode) {
      q = virtualNode;
      virtualNode = virtualNode->next;
      free(q);
    }
    free(p);
  }
  map->list = 0;
  map->nNode = 0;
}

// Displays information about the consistent hashing node.
void _dumpNode(const chNode *node) {

  if (!node) {
    printf("[debug]router.c: %s %s\n", "Failed to dump node.", "Null pointer.");
    return;
  }

  printf("[debug]router.c: ip: %s\n", inet_ntoa(node->addr));
  printf("[debug]router.c: number of virtual nodes: %d\n", node->nVirtualNode);
}

// Displays information about the consistent hashing map.
void _dumpMap(const chMap *map) {
  chNode *node;

  if (!map) {
    printf("[debug]router.c: %s %s\n", "Failed to dump map.", "Null pointer.");
    return;
  }
  
  printf("[debug]router.c: Number of nodes: %d\n", map->nNode);
  node = map->list;
  while (node) {
    printf("[debug]router.c: --\n");
    _dumpNode(node);
    node = node->next;
  }
  return;
}

// Loads hostnames of live servers from config file.
//
// Each line in the config file is a hostname/ip of a live server.
//
// TODO: This is a dummy version for demo.
//
void _loadLiveServers(const char *configFile, chMap *map) {
  FILE *fin;
  char line[MAX_LINE_LENGTH];
  int rv;
  struct addrinfo hints, *result;

  // Frees all the memory pre-allocated by the consistent hashing map.
  if (!map) return;
  if (!map->nNode) _freeNodes(map);

  // Adds live servers to the map according to the config file.
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  fin = fopen(CONFIG_FILE, "r");
  rv = fscanf(fin, "%s", line);
  while (!feof(fin)) {
    rv = getaddrinfo(line, NULL, &hints, &result);
    addNode(result->ai_addr, map);
    freeaddrinfo(result);
    rv = fscanf(fin, "%s", line);
  }
  fclose(fin);
}

// Makes a connection to the destination consistent hashing node.
// @returns file descriptor of the socket connection.
int _connectToNode(const chNode *node) {
  int sockfd;
  int rv;
  struct sockaddr_in serv_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(REDIS_PORT);
  memcpy(&serv_addr.sin_addr, &node->addr, sizeof(in_addr));
  rv = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

  if (rv < 0) {
    printf("router.c: %s %s\n", "Error connecting to Redis server.",
        inet_ntoa(node->addr));
    return -1;
  }
  return sockfd;
}

// Chooses a machine according to consistent hashing.
// @returns the file descriptor of the socket connection to the machine, or -1
// if no such machine is available.
//
// Caller of this function should close the connection.
//
// TODO: This is a dummy version for demo.
// If there is already a machine associated with that key, that machine is
// chosen.
// Otherwise, if @mustExist is 0, @returns -1.
// Otherwise, if @mustExist is 1, I just randomly pick a machine.
// If the connection to the chosen machine cannot be established, @returns -1.
//
int _chooseMachine(const char *key, const int mustExist) {
  chNode *node;
  chVirtualNode *virtualNode;
  unsigned char hash[MAX_HASH_LENGTH];
  int sockfd;
  int i, n;

  // Loads live servers information into the map.
  if (!map.list) _loadLiveServers(CONFIG_FILE, &map);

  // Chooses the machine associated with that key, if any.
  md5(key, hash);
  node = map.list;
  while (node) {
    virtualNode = node->list;
    while (virtualNode) {
      if (!memcmp(virtualNode->hash, hash, MD5_LENGTH)) {
        sockfd = _connectToNode(node);
        return sockfd;
      }
    }
    node = node->next;
  }

  // No machine associated with that key, behaves according to @mustExist.
  if (mustExist) return -1;
  // @mustExist = 0
  if (!map.nNode) return -1;
  srand((unsigned int) time(NULL));
  n = rand() % map.nNode;
  for (node = map.list, i = 0; i < n; ++i) node = node->next;
  //_dumpNode(node);  // debug
  addVirtualNode(key, node);
  sockfd = _connectToNode(node);
  return sockfd;
}

// Creates a bucket at the database considering consistent hashing.
//
// I choose a machine according to consistent hashing, and inserts a
// pre-defined blob into that machine. Hence if a bucket with a same @bucketId
// already exists, no bad effect would take place.
int routeCreateBucket(const char *bucketId) {
  int rv;
  int sockfd;

  // Finds the Redis server for bucket creation.
  sockfd = _chooseMachine(bucketId, 0);
  //printf("[debug]router.c: sockfd = %d\n", sockfd);  //debug
  if (sockfd == -1) {
    printf("router.c: %s %s\n", "Failed to create bucket.",
        "No Redis server available.");
    return ROUTER_FAILED;
  }

  // Creates bucket at that Redis server.
  rv = hSet(sockfd, bucketId, EXIST_BLOB_ID, strlen(EXIST_BLOB), EXIST_BLOB);
  close(sockfd);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Failed to create bucket.",
        "Lower layer replies with error.");
    return ROUTER_FAILED;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to create bucket.",
        "Lower layer fails to.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}

// Deletes a blob from the database considering consistent hashing.
//
// I choose a machine according to consistent hashing, and deletes the data from
// that machine.
int routeDeleteBlob(const char *bucketId, const char *blobId) {
  int rv;
  int sockfd;

  // Finds the Redis server for blob deletion.
  sockfd = _chooseMachine(bucketId, 1);
  //printf("[debug]router.c: sockfd = %d\n", sockfd);  //debug
  if (sockfd == -1) return ROUTER_OK;

  // Deletes blob from that Redis server.
  rv = hDel(sockfd, bucketId, blobId);
  close(sockfd);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Failed to delete blob.",
        "Lower layer replies with error.");
    return ROUTER_FAILED;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to delete blob.",
        "Lower layer fails to.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}

// Deletes a bucket from the database considering consistent hashing.
//
// I choose a machine according to consistent hashing, and deletes the data from
// that machine.
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

// Saves a blob into the database considering consistent hashing.
//
// I choose a machine according to consistent hashing, and save the data onto
// that machine.
int routeSaveBlob(const char *bucketId, const char *blobId,
    const int blobLength, const void *blob) {
  int rv;
  int sockfd;

  // Finds a Redis server for blob saving.
  sockfd = _chooseMachine(bucketId, 0);
  if (sockfd == -1) {
    printf("router.c: %s %s\n", "Failed to save blob.",
        "No Redis server available.");
    return ROUTER_FAILED;
  }

  // Saves blob onto that Redis server.
  rv = hSet(sockfd, bucketId, blobId, blobLength, blob);
  close(sockfd);
  if (rv == REDIS_ERR) {
    printf("router.c: %s %s\n", "Failed to save blob.",
        "Lower layer replies with error.");
    return ROUTER_FAILED;
  } else if (rv == REDIS_FAILED) {
    printf("router.c: %s %s\n", "Failed to save blob.",
        "Lower layer fails to.");
    return ROUTER_FAILED;
  }
  return ROUTER_OK;
}
