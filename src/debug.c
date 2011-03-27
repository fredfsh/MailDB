#include "redis.h"
#include <strings.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
  int sockfd;
  int rv;
  int result;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  int blobLength;
  char blob[MAX_BLOB_LENGTH];

  server = gethostbyname("alpha");
  serv_addr.sin_family = AF_INET;
  bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr,
      server->h_length);
  serv_addr.sin_port = htons(6379);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  rv = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (rv < 0) {
    printf("test.c: %s\n", "Error connecting to Redis.");
    return 0;
  }

  rv = hDel(sockfd, "fredfsh", "three");
  if (rv == REDIS_OK) {
    printf("Success.\n");
  } else if (rv == REDIS_FAILED) {
    printf("Failed.\n");
  } else if (rv == REDIS_ERR) {
    printf("Error.\n");
  }

  rv = hExists(sockfd, "fredfsh", "three", &result);
  if (rv == REDIS_OK) {
    printf("%d\n", result);
  } else if (rv == REDIS_FAILED) {
    printf("Failed.\n");
  } else if (rv == REDIS_ERR) {
    printf("Error.\n");
  }

  rv = hGet(sockfd, "fredfsh", "three", &blobLength, blob);
  if (rv == REDIS_OK) {
    if (blobLength == - 1) {
      printf("(nil)\n");
    } else {
      blob[blobLength == MAX_BLOB_LENGTH ? MAX_BLOB_LENGTH - 1 : blobLength]
          = '\0';  // for debug
      printf("%s\n", blob);
    }
  } else if (rv == REDIS_FAILED) {
    printf("Failed.\n");
  } else if (rv == REDIS_ERR) {
    printf("Error.\n");
  }

  rv = hSet(sockfd, "fredfsh", "three", 1, "3");
  if (rv == REDIS_OK) {
    printf("Success.\n");
  } else if (rv == REDIS_FAILED) {
    printf("Failed.\n");
  } else if (rv == REDIS_ERR) {
    printf("Error.\n");
  }

  rv = hExists(sockfd, "fredfsh", "three", &result);
  if (rv == REDIS_OK) {
    printf("%d\n", result);
  } else if (rv == REDIS_FAILED) {
    printf("Failed.\n");
  } else if (rv == REDIS_ERR) {
    printf("Error.\n");
  }

  rv = hGet(sockfd, "fredfsh", "three", &blobLength, blob);
  if (rv == REDIS_OK) {
    if (blobLength == - 1) {
      printf("(nil)\n");
    } else {
      blob[blobLength == MAX_BLOB_LENGTH ? MAX_BLOB_LENGTH - 1 : blobLength]
          = '\0';  // for debug
      printf("%s\n", blob);
    }
  } else if (rv == REDIS_FAILED) {
    printf("Failed.\n");
  } else if (rv == REDIS_ERR) {
    printf("Error.\n");
  }

  return 1;
}
