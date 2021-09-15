#include <stdio.h>
#include <string.h>
#include "request.c"

typedef struct queue queue;
typedef struct node node;

typedef struct num_queue num_queue;
typedef struct num_node num_node;

struct node
{
    struct node *next;
    struct request *data;
};

struct node *newNode(struct request *req)
{
    struct node *Nnode = (struct node *)malloc(sizeof(struct node));
    Nnode->next = NULL;
    Nnode->data = req;

    return Nnode;
}

struct queue
{
    int maxS;
    int size;
    struct node *head, *tail;
};

struct queue *newQueue(int n)
{
    struct queue *ptr_q = malloc(sizeof(struct queue));

    ptr_q->maxS = n;
    ptr_q->size = 0;
    ptr_q->head = ptr_q->tail = NULL;

    return ptr_q;
}

int qpush(struct request *nreq, struct queue *ptrq)
{

    if (ptrq->size >= ptrq->maxS)
        return 1;

    struct node *nNode = newNode(nreq);

    if (ptrq->tail)
        ptrq->tail->next = nNode;
    else
        ptrq->head = nNode;

    ptrq->tail = nNode;
    ptrq->size++;

    return 0;
}

struct request *qtop(struct queue *ptrq)
{
    if (ptrq->size > 0)
        return ptrq->head->data;
    return NULL;
}

struct request *qpop(struct queue *ptrq)
{
    if (ptrq->size > 0)
    {
        struct node *phead = ptrq->head;

        ptrq->head = ptrq->head->next;
        phead->next = NULL;

        ptrq->size--;

        if (!ptrq->size)
            ptrq->tail = NULL;

        struct request *req = phead->data;

        free(phead);
        return req;
    }

    return NULL;
}

// ################################# Queue for Number ###############################

struct num_node
{
    num_node *next;
    int data;
};

num_node *newNumNode(int val)
{
    num_node *Nnode = (num_node *)malloc(sizeof(num_node));
    Nnode->next = NULL;
    Nnode->data = val;

    return Nnode;
}

struct num_queue
{
    int maxS;
    int size;
    num_node *head, *tail;
};

num_queue *newNumQueue(int n)
{
    num_queue *ptr_q = malloc(sizeof(num_queue));

    ptr_q->head = ptr_q->tail = NULL;
    ptr_q->maxS = n;
    ptr_q->size = 0;

    return ptr_q;
}

int num_qpush(int data, num_queue *ptrq)
{
    if (!ptrq)
    {
        printf("Invalid Queue\n");
        exit(-1);
    }
    if (ptrq->size >= ptrq->maxS)
        return -1;

    num_node *nNode = newNumNode(data);

    if (ptrq->size)
        ptrq->tail->next = nNode;
    else
        ptrq->head = nNode;

    ptrq->tail = nNode;
    ptrq->size++;

    return 0;
}

int num_qtop(num_queue *ptrq)
{
    if (!ptrq)
    {
        printf("Invalid Queue\n");
        exit(-1);
    }
    if (ptrq->size > 0)
        return ptrq->head->data;
    return -1;
}

int num_qpop(struct num_queue *ptrq)
{
    if (!ptrq)
    {
        printf("Invalid Queue\n");
        exit(-1);
    }
    if (ptrq->size > 0)
    {
        num_node *phead = ptrq->head;

        ptrq->head = ptrq->head->next;
        ptrq->size--;

        phead->next = NULL;
        if (!ptrq->size)
            ptrq->tail = NULL;

        int res = phead->data;

        // free(phead);
        return res;
    }

    return -1;
}

/*
struct cnode
{
    char **c;
};

struct cqueue
{
    int maxS;
    int size;
    struct cnode *head, *tail;
};

cqueue *newcQueue(int n)
{
    cqueue *ptr_q = malloc(sizeof(cqueue));

    ptr_q->maxS = n;
    ptr_q->size = 0;

    ptr_q->head = ptr_q->tail = NULL;

    return ptr_q;
}

int qpush(struct request *nreq, struct queue *ptrq)
{

    if (ptrq->size >= ptrq->maxS)
        return 1;

    struct node *nNode = newNode(nreq);

    if (ptrq->tail)
        ptrq->tail->next = nNode;
    else
        ptrq->head = nNode;

    ptrq->tail = nNode;
    ptrq->size++;

    return 0;
}

struct request *qtop(struct queue *ptrq)
{
    if (ptrq->size > 0)
        return ptrq->head->data;

    return NULL;
}

struct request *qpop(struct queue *ptrq)
{
    if (ptrq->size > 0)
    {
        struct node *phead = ptrq->head;

        ptrq->head = ptrq->head->next;
        phead->next = NULL;

        ptrq->size--;

        if (!ptrq->size)
            ptrq->tail = NULL;

        return phead->data;
    }

    return NULL;
}
*/