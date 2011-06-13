/* Implementation of connection pool.

   A fixed number of connections to each Redis server are established in the
   init phase, to reduce the cost of resource allocation and release.
   
   The ConnectionPool consists of several ConnectionQueues, each corresponding
   to a Redis server. Elements in a connection queue are long-live socket
   connections to that Redis server, and they share the same mutex lock on the
   queue. Any caller who wants to employ a connection must acquire the lock of
   that queue first. Idle connections are always forwarded to the head of the
   queue for fast lookup.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef CONNECTIONPOOL_H_
#define CONNECTIONPOOL_H_

#include "threadpool.h"

#define CONNECTIONPOOL_OK 0
#define CONNECTIONPOOL_FAILED -1
#define CONNECTIONPOOL_ERR 1
#define CONNECTIONPOOL_FATAL_ERR 2

#define CONNECTION_STATE_DISCONNECTED   0       // just init
#define CONNECTION_STATE_IDLE           1       // connected but idle
#define CONNECTION_STATE_TRANSFERING    2       // transfering data

#define CONNECTION_NUM THREAD_NUM

#define REDIS_PORT 6379

// A redis command closure.
typedef struct Connection {
  struct ConnectionQueue *connectionQueue;  // reverse ref
  int state;
  int sockfd;
  struct Connection *previous;
  struct Connection *next;
} Connection;

// connection queue
typedef struct ConnectionQueue {
  struct Connection *header;  // double linked list with header element
  struct Connection *tail;
  struct in_addr *ip;  // IP address of destination Redis server
  pthread_mutex_t *lock;
  struct ConnectionQueue *next;
} ConnectionQueue;

// connection pool
typedef struct ConnectionPool {
  struct ConnectionQueue *header;  // linked list with header element
} ConnectionPool;

// Finishing function must be called after usage of the connection pool.
void connectionPoolDestroy();
// Initial function must be called before usage of the connection pool.
int connectionPoolInit();

// Puts back the connection into idle after usage.
void closeConnection(Connection *connection);
// Retrieves an idle connection in the pool.
Connection * openConnection(const struct in_addr *ip);
#endif
