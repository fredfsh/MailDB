/* Implementation of Redis commands.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef REDIS_H_
#define REDIS_H_

#define REDIS_OK 0
#define REDIS_FAILED -1
#define REDIS_ERR 1

#define REDIS_PORT 6379

#define REDIS_RETRY 100
#define REDIS_RETRY_INTERVAL 10 * 1000 // in microseconds

#define MAX_COMMAND_LENGTH 0xFF

// Deletes a key.
int del(const int sockfd, const char *key);
// Determines if a key exists.
int exists(const int sockfd, const char *key, int *result);
// Deletes a hash field.
int hDel(const int sockfd, const char *key, const char *field);
// Determines if a hash field exists.
int hExists(const int sockfd, const char *key, const char *field, int *result);
// Gets the value of a hash field.
int hGet(const int sockfd, const char *key, const char *field,
    int *blobLength, void *blob);
// Sets the string value of a hash field.
int hSet(const int sockfd, const char *key, const char *field,
    const int blobLength, const void *blob);
// Pings the server.
int ping(const int sockfd, int *result);

#endif
