#include <pthread.h>
#include <stdlib.h>

typedef struct threadpool threadpool;

struct threadpool
{
    pthread_mutex_t *mutex;   // Mutex to ensure queue is worked by threads one at a time
    pthread_cond_t *cond_var; // Conditional var to serve as signal to awake a thread
    pthread_t *threads;       // Threads which are reused for execution of incoming tasks
    int size;
};

threadpool *newThreadPool(int pool_size)
{
    threadpool *ptr_pool = (threadpool *)malloc(sizeof(threadpool));
    ptr_pool->threads = (pthread_t *)malloc(pool_size * sizeof(pthread_t));
    ptr_pool->size = pool_size;

    ptr_pool->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    ptr_pool->cond_var = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

    if (ptr_pool->mutex == NULL || ptr_pool->cond_var == NULL)
    {
        printf("ERRR\n");
        exit(-1);
    }

    int ret;
    if (pthread_mutex_init(ptr_pool->mutex, NULL) || pthread_cond_init(ptr_pool->cond_var, NULL))
    {
        printf("Variable Alloc Errro");
        exit(-1);
    }

    // for (int i = 0; i < maxsize; i++)
    // {
    // printf("{%d}\n", i);
    // printf("%ld", (ptr_pool->threads)[i]);
    // ret = pthread_create(ptr_pool->threads + i, NULL, thread_func, NULL);
    // if (ret)
    // {
    //     printf("%d: Error Code: %d\n", i, ret);
    //     return NULL;
    // }
    // }

    return ptr_pool;
}

void beginRoutine(threadpool *ptr_pool, void *(*thread_func)(void *), void *args)
{
    int ret;
    for (int i = 0; i < ptr_pool->size; i++)
    {
        ret = pthread_create(ptr_pool->threads + i, NULL, thread_func, NULL);
        if (ret)
        {
            printf("%d: Error Code: %d\n", i, ret);
            // return NULL;
        }
    }
}
void destroyPool(threadpool *ptr_pool)
{
    for (int i = 0; i < ptr_pool->size; i++)
        pthread_cancel((ptr_pool->threads)[i]);

    free(ptr_pool->threads);
    pthread_mutex_destroy(&ptr_pool->mutex);
    pthread_cond_destroy(&ptr_pool->cond_var);
    free(ptr_pool);
}