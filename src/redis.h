/* Implementation of Redis commands.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef REDIS_H_
#define REDIS_H_

#include "threadpool.h"
#include <netinet/in.h>
#include <pthread.h>

#define REDIS_OK 0
#define REDIS_FAILED -1
#define REDIS_ERR 1
#define REDIS_FATAL_ERR 2
#define STATUS_OK "OK"
#define EXIST_UNDEFINED 0
#define EXIST_NO 0
#define EXIST_YES 1

#define REDIS_RETRY 1000
#define REDIS_RETRY_INTERVAL 1 * 1000 // in microseconds

// Finishing function must be called after any operation.
void redisDestroy();
// Initial function must be called before any operation.
int redisInit();

// Deletes a key.
int del(void *arg);
// Determines if a key exists.
int exists(void *arg);
// Deletes a hash field.
int hDel(void *arg);
// Determines if a hash field exists.
int hExists(void *arg);
// Gets the value of a hash field from a single redis server.
int hGet(void *arg);
// Sets the string value of a hash field to a single redis server.
int hSet(void *arg);

#endif
