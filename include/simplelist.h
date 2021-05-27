#ifndef SIMPLELIST_H
#define SIMPLELIST_H

typedef struct lnode{
    void *content;
    struct lnode *next;
} templListNode;

typedef struct lnode{
    // Evaluation function to compare an element of the list
    // to an appropriate key or a different element.
    // Passed as an argument during the creation of the list.
    int (*eval_funct)(void *, void *);
    templListNode *front;
    templListNode *rear;
} templList;

#endif