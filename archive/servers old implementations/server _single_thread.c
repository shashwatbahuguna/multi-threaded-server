/*############ Compilation And execution
gcc server.c -o serv.out -ldl -lpthread
./serv.out sockfile
//############*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/un.h>
#include <stddef.h>
#include <limits.h>
#include "modules/dll_invoker.c"
#include "modules/queue.c"

#define MAXQ_SIZE 100

//################################## Dispatcher Related Variables ##################################//
pthread_t *dispatcher_pool;                              // Thread pool for the dispatcher
queue *disp_queue;                                       // Queue for storing incoming requests
pthread_mutex_t disp_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex to ensure queue is worked by threads one at a time
pthread_cond_t disp_cond_var = PTHREAD_COND_INITIALIZER; // Conditional var to serve as signal to awake a thread
//##################################################################################################//

//################################## Server Related Variables ##################################//
// pthread_t *server_pool;                                  // Thread pool for the server to accept incoming data from client
// queue *server_queue;                                     // Queue for storing incoming requests
// pthread_mutex_t serv_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex to ensure queue is worked by threads one at a time
// pthread_cond_t serv_cond_var = PTHREAD_COND_INITIALIZER; // Conditional var to serve as signal to awake a thread
//##############################################################################################//

bool create_worker_thread(int fd);

void *dispatcher_thread_func(void *nullptr) // No input arguments needed
{
    return NULL;
    while (true)
    {
        node *top;
        pthread_mutex_lock(&disp_mutex);
        top = qpop(disp_queue);
        // if (!top)
        // {
        // }
    }
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
    size_t size = (offsetof(struct sockaddr_un, sun_path) +
                   strlen(name.sun_path));

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
    while (1)
    {
        struct sockaddr_in caddr; /* client address */
        int len = sizeof(caddr);  /* address length could change */

        printf("Waiting for incoming connections...\n");
        int client_fd = accept(sock_fd, (struct sockaddr *)&caddr, &len); /* accept blocks */

        if (client_fd < 0)
        {
            log_msg("accept() failed. Continuing to next.", 0); /* don't terminate, though there's a problem */
            continue;
        }

        /* Start a worker thread to handle the received connection. */
        if (!create_worker_thread(client_fd))
        {
            log_msg("Failed to create worker thread. Continuing to next.", 0);
            continue;
        }

    } /* while(1) */
}

/**
 * This functions is executed in a separate thread.
 * @param sock_fd
 * 
 *  */
void *thread_function(void *p_sock_fd)
{
    int sock_fd = *((int *)p_sock_fd);
    free(p_sock_fd);

    log_msg("SERVER: thread_function: starting", false);

    char buffer[5000];
    memset(buffer, '\0', sizeof(buffer));

    int count = read(sock_fd, buffer, sizeof(buffer));
    if (count > 0)
    {
        printf("SERVER: Received from client: %s\n", buffer);
        write(sock_fd, buffer, sizeof(buffer));
        // write(sock_fd, buffer, sizeof(buffer)); /* echo as confirmation */
        JsonNode *node = json_decode(buffer);

        int argc = (int)json_find_member(node, "argc")->number_;
        char **args = (char **)malloc(sizeof(char *) * (argc));
        JsonNode *arr = json_find_member(node, "args");

        for (int i = 0; i < argc; i++)
        {
            copyString(args + i, json_find_element(arr, i)->string_);
        }

        char *c = exec_dll(json_find_member(node, "dll_name")->string_, json_find_member(node, "func_name")->string_, args);

        json_delete(node);

        write(sock_fd, c, sizeof(c));
    }
    close(sock_fd); /* break connection */
    log_msg("SERVER: thread_function: Done. Worker thread terminating.", false);

    pthread_exit(NULL); // Must be the last statement
}

/*
 * This function launches a new worker thread.
 * @param sock_fd
 * @return Return true if thread is successfully created, otherwise false.
 */
bool create_worker_thread(int sock_fd)
{
    log_msg("SERVER: Creating a worker thread.", false);
    pthread_t thr_id;
    int *p_sock_fd = malloc(sizeof(int));
    *p_sock_fd = sock_fd;
    int rc = pthread_create(&thr_id,
                            /* Attributes of the new thread, if any. */
                            NULL,
                            /* Pointer to the function which will be
             * executed in new thread. */
                            thread_function,
                            /* Argument to be passed to the above
             * thread function. */
                            (void *)p_sock_fd);
    if (rc)
    {
        log_msg("SERVER: Failed to create thread.", false);
        return false;
    }
    return true;
}

/**
 * Sends a message to the server socket.
 * @param msg Message to send
 * @param socket_file Path of the server socket on localhost.
 */
void send_message_to_socket(char *msg, char *socket_file)
{
    int sockfd = make_named_socket(socket_file, true);

    /* Write some stuff and read the echoes. */
    log_msg("CLIENT: Connect to server, about to write some stuff...", false);
    if (write(sockfd, msg, strlen(msg)) > 0)
    {
        /* get confirmation echoed from server and print */
        char buffer[5000];
        memset(buffer, '\0', sizeof(buffer));
        if (read(sockfd, buffer, sizeof(buffer)) > 0)
        {
            printf("CLIENT: Received from server:: %s\n", buffer);
        }
    }
    log_msg("CLIENT: Processing done, about to exit...", false);
    close(sockfd);
    /* close the connection */
}

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        printf("Usage: %s [Local socket file path]\n",
               argv[0]);
        exit(-1);
    }

    int server_pool_cnt, dispatcher_pool_cnt;
    server_pool_cnt = dispatcher_pool_cnt = 2;

    // server_pool = malloc(sizeof(pthread_t) * server_pool_cnt);
    // for (int i = 0; i < server_pool_cnt; i++)
    //     if (pthread_create(&server_pool[i], NULL, server_thread_func, NULL))
    //         log_msg("Error Unable to allocate threads", true);
    // server_queue = newQueue(INT_MAX);

    dispatcher_pool = malloc(dispatcher_pool_cnt * sizeof(pthread_t));
    for (int i = 0; i < dispatcher_pool_cnt; i++)
        if (pthread_create(&dispatcher_pool[i], NULL, dispatcher_thread_func, NULL))
            log_msg("Error Unable to allocate threads\n", true);
    disp_queue = newQueue(MAXQ_SIZE);

    start_server_socket(argv[1], 10);

    // char *c = "100000.0";
    // char **arg = &c;
    // struct queue *pq = newQueue(5);
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
/*  ## queue testing
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