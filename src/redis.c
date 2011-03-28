/* Refer to the comments at the beginning of "redis.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "def.h"
#include "redis.h"
#include <sys/socket.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Assembles a binary safe command stream according to Redis protocol, not
// including the last blob content stream.
//
// Content of the unified request protocol:
//
//   *<number of arguments> CR LF
//   $<number of bytes of argument 1> CR LF
//   <argument data> CR LF
//   ...
//   $<number of bytes of argument N> CR LF
//   <argument data> CR LF
//
// The @fmt is similar as that used by printf, but simpler.
//   s: const char *
//   b: number of bytes of blob content, only occurs last
int _commandStream(char *commandStream, const char *fmt, ...) {
  va_list ap;
  int n;
  char *s;
  char temp[MAX_COMMAND_LENGTH];

  sprintf(commandStream, "*%zu\r\n", strlen(fmt));
  va_start(ap, fmt);
  while (*fmt) {
    switch (*fmt++) {
      case 's':
        s = va_arg(ap, char *);
        sprintf(temp, "$%zu\r\n%s\r\n", strlen(s), s);
        strcat(commandStream, temp);
        break;
      case 'b':
        n = va_arg(ap, int);
        sprintf(temp, "$%d\r\n", n);
        strcat(commandStream, temp);
        break;
    }
  }
  va_end(ap);
  return REDIS_OK;
}

// Receives blob content from a Redis server.
//
// @streamLength is set to -1 if the blob doesn't exist.
int _recvBlob(const int sockfd, int *streamLength, void *blob) {
  int retry;
  int n;
  int rv;
  char *pos;
  char reply[MAX_COMMAND_LENGTH];

  // Reads metadata from Redis reply.
  retry = REDIS_RETRY;
  while (--retry >= 0) {
    rv = recv(sockfd, reply, MAX_COMMAND_LENGTH, MSG_DONTWAIT | MSG_PEEK);
    //printf("[debug]redis.c: recv buffer length: %d.\n", rv);
    if (rv <= 0) {
      usleep(REDIS_RETRY_SLEEP);
      continue;
    }
    if (reply[0] == '-') {
      printf("redis.c: %s %s\n", "Failed to receive blob from Redis.",
          "Redis server replies with error.");
      return REDIS_FAILED;
    } else if (reply[0] != '$') {
      printf("redis.c: %s %s\n", "Failed to receive blob from Redis.",
          "No buffer length indicated in Redis server's reply.");
      return REDIS_FAILED;
    }
    break;
  }
  if (retry < 0) {
      printf("redis.c: %s %s\n", "Failed to receive blob from Redis.",
          "No reply from server.");
      return REDIS_FAILED;
  }

  // Reads length of blob content from Redis reply.
  pos = strstr(reply, "\r\n");
  if (!pos) {
      printf("redis.c: %s %s\n", "Failed to receive blob from Redis.",
          "No buffer length indicated in Redis server's reply.");
      return REDIS_FAILED;
  }
  // Cleans metadata in the recv buffer.
  rv = recv(sockfd, reply, (pos - reply) + 2, MSG_DONTWAIT);
  n = atoi(&reply[1]);
  if (n > MAX_BLOB_LENGTH) {
      printf("redis.c: %s %s\n", "Warning.",
          "Blob content exceeding buffer length truncated.");
      n = MAX_BLOB_LENGTH;
  }
  *streamLength = n;
  if (n == -1) return REDIS_OK;

  // Reads blob content from Redis reply.
  retry = REDIS_RETRY;
  while (--retry >= 0) {
    rv = recv(sockfd, blob, n, MSG_DONTWAIT);
    if (rv <= 0) {
      usleep(REDIS_RETRY_SLEEP);
      continue;
    } else if (rv != n) {
      printf("redis.c: %s %s\n", "Failed to receive blob from Redis.",
          "Length of blob content not equals as indicated.");
      return REDIS_FAILED;
    }
    break;
  }
  if (retry < 0) {
      printf("redis.c: %s %s\n", "Failed to receive blob from Redis.",
          "No reply of blob content from server.");
      return REDIS_FAILED;
  }
  rv = recv(sockfd, reply, 2, MSG_DONTWAIT);
  return REDIS_OK;
}

// Receives an integer reply from Redis server.
//
// Integer replies are preceded by ":".
int _recvInt(const int sockfd, int *result) {
  int retry;
  int rv;
  char reply[MAX_COMMAND_LENGTH];

  retry = REDIS_RETRY;
  while (--retry >= 0) {
    rv = recv(sockfd, reply, MAX_COMMAND_LENGTH, MSG_DONTWAIT);
    //printf("[debug]redis.c: recv buffer length: %d.\n", rv);
    if (rv <= 0) {
      usleep(REDIS_RETRY_SLEEP);
      continue;
    }
    if (reply[0] == '-') {
      printf("redis.c: %s %s\n", "Failed to receive integer from Redis.",
          "Redis server replies with error.");
      return REDIS_FAILED;
    } else if (reply[0] != ':') {
        printf("redis.c: %s %s\n", "Failed to receive integer from Redis.",
            "No integer replied from Redis.");
        return REDIS_FAILED;
    }
    break;
  }
  if (retry < 0) {
      printf("redis.c: %s %s\n", "Failed to execute HSET.",
          "No reply from server.");
      return REDIS_FAILED;
  }
  *result = atoi(&reply[1]);
  return REDIS_OK;
}

//Executes HDEL command to a Redis server.
int hDel(const int sockfd, const char *key, const char *field) {
  int rv;
  int result;
  char commandStream[MAX_COMMAND_LENGTH];

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "sss", "HDEL", key, field);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HDEL.", "Command not sent.");
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute HDEL.", "Command not sent.");
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, &result);
  return rv;
}

//Executes EXISTS command to a Redis server.
int exists(const int sockfd, const char *key, int *result) {
  int rv;
  char commandStream[MAX_COMMAND_LENGTH];

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "ss", "EXISTS", key);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing EXISTS.", "Command not sent.");
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute EXISTS.",
        "Command not sent.");
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, result);
  return rv;
}

//Executes DEL command to a Redis server.
int del(const int sockfd, const char *key) {
  int rv;
  int result;
  char commandStream[MAX_COMMAND_LENGTH];

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "ss", "DEL", key);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing DEL.", "Command not sent.");
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute DEL.", "Command not sent.");
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, &result);
  return rv;
}

//Executes HEXISTS command to a Redis server.
int hExists(const int sockfd, const char *key, const char *field, int *result) {
  int rv;
  char commandStream[MAX_COMMAND_LENGTH];

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "sss", "HEXISTS", key, field);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HEXISTS.", "Command not sent.");
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute HEXISTS.",
        "Command not sent.");
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, result);
  return rv;
}

// Executes HGET command to a Redis server.
//
// @streamLength is set to -1 if the blob doesn't exist.
int hGet(const int sockfd, const char *key, const char *field,
    int *blobLength, void *blob) {
  int rv;
  char commandStream[MAX_COMMAND_LENGTH];

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "sss", "HGET", key, field);
  //printf("[debug]redis.c: commandStream: %s\n", commandStream);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HGET.", "Command not sent.");
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute HGET.", "Command not sent.");
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvBlob(sockfd, blobLength, blob);
  return rv;
}

// Executes HSET command to a redis server.
int hSet(const int sockfd, const char *key, const char *field,
    const int blobLength, const void *blob) {
  int rv;
  int result;
  char commandStream[MAX_COMMAND_LENGTH];

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "sssb", "HSET", key, field, blobLength);
  //printf("[debug]redis.c: command length: %zu.", strlen(commandStream));
  //printf("[debug]redis.c: %s", commandStream);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HSET.", "Command not sent.");
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute HSET.", "Command not sent.");
    return REDIS_FAILED;
  }

  // Writes blob content to Redis server.
  rv = write(sockfd, blob, blobLength);
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HSET.", "Blob not sent.");
    return REDIS_ERR;
  } else if (rv != blobLength) {
    printf("redis.c: %s %s\n", "Failed to execute HSET.", "Blob not sent.");
    return REDIS_FAILED;
  }
  rv = write(sockfd, "\r\n", 2);

  // Reads Redis reply.
  rv = _recvInt(sockfd, &result);
  return rv;
}
