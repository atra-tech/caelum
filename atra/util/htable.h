/*
 *  ATRA ENGINE (2019)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Description:
 *  Hash table module for associative arrays.
 *
 *  */

#ifndef HTABLE_H
#define HTABLE_H

#include "list.h"
#include "util.h"

#define HASH_PRIME_SIZE 32

typedef struct {
    uint size, occupied;
    void **keys;
    void **vals;
    uint (*hash)(const void *);
    uint (*equal)(const void *, const void*);
} HTable;

HTable *htablecreate(uint (*hash)(const void *),
        uint (*eq)(const void *, const void *));
void htableclear(HTable *);
int htableempty(HTable *);
int htableadd(HTable *, void *key, void *val);
void *htableget(HTable *, void *key);
void htableremove(HTable *, void *key);
void htabledeallocelem(HTable *, void *key);
void htabledealloc(HTable *);
void htabledelete(HTable *);

uint htablestringhash(const void *);
uint htablestringequal(const void *, const void *);

#endif
