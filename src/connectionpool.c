/* Refer to the comments at the beginning of "connectionpool.h".

   By fredfsh (fredfsh@gmail.com)
*/

#include "def.h"
#include "config-server.h"
#include "connectionpool.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

ConnectionPool *g_connectionPool;

// Releases resoures of ConnectionQueue structure.
void __freeConnectionQueue(ConnectionQueue *connectionQueue) {
  Connection *p;

  // Releases resoures of Connection structures in this queue.
  p = connectionQueue->header->next;
  while (p) {
    connectionQueue->header->next = p->next;
    if (p->state != CONNECTION_STATE_DISCONNECTED) close(p->sockfd);
    free(p);
    p = connectionQueue->header->next;
  }
  free(connectionQueue->header);

  free(connectionQueue->ip);
  pthread_mutex_destroy(connectionQueue->lock);
  free(connectionQueue->lock);
  free(connectionQueue);
}

// Constructs a Connection structure.
// @returns the structure when succeeded, or NULL when failed.
//
// Makes a connection to the target Redis server.
Connection * __newConnection(const struct in_addr ip,
    ConnectionQueue *connectionQueue) {
  int sockfd;
  int rv;
  struct sockaddr_in serv_addr;
  Connection *result;

  do {
    result = NULL;

    // Creates a socket.
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) break;

    // Connects to the Redis server.
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(REDIS_PORT);
    memcpy(&serv_addr.sin_addr, &ip, sizeof(struct in_addr));
    rv = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (rv < 0) break;

    // Allocates memory and builds structure.
    result = (Connection *) calloc(1, sizeof(Connection));
    if (!result) break;
    result->connectionQueue = connectionQueue;
    result->state = CONNECTION_STATE_IDLE;
    result->sockfd = sockfd;

    return result;

  } while(0);

  // error handler
  if (result) free(result);
  return NULL;
}

// Constructs a ConnectionQueue structure.
//
// Makes fixed number of connections to the target Redis server.
ConnectionQueue * __newConnectionQueue(const struct in_addr ip) {
  int i;
  ConnectionQueue *result;
  Connection *p, *q;

  result = (ConnectionQueue *) calloc(1, sizeof(ConnectionQueue));
  if (!result) return NULL;

  // connections
  result->header = (Connection *) malloc(sizeof(Connection));
  p = result->header;
  for (i = 0; i < CONNECTION_NUM; ++i) {
    q = __newConnection(ip, result);
    if (!q) {
      __freeConnectionQueue(result);
      return NULL;
    }
    p->next = q;
    q->previous = p;
    p = q;
  }
  result->tail = p;

  result->ip = (struct in_addr *) malloc(sizeof(struct in_addr));
  memcpy(result->ip, &ip, sizeof(struct in_addr));
  result->lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(result->lock, NULL);

  return result;
}

// Puts back the connection into idle after usage.
//
// Forwards it to the head for fast lookup.
void closeConnection(Connection *connection) {
  int rv;
  char reply[MAX_BLOB_LENGTH];
  Connection *p;
  ConnectionQueue *q;

  q = connection->connectionQueue;
  pthread_mutex_lock(q->lock);
  // Clears buffer.
  do {
    rv = recv(connection->sockfd, reply, MAX_BLOB_LENGTH, MSG_DONTWAIT);
  } while (rv > 0);
  connection->state = CONNECTION_STATE_IDLE;
  p = q->header;
  connection->previous->next = connection->next;
  if (connection->next) connection->next->previous = connection->previous;
  if (p->next) p->next->previous = connection;
  connection->next = p->next;
  p->next = connection;
  connection->previous = p;
  pthread_mutex_unlock(q->lock);
}

// Finishing function must be called after usage of the connection pool.
void connectionPoolDestroy() {
  ConnectionQueue *p;

  p = g_connectionPool->header->next;
  while (p) {
    g_connectionPool->header->next = p->next;
    free(p);
    p = g_connectionPool->header->next;
  }
  free(g_connectionPool->header);
  free(g_connectionPool);
}

// Initial function must be called before usage of the connection pool.
int connectionPoolInit() {
  int rv;
  FILE *fin;
  char line[MAX_LINE_LENGTH];
  struct addrinfo hints, *result;
  ConnectionQueue *p;

  // memory allocation
  g_connectionPool = (ConnectionPool *) calloc(1, sizeof(ConnectionPool));
  g_connectionPool->header =
      (ConnectionQueue *) calloc(1, sizeof(ConnectionQueue));

  // connection establishment
  p = g_connectionPool->header;
  fin = fopen(CONFIG_FILE, "r");
  if (!fin) {
    printf("connectionpool.c: %s %s\n", "Error initializing connection pool.",
        "Config file for redis servers not found.");
    return CONNECTIONPOOL_ERR;
  }
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  rv = fscanf(fin, "%s", line);
  while (!feof(fin)) {
    rv = getaddrinfo(line, NULL, &hints, &result);
    if (rv || !result) {
      freeaddrinfo(result);
      fclose(fin);
      connectionPoolDestroy();
      return CONNECTIONPOOL_FAILED;
    }
    p->next = __newConnectionQueue(
        ((struct sockaddr_in *) result->ai_addr)->sin_addr);
    if (!p->next) {
      freeaddrinfo(result);
      fclose(fin);
      connectionPoolDestroy();
      return CONNECTIONPOOL_FAILED;
    }
    freeaddrinfo(result);
    p = p->next;
    rv = fscanf(fin, "%s", line);
  }
  return CONNECTIONPOOL_OK;
}

// Retrieves an idle connection in the pool.
//
// And forwards it to the tail of the queue.
Connection * openConnection(const struct in_addr *ip) {
  ConnectionQueue *p;
  Connection *q;
  int rv;
  char reply[MAX_BLOB_LENGTH];

  p = g_connectionPool->header->next;
  while (p) {
    // Locates the connection queue.
    if (memcmp(p->ip, ip, sizeof(struct in_addr))) {
      p = p->next;
      continue;
    }

    // Finds an idle connection.
    pthread_mutex_lock(p->lock);
    q = p->header->next;
    if (q && (q->state == CONNECTION_STATE_IDLE)) {
      q->state = CONNECTION_STATE_TRANSFERING;
      // Clears buffer.
      do {
        rv = recv(q->sockfd, reply, MAX_BLOB_LENGTH, MSG_DONTWAIT);
      } while (rv > 0);
      if (q->next) q->next->previous = p->header;
      p->header->next = q->next;
      p->tail->next = q;
      q->previous = p->tail;
      q->next = NULL;
    }
    pthread_mutex_unlock(p->lock);
    return q;
  }

  return NULL;
}
