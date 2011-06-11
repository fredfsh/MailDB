/* Refer to the comments at the beginning of "threadpool.h".

   By fredfsh (fredfsh@gmail.com)
*/

#include "def.h"
#include "threadpool.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

ThreadPool *g_threadPool;

// Releases resoures of redis command structure.
void _freeRedisCommand(RedisCommand *redisCommand) {
  pthread_mutex_lock(redisCommand->lock);
  if (--redisCommand->refNum == 0) {
    free(redisCommand->key);
    free(redisCommand->field);
    free(redisCommand->value);
    pthread_mutex_destroy(redisCommand->lock);
    free(redisCommand->lock);
    free(redisCommand->blob);
    free(redisCommand);
    return;
  }
  pthread_mutex_unlock(redisCommand->lock);
}

// Releases resoures of thread task structure.
void _freeThreadTask(ThreadTask *threadTask) {
  _freeRedisCommand(threadTask->redisCommand);
  free(threadTask->ip);
  free(threadTask);
}

// Constructs a thread task structure.
//
// Increases the reference count of the redis command by 1.
ThreadTask * _newThreadTask(const struct in_addr *ip,
    RedisCommand *redisCommand) {
  ThreadTask *result;

  result = (ThreadTask *) malloc(sizeof(ThreadTask));
  memset(result, 0, sizeof(ThreadTask));
  ++redisCommand->refNum;
  result->redisCommand = redisCommand;
  result->ip = (struct in_addr *) malloc(sizeof(struct in_addr));
  memcpy(result->ip, ip, sizeof(struct in_addr));

  return result;
}

// Init routine of a thread.
//
// Threads start busy waiting here. When the stask queue is empty, the thread
// blocks at pthread_cond_wait.
void * _threadDo(void *nouse) {
  ThreadTask *threadTask;

  while (1) {
    /*(

      lock mutex
      while (!condition) wait cond
      condition = 0
      unlock mutex

      lock mutex
      condtion = 1
      unlock mutex
      signal

      */
    pthread_cond_wait(g_threadPool->cond, g_threadPool->lock);
    threadTask = g_threadPool->taskQueue;
    --g_threadPool->waitingTasksNum;
    if ( (g_threadPool->taskQueue = threadTask->next) ) {
      pthread_cond_signal(g_threadPool->cond);
    }
    --g_threadPool->idleThreadsNum;
    pthread_mutex_unlock(g_threadPool->lock);

    if (threadTask) threadTask->redisCommand->command(threadTask);
    pthread_mutex_lock(g_threadPool->lock);
    ++g_threadPool->idleThreadsNum;
    pthread_mutex_unlock(g_threadPool->lock);
  }

  return NULL;  // unreachable
}

// Executes a redis command.
//
// This function constructs thread tasks for worker threads to execute. After
// enqueuing these tasks, the function returns immediately.
void execute(const int ipNum, const struct in_addr *ip,
    RedisCommand *redisCommand) {
  int i;
  int retry;
  ThreadTask *threadTasks[N], *p;

  // We do as much as we can before operate the task queue, to reduce the
  // overhead of acquiring and releasing the lock.
  threadTasks[0] = _newThreadTask(&ip[0], redisCommand);
  for (i = 1; i < ipNum; ++i) {
    threadTasks[i] = _newThreadTask(&ip[i], redisCommand);
    threadTasks[i - 1]->next = threadTasks[i];
  }

  // Adds tasks to the task queue.
  retry = ADD_RETRY;
  while (--retry >= 0) {
    pthread_mutex_lock(g_threadPool->lock);
    if (g_threadPool->waitingTasksNum >= MAX_WAITING_TASKS_NUM) {
      pthread_mutex_unlock(g_threadPool->lock);
      usleep(ADD_RETRY_INTERVAL);
      continue;
    }
    p = g_threadPool->taskQueue;
    if (p) {
      while (p->next) p = p->next;
      p->next = threadTasks[0];
    } else {
      g_threadPool->taskQueue = threadTasks[0];
    }
    g_threadPool->waitingTasksNum += ipNum;
    pthread_mutex_unlock(g_threadPool->lock);
    pthread_cond_signal(g_threadPool->cond);
    return;
  }

  printf("threadpool.c: %s %s\n", "Failed to add tasks to thread pool.",
      "Too many waiting tasks.");
}

// Constructs redis command structure.
RedisCommand * newRedisCommand(int (*command) (void *),
    const char *key, const char *field, const int valueLength,
    const void *value) {
  int length;
  RedisCommand *result;

  result = (RedisCommand *) malloc(sizeof(RedisCommand));
  memset(result, 0, sizeof(RedisCommand));
  result->command = command;
  if (key) {
    result->key = (char *) malloc(MAX_COMMAND_LENGTH);
    strcpy(result->key, key);
  }
  if (field) {
    result->field = (char *) malloc(MAX_COMMAND_LENGTH);
    strcpy(result->field, field);
  }
  if (valueLength && value) {
    result->value = malloc(MAX_BLOB_LENGTH);
    length = valueLength + sizeof(int);
    memcpy(result->value, &length, sizeof(int));
    memcpy(&((int *) result->value)[1], value, valueLength);
  }
  result->lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(result->lock, NULL);
  result->blob = malloc(N * MAX_BLOB_LENGTH);
  // @refNum is initialized as 1 so the structure won't be destroyed after all
  // of the worker threads finishes but before we check the result.
  result->refNum = 1;

  return result;
}

// Reads result.
// @successNum: Threshold of number of success threads.
// @exist: NULL if no existence status should be read.
// @blob: NULL if no blob content should be read.
// @returns THREADPOOL_OK when enough worker threads succeed.
//
// @refNum of the @threadTask->redisCommand is decreased by 1.
int readResult(const int successNum, RedisCommand *redisCommand, int *exist,
    void *blob) {
  int rv;
  int retry;
  int chosen;
  void *chosenBlob;

  retry = LOCK_RETRY;
  while (--retry >= 0) {
    rv = pthread_mutex_trylock(redisCommand->lock);
    if (!rv) {
      if (redisCommand->successNum >= successNum) {
        if (exist) *exist = redisCommand->exist;
        if (blob) {
          chosen = rand() % redisCommand->successNum;
          chosenBlob
              = &((char *) redisCommand->blob)[chosen * MAX_BLOB_LENGTH];
          memcpy(blob, chosenBlob, MAX_BLOB_LENGTH);
        }
        pthread_mutex_unlock(redisCommand->lock);
        _freeRedisCommand(redisCommand);
        return THREADPOOL_OK;
      }
      pthread_mutex_unlock(redisCommand->lock);
      if (retry == 1) {
        printf("threadpool.c: %s %s\n", "Failed to read result.",
            "Not enough replies gathered from redis.");
        printf("[debug]threadpool.c: %s %d\n",
            "Number of tasks waiting in task queue:",
            g_threadPool->waitingTasksNum);
        printf("[debug]threadpool.c: %s %d\n", "Number of idle threads:",
            g_threadPool->idleThreadsNum);
        return THREADPOOL_FAILED;
      }
    }
    usleep(LOCK_RETRY_INTERVAL);
  }
  printf("threadpool.c: %s %s\n", "Failed to read result.",
      "Unable to acquire the mutex.");
  printf("[debug]threadpool.c: successNum = %d, refNum = %d\n",
      redisCommand->successNum, redisCommand->refNum);
  return THREADPOOL_FAILED;
}
// Initial function must be called before usage of the thread pool.
int threadPoolInit() {
  int i;
  int rv;

  // Seeds random number generator.
  srand((unsigned int) time(NULL));

  // Memory allocation.
  g_threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
  memset(g_threadPool, 0, sizeof(ThreadPool));
  g_threadPool->lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(g_threadPool->lock, NULL);
  g_threadPool->cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(g_threadPool->cond, NULL);
  g_threadPool->threads =
    (pthread_t *) malloc(THREAD_NUM * sizeof(pthread_t));
  for (i = 0; i < THREAD_NUM; ++i) {
    rv = pthread_create(&g_threadPool->threads[i], NULL, _threadDo, NULL);
    if (rv) {
      printf("threadpool.c: %s %s\n", "Error initalizing thread pool.",
          "Unable to create enough threads.");
      return THREADPOOL_ERR;
    }
  }
  g_threadPool->idleThreadsNum = THREAD_NUM;
  return THREADPOOL_OK;
}

// Finishing function must be called after usage of the thread pool.
void threadPoolDestroy() {
  int i;
  ThreadTask *p;

  for (i = 0; i < THREAD_NUM; ++i) pthread_cancel(g_threadPool->threads[i]);
  while (g_threadPool->taskQueue) {
    p = g_threadPool->taskQueue;
    g_threadPool->taskQueue = p->next;
    _freeThreadTask(p);
  }
  pthread_mutex_destroy(g_threadPool->lock);
  free(g_threadPool->lock);
  pthread_cond_destroy(g_threadPool->cond);
  free(g_threadPool->cond);
  free(g_threadPool);
}

// Writes back result.
// @blob: NULL if no blob content should be read.
int writeResult(const int exist, const void *blob, ThreadTask *threadTask) {
  int rv;
  int retry;
  int blobLength;
  int successNum;
  RedisCommand *redisCommand;

  redisCommand = threadTask->redisCommand;
  retry = LOCK_RETRY;
  while (--retry >= 0) {
    rv = pthread_mutex_trylock(redisCommand->lock);
    if (!rv) {
      successNum = redisCommand->successNum;
      redisCommand->successNum = successNum + 1;
      redisCommand->exist |= exist;
      if (blob) {
        memcpy(&blobLength, blob, sizeof(int));
        memcpy(&((char *) redisCommand->blob)[successNum * MAX_BLOB_LENGTH],
            blob, blobLength);
      }
      rv = pthread_mutex_unlock(redisCommand->lock);
      if (rv) {
        printf("threadpool.c: %s %s\n", "Error writing back result.",
            "Unable to release the mutex.");
        return THREADPOOL_FATAL_ERR;
      }
      _freeRedisCommand(redisCommand);
      return THREADPOOL_OK;
    }
    usleep(LOCK_RETRY_INTERVAL);
  }
  printf("threadpool.c: %s %s\n", "Failed to write back result.",
      "Unable to acquire the mutex.");
  return THREADPOOL_FAILED;
}
