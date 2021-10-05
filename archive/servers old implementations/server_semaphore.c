/*############ Compilation And execution
gcc server.c -o serv.out -ldl -lpthread
./serv.out sockfile
//############*/
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/un.h>
#include <stddef.h>
#include <limits.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>

#include "modules/dll_invoker.c"
#include "modules/queue.c"
#include "modules/thread_pool.c"

#define MAXQ_SIZE 100
int keep_alive = 1;
//##################  Dispatcher (which also acts as Socket Reciever) Thread Control ##################//

threadpool *disp_pool; // Thread pool for the dispatcher
num_queue *disp_queue; // Queue for storing incoming requests

sem_t cnt_thread, bool_isended;
//#####################################################################################################//

pthread_mutex_t wait_kill = PTHREAD_MUTEX_INITIALIZER; //Mutex for closing all threads when kill command recieved

int handle_conn(int sock_fd); //, pthread_mutex_t *mut);

long long lim_desc, max_mem;
long long conv = 1048576; // B to MB

lim_desc = 100;
max_mem = 10485760; // 10 MBs

bool resources_available()
{
    long long cnt_descriptors = 0;
    for (long long i = 0; i < getdtablesize(); i++)
        if (fcntl(i, F_GETFD) != -1)
            cnt_descriptors++;

    long long mem_use;
    struct rusage *usage = malloc(sizeof(struct rusage));
    getrusage(RUSAGE_SELF, usage);
    mem_use = usage->ru_maxrss;
    if (cnt_descriptors > lim_desc || mem_use > max_mem)
        return false;
    return true;
}

int i = 0;

void MsgPrintClient(int client_fd, char *msg)
{
    printf("%s", msg);
    write(client_fd, msg, sizeof(msg));
}

void *dispatcher_thread_func(void *nullptr) // No input arguments needed
{
    int client_fd = -1;
    while (keep_alive)
    {
        pthread_mutex_lock(disp_pool->mutex);
        client_fd = num_qtop(disp_queue);
        if (client_fd < 0 || !resources_available())
        {
            pthread_cond_wait(disp_pool->cond_var, disp_pool->mutex);
            client_fd = num_qpop(disp_queue);
        }
        else
            num_qpop(disp_queue);

        pthread_mutex_unlock(disp_pool->mutex);

        if (client_fd >= 0)
        {
            sem_post(&cnt_thread);
            // if (fork() == 0)
            // {
            int exit_status = handle_conn(client_fd);
            // exit(0);
            // }
            // else
            // {
            // int *status = malloc(sizeof(int));
            // pid_t pid = wait(status);

            // exit_status  = WEXITSTATUS(*status);
            if (exit_status == 1) // Dll Not Found
                MsgPrintClient(client_fd, "Error! Specified Dll File not found at the provided path\n");
            else if (exit_status == 2) // Function Not Found
                MsgPrintClient(client_fd, "Error! Specified Function not found in the Dll File not found at the provided path\n");
            else if (exit_status == 3) // File Descriptor Limit
                MsgPrintClient(client_fd, "Error! File Descriptor Limit Exceeded\n");
            else if (exit_status == 5) // Kill Signal
                MsgPrintClient(client_fd, "SERVER: RECIEVED CLOSING COMMAND, CLOSING ALL THREADS"),
                    pthread_mutex_unlock(&wait_kill);
            else if (exit_status == -1)
                MsgPrintClient(client_fd, "SEGFAULT Error! File Descriptor or Memory Limit Exceeded\n");

            close(client_fd);
            sem_wait(&cnt_thread);
        }
    }
    return NULL;
}

void log_msg(const char *msg, bool terminate)
{
    printf("%s\n", msg);
    if (terminate)
        exit(-1); /* failure */
}

/*
 * Create a named (AF_LOCAL) socket at a given file path.
 * @param socket_file
 * @param is_client whether to create a client socket or server socket
 * @return Socket descriptor
 */

int make_named_socket(const char *socket_file, bool is_client)
{
    printf("Creating AF_LOCAL socket at path %s\n", socket_file);
    if (access(socket_file, F_OK) != -1)
    {
        log_msg("An old socket file exists, removing it.", false);
        if (unlink(socket_file) != 0)
        {
            log_msg("Failed to remove the existing socket file.", true);
        }
    }
    struct sockaddr_un name;
    /* Create the socket. */
    int sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        log_msg("Failed to create socket.", true);
    }

    /* Bind a name to the socket. */
    name.sun_family = AF_LOCAL;
    strncpy(name.sun_path, socket_file, sizeof(name.sun_path));
    name.sun_path[sizeof(name.sun_path) - 1] = '\0';

    /* The size of the address is
       the offset of the start of the socket_file,
       plus its length (not including the terminating null byte).
       Alternatively you can just do:
       size = SUN_LEN (&name);
   */
    ssize_t size = SUN_LEN(&name);
    // size_t size = (offsetof(struct sockaddr_un, sun_path) +
    //                strlen(name.sun_path));

    if (bind(sock_fd, (struct sockaddr *)&name, size) < 0)
    {
        log_msg("bind failed", 1);
    }

    return sock_fd;
}

/*
 * Starts a server socket that waits for incoming client connections.
 * @param socket_file
 * @param max_connects
 * _Noreturn 
 */
void start_server_socket(char *socket_file, int max_connects)
{
    int sock_fd = make_named_socket(socket_file, false);

    /* listen for clients, up to MaxConnects */
    if (listen(sock_fd, max_connects) < 0)
    {
        log_msg("Listen call on the socket failed. Terminating.", true); /* terminate */
    }

    log_msg("Listening for client connections...\n", false);
    /* Listens indefinitely */
    while (keep_alive)
    {
        struct sockaddr_in caddr; /* client address */
        int len = sizeof(caddr);  /* address length could change */

        printf("Waiting for incoming connections...\n");
        int client_fd = accept(sock_fd, (struct sockaddr *)&caddr, &len); /* accept blocks */

        // printf("Thread Cnt: %d\n", active_count);
        if (client_fd < 0)
        {
            log_msg("accept() failed. Continuing to next.", 0); /* don't terminate, though there's a problem */
        }
        else
        {
            pthread_mutex_lock(disp_pool->mutex);
            num_qpush(client_fd, disp_queue);

            pthread_cond_signal(disp_pool->cond_var);
            pthread_mutex_unlock(disp_pool->mutex);
        }
        // printf("Thread Cnt: %d\n", active_count);
    }
}

/**
 * This functions is executed in a separate thread.
 * @param sock_fd // Socket File Descriptor of client sending request
 * 
 *  */

int handle_conn(int sock_fd) //, pthread_mutex_t *mut)
{

    //ToDo:remove
    /*
    SET THREAD LIMITS IN A SEPARATE (CHILD) PROCESS 
    long long i;
    ssize_t fd_new_lim = 5;
    ssize_t fd_curr_lim = getdtablesize();

    struct rlimit *stack_lim = malloc(sizeof(struct rlimit)), *heap_lim = malloc(sizeof(struct rlimit));
    unsigned long mem_lim_mb = 8; //in MB

    unsigned long conv = 1048576;
    unsigned long mem_lim = conv * mem_lim_mb;

    stack_lim->rlim_cur = stack_lim->rlim_max = mem_lim;
    heap_lim->rlim_cur = heap_lim->rlim_max = (10ul) * mem_lim;

    setrlimit(RLIMIT_DATA, heap_lim);
    setrlimit(RLIMIT_STACK, stack_lim);
    
    for (i = 3; i < fd_curr_lim; i++)
    {
        errno = 0;
        if (i != sock_fd && fcntl(i, F_GETFD) != -1 && errno != EBADF)
            close(i);
    }
    if (sock_fd >= fd_new_lim)
    {
        errno = 0;
        i = fcntl(sock_fd, F_DUPFD, 2);

        if (i == -1 || errno == EBADF)
        {
            printf("THIS ERR: %lld %d\n", i, sock_fd);
            exit(0);
        }
        close(sock_fd);
        sock_fd = i;
    }

    struct rlimit *num_fd_lim = malloc(sizeof(struct rlimit));
    num_fd_lim->rlim_cur = fd_new_lim;
    num_fd_lim->rlim_max = fd_new_lim;

    setrlimit(RLIMIT_NOFILE, num_fd_lim);
    */

    log_msg("SERVER: thread_function: starting", false);

    char buffer[5000];
    memset(buffer, '\0', sizeof(buffer));

    int *status = malloc(sizeof(int));
    *status = 0;
    int count = read(sock_fd, buffer, sizeof(buffer));
    if (count > 0)
    {
        printf("SERVER: Received from client: %s\n", buffer);
        write(sock_fd, buffer, sizeof(buffer)); /* echo as confirmation */
        JsonNode *node = json_decode(buffer);
        if (!node)
        {
            printf("Error! Memory Limit Exceeded\n");
        }
        if (strcmp(json_find_member(node, "dll_name")->string_, "kill") == 0)
        {
            keep_alive = 0;
            return 5;
        }

        int argc = (int)json_find_member(node, "argc")->number_;
        char *args[argc];

        JsonNode *arr = json_find_member(node, "args");

        for (int i = 0; i < argc; i++)
            copyString(args + i, json_find_element(arr, i)->string_);

        char *c = exec_dll(json_find_member(node, "dll_name")->string_, json_find_member(node, "func_name")->string_, args, &status);

        for (int i = 0; i < argc; i++)
            free(*(args + i));

        write(sock_fd, c, sizeof(c));
        free(c);
    }
    close(sock_fd); /* break connection */
    log_msg("SERVER: thread_function: Done. ", false);

    return status;
    // Thread is not closed, stays awake untill server is closed or kill request is given
}

void *threadKiller(void *nullptr)
{
    pthread_mutex_lock(&wait_kill);
    int ret;
    for (int i = 0; i < disp_pool->size; i++)
        if (ret = pthread_cancel((disp_pool->threads)[i]))
            printf("SERVER: ERROR CODE %d: ERROR CLOSING THREAD !!. IGNORING", ret);
    log_msg("SERVER: SUCCESSULLY CLOSED ALL THREADS. NOW EXITING", false);
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s [Local socket file path]\n",
               argv[0]);
        exit(-1);
    }

    pthread_mutex_lock(&wait_kill);
    pthread_t thread_killer;
    pthread_create(&thread_killer, NULL, threadKiller, NULL);

    sem_init(&cnt_thread, 0, 0);
    sem_init(&bool_isended, 0, 0);

    int dispatcher_pool_cnt;
    dispatcher_pool_cnt = 10;

    disp_queue = newNumQueue(MAXQ_SIZE);
    disp_pool = newThreadPool(dispatcher_pool_cnt);

    if (beginRoutine(disp_pool, dispatcher_thread_func, NULL))
        log_msg("Error !! Unable to allocate threads", true);

    start_server_socket(argv[1], 100);
    destroyPool(disp_pool);
}

/*
JSON Trial
int main()
{
    char *c = "abcd";
    char **arg = &c;

    struct request *obj = newReq("abcd", "bcs", arg);
    JsonNode *json = json_mkobject();
    json_append_member(json, "dll_name", json_mkstring(obj->dll_name));
    json_append_member(json, "func_name", json_mkstring(obj->func_name));
    json_append_member(json, "num", json_mknumber(100));
    char *cJson = json_stringify(json, "");
    printf("%s\n", cJson);
    JsonNode *decoded = json_decode(cJson);}}}

    int ret = (int)json_find_member(decoded, "num")->number_;
    printf("%d\n", ret + 1);
}
*/
/*  ## Queue Testing
while (pq->size < 5)
{
    qpush(newReq("abcd", "bcs", arg), pq);
}

if (qpush(newReq("abcd", "bcs", arg), pq))
    printf("Error Already Full\n");

struct request *ptr;
while (pq->size)
{
    ptr = qpop(pq);
    printf("%s %s %s\n", ptr->dll_name, ptr->func_name, ptr->args[0]);
}
if (!qpop(pq))
    printf("Already Empty\n");
*/