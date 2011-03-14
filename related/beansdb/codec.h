/*
 *  Beansdb - A high available distributed key-value storage system:
 *
 *      http://beansdb.googlecode.com
 *
 *  Copyright 2010 Douban Inc.  All rights reserved.
 *
 *  Use and distribution licensed under the BSD license.  See
 *  the LICENSE file for full text.
 *
 *  Authors:
 *      Davies Liu <davies.liu@gmail.com>
 *
 */

#ifndef __CODEC_H__
#define __CODEC_H__

void dc_init(const char *path);
int dc_encode(char* buf, const char *src, int len);
int dc_decode(char* buf, const char *src, int len);

#endif

