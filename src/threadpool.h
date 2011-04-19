/* Implementation of thread pool.

   To implement "R+W>=N" semantics for data replication, we employ multi-thread
   support. A thread pool is implemented to reduce the overhead of thread
   construction and destruction.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <pthread.h>

#define THREADPOOL_OK 0
#define THREADPOOL_FAILED -1
#define THREADPOOL_ERR 1
#define THREADPOOL_FATAL_ERR 2

#define THREAD_NUM 0xF

#define LOCK_RETRY 10
#define LOCK_RETRY_INTERVAL 10 * 1000 // in microseconds

// A redis command closure.
typedef struct RedisCommand {
  /* input */
  int (*command) (void *);  // command routine
  char *key;
  char *field;
  void *value;
  /* output */
  int successNum;  // Increased by 1 when successful.
  int exist;
  void *blob;  // i-th successful thread writes at blob[i * MAX_BLOB_LENGTH].
  /* miscellaneous */
  pthread_mutex_t *lock;  // write back lock
  int refNum;  // Increased by 1 when attached to a threadpool task structure.
} RedisCommand;

// Thread task.
typedef struct ThreadTask {
  RedisCommand *redisCommand;
  struct in_addr *ip;
  struct ThreadTask *next;
} ThreadTask;

// Thread task queue.
typedef struct ThreadPool {
  struct ThreadTask *taskQueue;
  pthread_mutex_t *lock;
  pthread_cond_t *cond;
  pthread_t *threads;
} ThreadPool;

// Finishing function must be called after usage of the thread pool.
void threadPoolDestroy();
// Initial function must be called before usage of the thread pool.
int threadPoolInit();

// Executes a redis command.
void execute(const int ipNum, const struct in_addr *ip,
    RedisCommand *redisCommand);
// Constructs redis command structure.
RedisCommand * newRedisCommand(int (*command) (void *), const char *key,
    const char *field, const int valueLength, const void *value);
// Reads result.
int readResult(const int successNum, RedisCommand *redisCommand, int *exist,
    void *blob);
// Writes back result.
int writeResult(const int exist, const void *blob, ThreadTask *threadTask);

#endif
