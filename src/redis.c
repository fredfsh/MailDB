/* Refer to the comments at the beginning of "redis.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "connectionpool.h"
#include "def.h"
#include "redis.h"
#include "threadpool.h"
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
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

// Receives blob content from a redis server.
int _recvBulk(const int sockfd, void *blob) {
  int retry;
  int n;
  int rv, rrv;
  char *pos;
  char reply[MAX_COMMAND_LENGTH];

  // Reads metadata from Redis reply.
  retry = REDIS_RETRY;
  while (--retry >= 0) {
    rv = recv(sockfd, reply, MAX_COMMAND_LENGTH, MSG_DONTWAIT | MSG_PEEK);
    //printf("[debug]redis.c: recv buffer length: %d.\n", rv);
    if (rv <= 0) {
      usleep(REDIS_RETRY_INTERVAL);
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
  reply[(pos - reply) + 2] = '\0';
  n = atoi(&reply[1]);
  //printf("[debug]redis.c: pos = %ld, n = %d\n", pos - reply + 2, n);  // debug
  if (n > MAX_BLOB_LENGTH) {
    printf("redis.c: %s %s\n", "Warning.",
        "Blob content exceeding buffer length truncated.");
    n = MAX_BLOB_LENGTH;
  } else if (n < 0) {
    n = sizeof(int);
  }
  memcpy(blob, &n, sizeof(int));

  // Reads blob content from Redis reply.
  if (n == sizeof(int)) return REDIS_OK;
  retry = REDIS_RETRY;
  rv = 0;
  while (--retry >= 0) {
    //printf("[debug]redis.c: pre-rv = %d\n", rv);  // debug
    rrv = recv(sockfd, &((unsigned char *) blob)[sizeof(int) + rv], n - rv,
        MSG_DONTWAIT);
    if (rrv != -1) rv += rrv;
    //printf("[debug]redis.c: post-rv = %d\n", rv);  // debug
    if (rv == n) break;
    usleep(REDIS_RETRY_INTERVAL);
  }
  if (retry < 0) {
    if (rv == 0) {
      printf("redis.c: %s %s\n", "Failed to receive blob from Redis.",
          "No reply of blob content from server.");
      return REDIS_FAILED;
    } else {
      printf("redis.c: %s %s\n", "Failed to receive blob from Redis.",
          "Length of blob content not equals as indicated.");
      printf("[debug]redis.c: pos = %ld, reply = %s\n", pos - reply, reply);  // debug
      printf("[debug]redis.c: rv = %d, n = %d\n", rv, n);  // debug
      return REDIS_FAILED;
    }
  }
  // Cleans trailing "\r\n" in the socket buffer.
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
      usleep(REDIS_RETRY_INTERVAL);
      continue;
    }
    if (reply[0] == '-') {
      printf("redis.c: %s %s\n", "Failed to receive integer.",
          "Redis server replies with error.");
      return REDIS_FAILED;
    } else if (reply[0] != ':') {
        printf("redis.c: %s %s\n", "Failed to receive integer.",
            "No integer reply from Redis.");
        return REDIS_FAILED;
    }
    break;
  }
  if (retry < 0) {
      printf("redis.c: %s %s\n", "Failed to receive integer.",
          "No reply from Redis.");
      return REDIS_FAILED;
  }
  *result = atoi(&reply[1]);
  return REDIS_OK;
}

// Receives a status reply from Redis server.
//
// Status replies are preceded by "+".
int _recvStatus(const int sockfd, char *status) {
  int retry;
  int rv;
  int pos;
  char reply[MAX_COMMAND_LENGTH];

  retry = REDIS_RETRY;
  while (--retry >= 0) {
    rv = recv(sockfd, reply, MAX_COMMAND_LENGTH, MSG_DONTWAIT);
    if (rv <= 0) {
      usleep(REDIS_RETRY_INTERVAL);
      continue;
    }
    if (reply[0] == '-') {
      printf("redis.c: %s %s\n", "Failed to receive status.",
          "Redis server replies with error.");
      return REDIS_FAILED;
    } else if (reply[0] != '+') {
        printf("redis.c: %s %s\n", "Failed to receive status.",
            "No status reply from Redis.");
        return REDIS_FAILED;
    }
    break;
  }

  if (retry < 0) {
      printf("redis.c: %s %s\n", "Failed to receive status.",
          "No reply from server.");
      return REDIS_FAILED;
  }
  pos = strstr(reply, "\r\n") - reply;
  strncpy(status, &reply[1], pos - 1);
  status[pos - 1] = '\0';
  return REDIS_OK;
}

//Executes DEL command to a Redis server.
int del(void *arg) {
  int sockfd;
  int rv;
  int result;
  char commandStream[MAX_COMMAND_LENGTH];
  Connection *connection;
  ThreadTask *threadTask;

  // Casts arguments.
  threadTask = (ThreadTask *) arg;

  // Connects to redis server.
  connection = openConnection(threadTask->ip);
  if (!connection) {
    printf("redis.c: %s %s\n", "Failed to execute DEL.",
        "Failed to get idle connection from pool.");
    return REDIS_FAILED;
  }
  sockfd = connection->sockfd;

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "ss", "DEL", threadTask->redisCommand->key);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing DEL.", "Command not sent.");
    closeConnection(connection);
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute DEL.", "Command not sent.");
    closeConnection(connection);
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, &result);
  if (rv != REDIS_OK) {
    closeConnection(connection);
    return rv;
  }
  
  // Writes back result.
  rv = writeResult(EXIST_UNDEFINED, NULL, threadTask);
  if (rv == THREADPOOL_FATAL_ERR) {
    rv = REDIS_FATAL_ERR;
  } else if (rv != THREADPOOL_OK) {
    rv = REDIS_FAILED;
  } else {
    rv = REDIS_OK;
  }
  closeConnection(connection);
  return rv;
}

//Executes EXISTS command to a Redis server.
//@returns REDIS_OK when existed.
int exists(void *arg) {
  int sockfd;
  int rv;
  int result;
  int exist;
  char commandStream[MAX_COMMAND_LENGTH];
  Connection *connection;
  ThreadTask *threadTask;

  // Casts arguments.
  threadTask = (ThreadTask *) arg;

  // Connects to redis server.
  connection = openConnection(threadTask->ip);
  if (!connection) {
    printf("redis.c: %s %s\n", "Failed to execute EXISTS.",
        "Failed to get idle connection from pool.");
    return REDIS_FAILED;
  }
  sockfd = connection->sockfd;

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "ss", "EXISTS", threadTask->redisCommand->key);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing EXISTS.", "Command not sent.");
    closeConnection(connection);
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute EXISTS.",
        "Command not sent.");
    closeConnection(connection);
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, &result);
  //printf("[debug]redis.c: rv = %d, result = %d.\n", rv, result);  // debug
  if (rv != REDIS_OK) {
    //printf("[debug]redis.c: exist bucket failed.\n");  // debug
    closeConnection(connection);
    return rv;
  }
  if (result) {
    exist = EXIST_YES;
  } else {
    exist = EXIST_NO;
  }
  
  // Writes back result.
  rv = writeResult(exist, NULL, threadTask);
  if (rv == THREADPOOL_FATAL_ERR) {
    rv = REDIS_FATAL_ERR;
  } else if (rv != THREADPOOL_OK) {
    rv = REDIS_FAILED;
  } else {
    rv = REDIS_OK;
  }
  closeConnection(connection);
  return rv;
}

//Executes HDEL command to a Redis server.
int hDel(void *arg) {
  int sockfd;
  int rv;
  int result;
  char commandStream[MAX_COMMAND_LENGTH];
  Connection *connection;
  ThreadTask *threadTask;

  // Casts arguments.
  threadTask = (ThreadTask *) arg;

  // Connects to redis server.
  connection = openConnection(threadTask->ip);
  if (!connection) {
    printf("redis.c: %s %s\n", "Failed to execute HDEL.",
        "Failed to get idle connection from pool.");
    return REDIS_FAILED;
  }
  sockfd = connection->sockfd;

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "sss", "HDEL", threadTask->redisCommand->key,
      threadTask->redisCommand->field);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HDEL.", "Command not sent.");
    closeConnection(connection);
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute HDEL.", "Command not sent.");
    closeConnection(connection);
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, &result);
  if (rv != REDIS_OK) {
    closeConnection(connection);
    return rv;
  }
  
  // Writes back result.
  rv = writeResult(EXIST_UNDEFINED, NULL, threadTask);
  if (rv == THREADPOOL_FATAL_ERR) {
    rv = REDIS_FATAL_ERR;
  } else if (rv != THREADPOOL_OK) {
    rv = REDIS_FAILED;
  } else {
    rv = REDIS_OK;
  }
  closeConnection(connection);
  return rv;
}

//Executes HEXISTS command to a Redis server.
//@returns REDIS_OK when existed.
int hExists(void *arg) {
  int sockfd;
  int rv;
  int result;
  int exist;
  char commandStream[MAX_COMMAND_LENGTH];
  Connection *connection;
  ThreadTask *threadTask;

  // Casts arguments.
  threadTask = (ThreadTask *) arg;

  // Connects to redis server.
  connection = openConnection(threadTask->ip);
  if (!connection) {
    printf("redis.c: %s %s\n", "Failed to execute HEXISTS.",
        "Failed to get idle connection from pool.");
    return REDIS_FAILED;
  }
  sockfd = connection->sockfd;

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "sss", "HEXISTS",
      threadTask->redisCommand->key, threadTask->redisCommand->field);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HEXISTS.", "Command not sent.");
    closeConnection(connection);
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute HEXISTS.",
        "Command not sent.");
    closeConnection(connection);
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, &result);
  if (rv != REDIS_OK) {
    closeConnection(connection);
    return rv;
  }
  if (result) {
    exist = EXIST_YES;
  } else {
    exist = EXIST_NO;
  }
  
  // Writes back result.
  rv = writeResult(exist, NULL, threadTask);
  if (rv == THREADPOOL_FATAL_ERR) {
    rv = REDIS_FATAL_ERR;
  } else if (rv != THREADPOOL_OK) {
    rv = REDIS_FAILED;
  } else {
    rv = REDIS_OK;
  }
  closeConnection(connection);
  return rv;
}

// Executes HGET command to a Redis server.
int hGet(void *arg) {
  int sockfd;
  int rv;
  char commandStream[MAX_COMMAND_LENGTH];
  unsigned char blob[MAX_BLOB_LENGTH];
  Connection *connection;
  ThreadTask *threadTask;

  // Casts arguments.
  threadTask = (ThreadTask *) arg;

  // Connects to redis server.
  connection = openConnection(threadTask->ip);
  if (!connection) {
    printf("redis.c: %s %s\n", "Failed to execute HGET.",
        "Failed to get idle connection from pool.");
    return REDIS_FAILED;
  }
  sockfd = connection->sockfd;

  // Writes command metadata to Redis server.
  _commandStream(commandStream, "sss", "HGET", threadTask->redisCommand->key,
      threadTask->redisCommand->field);
  //printf("[debug]redis.c: sockfd = %d\n", sockfd);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HGET.", "Command not sent.");
    closeConnection(connection);
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute HGET.", "Command not sent.");
    closeConnection(connection);
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvBulk(sockfd, blob);
  if (rv != REDIS_OK) {
    closeConnection(connection);
    return rv;
  }
  
  // Writes back result.
  rv = writeResult(EXIST_UNDEFINED, blob, threadTask);
  //printf("[debug]redis.c: rv = %d\n", rv);  // debug
  if (rv == THREADPOOL_FATAL_ERR) {
    rv = REDIS_FATAL_ERR;
  } else if (rv != THREADPOOL_OK) {
    rv = REDIS_FAILED;
  } else {
    rv = REDIS_OK;
  }
  closeConnection(connection);
  return rv;
}

// Executes HSET command to a redis server.
int hSet(void *arg) {
  int sockfd;
  int rv;
  int result;
  int valueLength;
  char commandStream[MAX_COMMAND_LENGTH];
  Connection *connection;
  ThreadTask *threadTask;

  // Casts arguments.
  threadTask = (ThreadTask *) arg;

  // Connects to redis server.
  connection = openConnection(threadTask->ip);
  if (!connection) {
    printf("redis.c: %s %s\n", "Failed to execute HSET.",
        "Failed to get idle connection from pool.");
    return REDIS_FAILED;
  }
  sockfd = connection->sockfd;

  // Writes command metadata to Redis server.
  memcpy(&valueLength, threadTask->redisCommand->value, sizeof(int));
  _commandStream(commandStream, "sssb", "HSET", threadTask->redisCommand->key,
      threadTask->redisCommand->field, valueLength);
  rv = write(sockfd, commandStream, strlen(commandStream));
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HSET.", "Command not sent.");
    closeConnection(connection);
    return REDIS_ERR;
  } else if (rv != (int) strlen(commandStream)) {
    printf("redis.c: %s %s\n", "Failed to execute HSET.", "Command not sent.");
    closeConnection(connection);
    return REDIS_FAILED;
  }

  // Writes blob content to Redis server.
  rv = write(sockfd, threadTask->redisCommand->value, valueLength);
  //printf("[debug]redis.c: blobLength = %d\n", blobLength);  // debug
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HSET.", "Blob not sent.");
    closeConnection(connection);
    return REDIS_ERR;
  } else if (rv != valueLength) {
    printf("redis.c: %s %s\n", "Failed to execute HSET.", "Blob not sent.");
    closeConnection(connection);
    return REDIS_FAILED;
  }
  rv = write(sockfd, "\r\n", 2);
  if (rv == -1) {
    printf("redis.c: %s %s\n", "Error executing HSET.", "Blob not sent.");
    closeConnection(connection);
    return REDIS_ERR;
  } else if (rv != 2) {
    printf("redis.c: %s %s\n", "Failed to execute HSET.", "Blob not sent.");
    closeConnection(connection);
    return REDIS_FAILED;
  }

  // Reads Redis reply.
  rv = _recvInt(sockfd, &result);
  if (rv != REDIS_OK) {
    printf("[debug]redis.c: %s %x\n", "Caused by",
        *((unsigned int *) threadTask->ip));
    closeConnection(connection);
    return rv;
  }
  
  // Writes back result.
  rv = writeResult(EXIST_UNDEFINED, NULL, threadTask);
  if (rv == THREADPOOL_FATAL_ERR) {
    rv = REDIS_FATAL_ERR;
  } else if (rv != THREADPOOL_OK) {
    rv = REDIS_FAILED;
  } else {
    rv = REDIS_OK;
  }
  closeConnection(connection);
  return rv;
}

// Finishing function must be called after any operation.
void redisDestroy() {
  connectionPoolDestroy();
}

// Initial function must be called before any operation.
int redisInit() {
  int rv;

  rv = connectionPoolInit();
  if (rv == CONNECTIONPOOL_ERR) {
    printf("redis.c: %s %s\n", "Failed to initialize redis layer.",
        "Error initializing connection pool.");
    return REDIS_FAILED;
  } else if (rv == CONNECTIONPOOL_FAILED) {
    printf("redis.c: %s %s\n", "Failed to initialize redis layer.",
        "Failed to initialize connection pool.");
    return REDIS_FAILED;
  }

  printf("[debug]redis.c: Client connection pool init success.\n");
  return REDIS_OK;
}
