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

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#include "thread.h"

typedef struct TPoolJob TPoolJob;

struct TPoolJob {
    ThreadFunc func;
    void *arg;
    TPoolJob *next;
};

struct TPool {
    TPoolJob *first;
    TPoolJob *last;
    SDL_mutex *mutex;
    SDL_cond *jobcond;
    SDL_cond *workingcond;
    size_t workingcnt;
    size_t threadcnt;
    uint stop;
};

static TPoolJob *tpooljobcreate(ThreadFunc func, void *arg);
static void tpooljobdelete(TPoolJob *job);
static TPoolJob *tpooljobget(TPool *pool);
static int tpoolworker(void *arg);

TPool *
tpoolcreate(uint count)
{
    TPool *pool;
    SDL_Thread *thread;
    char name[18] = {'\0'};
    size_t i;

    if (count == 0)
        count = 2;

    pool = calloc(1, sizeof(TPool));
    pool->threadcnt = count;

    pool->mutex = SDL_CreateMutex();
    pool->jobcond = SDL_CreateCond();
    pool->workingcond = SDL_CreateCond();

    for (i = 0; i < count; i++) {
        sprintf(name, "caelum_worker_%i", (int)i);
        thread = SDL_CreateThread(tpoolworker, name, pool);
    }

    return pool;
}

void
tpooldelete(TPool *pool)
{
    TPoolJob *job;
    TPoolJob *nextjob;

    SDL_LockMutex(pool->mutex);
    job = pool->first;

    while (job) {
        nextjob = job->next;
        tpooljobdelete(job);
        job = nextjob;
    }
    pool->stop = 1;
    SDL_CondBroadcast(pool->jobcond);
    SDL_UnlockMutex(pool->mutex);

    tpoolwait(pool);

    SDL_DestroyMutex(pool->mutex);
    SDL_DestroyCond(pool->jobcond);
    SDL_DestroyCond(pool->workingcond);

    free(pool);
}

int
tpooladdjob(TPool *pool, ThreadFunc func, void *arg)
{
    TPoolJob *job;

    job = tpooljobcreate(func, arg);
    SDL_LockMutex(pool->mutex);

    if (!pool->first) {
        pool->first = job;
        pool->last = pool->first;
    } else {
        pool->last->next = job;
        pool->last = job;
    }
    SDL_CondBroadcast(pool->jobcond);
    SDL_UnlockMutex(pool->mutex);

    return 1;
}

void
tpoolwait(TPool *pool)
{
    SDL_LockMutex(pool->mutex);

    while (1) {
        if ((!pool->stop && pool->workingcnt != 0) ||
                (pool->stop && pool->threadcnt != 0)) {
            SDL_CondWait(pool->workingcond, pool->mutex);
        } else {
            break;
        }
    }
    SDL_UnlockMutex(pool->mutex);
}

TPoolJob *
tpooljobcreate(ThreadFunc func, void *arg)
{
    TPoolJob *job;

    job = malloc(sizeof(TPoolJob));
    job->func = func;
    job->arg = arg;
    job->next = NULL;

    return job;
}

void
tpooljobdelete(TPoolJob *job)
{
    free(job);
}

TPoolJob *
tpooljobget(TPool *pool)
{
    TPoolJob *job;

    job = pool->first;

    if (!job) {
        return NULL;
    }
    if (!job->next) {
        pool->first = NULL;
        pool->last = NULL;
    } else {
        pool->first = job->next;
    }

    return job;
}

int
tpoolworker(void *arg)
{
    TPool *pool = arg;
    TPoolJob *job;

    while (1) {
        SDL_LockMutex(pool->mutex);
        
        while (!pool->first && !pool->stop) {
            SDL_CondWait(pool->jobcond, pool->mutex);
        }

        if (pool->stop) {
            break;
        }
        job = tpooljobget(pool);
        pool->workingcnt++;
        SDL_UnlockMutex(pool->mutex);

        if (job) {
            job->func(job->arg);
            tpooljobdelete(job);
        }
        SDL_LockMutex(pool->mutex);
        pool->workingcnt--;
        
        if (!pool->stop && pool->workingcnt == 0 && !pool->first) {
            SDL_CondSignal(pool->workingcond);
        }
        SDL_UnlockMutex(pool->mutex);
    }
    pool->threadcnt--;
    SDL_CondSignal(pool->workingcond);
    SDL_UnlockMutex(pool->mutex);

    return 0;
}
