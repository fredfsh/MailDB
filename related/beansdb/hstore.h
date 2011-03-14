/*
 *  Beansdb - A high available distributed key-value storage system:
 *
 *      http://beansdb.googlecode.com
 *
 *  Copyright 2009 Douban Inc.  All rights reserved.
 *
 *  Use and distribution licensed under the BSD license.  See
 *  the LICENSE file for full text.
 *
 *  Authors:
 *      Davies Liu <davies.liu@gmail.com>
 */

#ifndef __HSTORE_H__
#define __HSTORE_H__

typedef struct t_hstore HStore;

HStore* hs_open(const char *path, int height, time_t before);
void    hs_flush(HStore *store, int limit);
void    hs_close(HStore *store);
char*   hs_get(HStore *store, char *key, int *vlen, uint32_t *flag);
bool    hs_set(HStore *store, char *key, char* value, int vlen, uint32_t flag, int version);
bool    hs_append(HStore *store, char *key, char* value, int vlen);
int64_t hs_incr(HStore *store, char *key, int64_t value); 
bool    hs_delete(HStore *store, char *key);
uint64_t hs_count(HStore *store, uint64_t *curr);
bool    hs_optimize(HStore *store, int limit);
#endif
