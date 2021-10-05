/*############ Compilation And execution
gcc server.c -o serv.out -ldl -lpthread
./serv.out sockfile 15 20 10
//############*/
#define _GNU_SOURCE
#include "modules/json_module/json.h" //Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)
#include "modules/json_module/json.c" //Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)

#include <unistd.h>

#include "modules/dll_invoker.c"
#include "modules/queue.c"
#include "modules/thread_pool.c"

// #include <signal.h>

void log_msg(const char *msg, bool terminate)
{
    printf("%s\n", msg);
    if (terminate)
        exit(-1); /* failure */
}

unsigned long conv = 1048576; // MB to B
unsigned long mem_lim;        // Memory limit in Bytes
unsigned long fd_limit;       // File descriptor limit
int thread_cnt;               // Thread Count

#include "server_implementations.c"

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        printf("Usage: %s [Local socket file path] [Active Thread Count] [File Descriptor Limit] [Memory Limit MB]\n",
               argv[0]);
        exit(-1);
    }

    pthread_mutex_lock(&wait_kill);
    pthread_t thread_killer;
    pthread_create(&thread_killer, NULL, threadKiller, NULL);
    char *end;
    thread_cnt = strtod(argv[2], &end);
    bool valid = true;
    if (end == argv[2])
        valid = false;
    if (valid)
    {
        fd_limit = strtoul(argv[3], &end, 10);
        if (end == argv[3] || fd_limit < 7)
            valid = false;
    }
    if (valid)
    {
        mem_lim = strtoul(argv[4], &end, 10);
        if (end == argv[4])
            valid = false;
    }
    if (!valid)
    {
        log_msg("Error Invalid Parameters.", false);
        printf("Usage: %s [Local socket file path] [Active Thread Count] [File Descriptor Limit] [Memory Limit MB]\n", argv[0]);

        printf("NOTE THAT ATLEAST 7 FILE DESCRIPTORS are needed always. (For: STDIN, STDOUT, STDERR, SOCKETFILE, SOCKET CONN, DLOPEN LIBRARY). \nNote that since threads are always active, the total memory size should be greater than the minimum stack size of each thread * thread count. \nCheck minimum stack size here: \nhttps://man7.org/linux/man-pages/man3/pthread_create.3.html#:~:text=per-architecture,BUGS .\n");
        exit(-1);
    }

    struct rlimit *mem_lim_st = malloc(sizeof(struct rlimit)), *fptr_lim = malloc(sizeof(struct rlimit));
    mem_lim_st->rlim_cur = mem_lim_st->rlim_max = mem_lim;
    fptr_lim->rlim_cur = fptr_lim->rlim_max = fd_limit;

    setrlimit(RLIMIT_STACK, mem_lim_st);
    setrlimit(RLIMIT_NOFILE, fptr_lim);

    disp_queue = newNumQueue(MAXQ_SIZE);
    disp_pool = newThreadPool(thread_cnt);

    if (beginRoutine(disp_pool, dispatcher_thread_func, NULL))
        log_msg("Error !! Unable to allocate threads.\n Try decreasing thread count.\n", true);
    // printf("Threads: %d", thread_cnt);
    start_server_socket(argv[1], 100);
    destroyPool(disp_pool);
}