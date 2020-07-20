/*
 *  ATRA ENGINE (2019)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Description:
 *  Defines types and functions for easily creating and
 *  managing a flexible thread worker pool.
 *
 *  */

#ifndef ATRA_THREAD_H
#define ATRA_THREAD_H

#include "util/util.h"

struct TPool;
typedef struct TPool TPool;

typedef void (*ThreadFunc)(void *arg);

TPool *tpoolcreate(uint count);
void tpooldelete(TPool *pool);

int tpooladdjob(TPool *pool, ThreadFunc func, void *arg);
void tpoolwait(TPool *pool);

#endif /* ATRA_THREAD_H */
