/* Header file of global definitions.

   By fredfsh (fredfsh@gmail.com)
*/
#ifndef DEF_H_
#define DEF_H_

typedef unsigned int u_int;
typedef unsigned char u_char;
typedef long util_long;

#define CFGSRV_HOST "10.0.1.201"

#define EXIST_BLOB_ID "#ExistBlobId#"
#define EXIST_BLOB "#ExistBlob#"

#define MAX_COMMAND_LENGTH 0xFF
#define MAX_KEY_LENGTH 0x3F
#define MAX_BLOB_LENGTH 0xFFFF

#define C 0x5  // max hosts per key
#define N 0x3
#define R 0x2
#define W 0x2

#endif
