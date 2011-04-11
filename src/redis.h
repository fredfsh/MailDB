/* Implementation of Redis commands.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef REDIS_H_
#define REDIS_H_

#include <netinet/in.h>

#define REDIS_OK 0
#define REDIS_FAILED -1
#define REDIS_ERR 1
#define STATUS_OK "OK"

#define REDIS_PORT 6379

#define REDIS_RETRY 100
#define REDIS_RETRY_INTERVAL 10 * 1000 // in microseconds

#define MAX_COMMAND_LENGTH 0xFF

// Makes a connection to the destination redis server.
int connectByIp(const in_addr *ip);

// Deletes a key.
int del(const in_addr *ip, const char *key);
// Determines if a key exists.
int exists(const in_addr *ip, const char *key);
// Deletes a hash field.
int hDel(const in_addr *ip, const char *key, const char *field);
// Determines if a hash field exists.
int hExists(const in_addr *ip, const char *key, const char *field);
// Gets the value of a hash field from a single redis server.
int hGet(const in_addr *ip, const char *key, const char *field,
    int *blobLength, void *blob);
// Sets the string value of a hash field to a single redis server.
int hSet(const in_addr *ip, const char *key, const char *field,
    const int blobLength, const void *blob);

#endif
