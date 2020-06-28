/*
 *  ATRA ENGINE (2019)
 *
 *  Copyright Robert Saarloos, All rights reserved.
 *
 *  Description:
 *  A dynamically growing/shrinking, general-purpose list type.
 *
 *  */

#ifndef ATRA_LIST_H
#define ATRA_LIST_H

#define INIT_LIST_CAP 8

#include "util.h"

typedef struct {
    void **elem;
    uint size;
    uint cap;
} List;

List *listcreate();
void *listat(List *list, uint index);
void listpushback(List *list, void *elem);
void listinsert(List *list, void *elem, uint index);
void liatpopback(List *list);
void *listback(List *list);
void listclear(List *list);
void listremove(List *list, uint index);
void listremoverange(List *list, uint start, uint end);
void listfreemove(List *list, uint index);
void listfreemoverange(List *list, uint start, uint end);
void listmoveelem(List *list, uint index, uint dest);
void listdealloc(List *list);
void listdelete(List *list);

#endif /* ATRA_LIST_H */
