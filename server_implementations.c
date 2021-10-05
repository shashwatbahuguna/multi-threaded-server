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

#define MAXQ_SIZE 100
int keep_alive = 1;

//##################  Dispatcher (which also acts as Socket Reciever) Thread Control ##################//

threadpool *disp_pool; // Thread pool for the dispatcher
num_queue *disp_queue; // Queue for storing incoming requests

//#####################################################################################################//

pthread_mutex_t wait_kill = PTHREAD_MUTEX_INITIALIZER; //Mutex for closing all threads when kill command recieved

int handle_conn(int sock_fd); //, pthread_mutex_t *mut);

// int active_count = 0;

int i = 0;

void MsgPrintClient(int client_fd, char *msg)
{
    printf("%s", msg);
    write(client_fd, msg, strlen(msg) + 1);
}

void *dispatcher_thread_func(void *nullptr) // No input arguments needed
{
    int client_fd = -1;
    while (keep_alive)
    {
        pthread_mutex_lock(disp_pool->mutex);
        client_fd = num_qpop(disp_queue);
        if (client_fd < 0)
        {
            pthread_cond_wait(disp_pool->cond_var, disp_pool->mutex);
            client_fd = num_qpop(disp_queue);
        }
        pthread_mutex_unlock(disp_pool->mutex);
        if (client_fd >= 0)
        {
            // if (fork() == 0)
            // {
            int exit_status = handle_conn(client_fd);
            int *status = malloc(sizeof(int));
            pid_t pid = wait(status);

            if (exit_status == 1) // Dll Not Found
                MsgPrintClient(client_fd, "Error! Specified Dll File not found at the provided path\n");
            else if (exit_status == 2) // Function Not Found
                MsgPrintClient(client_fd, "Error! Specified Function not found in the Dll File at the provided path\n");
            else if (exit_status == 3) // File Descriptor Limit
                MsgPrintClient(client_fd, "Invalid Arguments for the function given\n");
            else if (exit_status == 4)
                MsgPrintClient(client_fd, "Error Provided Function is Not Supported for DLL Execution\n");
            else if (exit_status == 5) // Kill Signal
                MsgPrintClient(client_fd, "SERVER: RECIEVED CLOSING COMMAND, CLOSING ALL THREADS\n"),
                    pthread_mutex_unlock(&wait_kill);
            else if (exit_status == -1)
                MsgPrintClient(client_fd, "SEGFAULT Error! File Descriptor or Memory Limit Exceeded\n");

            close(client_fd);
            // if (fork() == 0) // Create a child process to handle specified limits
            // {
            //     handle_conn(client_fd);
            //     exit(0);
            // }
            // else
            // {
            //     int *status = malloc(sizeof(int));
            //     pid_t pid = wait(status);

            //     int exit_status = WEXITSTATUS(*status);
            //     if (exit_status == 1) // Dll Not Found
            //         MsgPrintClient(client_fd, "Error! Specified Dll File not found at the provided path\n");
            //     else if (exit_status == 2) // Function Not Found
            //         MsgPrintClient(client_fd, "Error! Specified Function not found in the Dll File not found at the provided path\n");
            //     else if (exit_status == 3) // File Descriptor Limit
            //         MsgPrintClient(client_fd, "Error! File Descriptor Limit Exceeded\n");
            //     else if (exit_status == 5) // Kill Signal
            //         MsgPrintClient(client_fd, "SERVER: RECIEVED CLOSING COMMAND, CLOSING ALL THREADS"),
            //             pthread_mutex_unlock(&wait_kill);
            //     else if (WTERMSIG(*status) == SIGSEGV || exit_status == -1)
            //         MsgPrintClient(client_fd, "SEGFAULT Error! File Descriptor or Memory Limit Exceeded\n");

            //     close(client_fd);
            // }
        }
    }
    return NULL;
}

/*
 * Create a named (AF_LOCAL) socket at a given file path.
 * @param socket_file
 * @return Socket descriptor
 */

int make_named_socket_server(const char *socket_file)
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
    int sock_fd = make_named_socket_server(socket_file);

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

        // printf("Thread Cnt: %d\n", active_count);
        if (client_fd < 0)
        {
            log_msg("accept() failed.\n", 0); /* don't terminate, though there's a problem */
            if (errno == EMFILE)
            {
                log_msg("Error! File Descriptor Limit Reached\n. Try Increasing the limit.", false);
                // pthread_mutex_unlock(&wait_kill);
                exit(-1);
                return;
            }
            log_msg("Continuing to next.\n", 0);
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
    // printf("MAX:: %d\n", disp_queue->maxS);
    /*long long i;
    ssize_t fd_new_lim = 5;
    ssize_t fd_curr_lim = getdtablesize();

    struct rlimit *stack_lim = malloc(sizeof(struct rlimit)), *heap_lim = malloc(sizeof(struct rlimit));
    unsigned long mem_lim_mb = 8; //in MB

    unsigned long conv = 1048576;
    unsigned long mem_lim = conv * mem_lim_mb;

    stack_lim->rlim_cur = stack_lim->rlim_max = mem_lim;
    setrlimit(RLIMIT_STACK, stack_lim);
    
    for (i = 3; i < fd_curr_lim; i++)
    {
        errno = 0;
        if (i != sock_fd && fcntl(i, F_GETFD) != -1 && errno != EBADF)
            close(i);
    }
    if (sock_fd >= fd_new_lim)
    {NULL
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
    int err_code = 0;
    if (fcntl(sock_fd, F_GETFD) == -1)
    {
        printf("Error! Invalid Socket File, Connection Unexpectedly Terminated. \n");
        return 0;
    }

    log_msg("SERVER: thread_function: starting", false);

    char buffer[5000];
    memset(buffer, '\0', sizeof(buffer));

    int count = read(sock_fd, buffer, sizeof(buffer));
    if (count > 0)
    {
        printf("SERVER: Received from client: %s\n", buffer);
        write(sock_fd, buffer, sizeof(buffer)); /* echo as confirmation */

        bool valid = json_validate(buffer);
        JsonNode *node;
        int argc;
        JsonNode *arr;
        if (valid)
        {
            node = json_decode(buffer);
            if (!node || !json_find_member(node, "dll_name") || !json_find_member(node, "func_name") || !json_find_member(node, "argc") || !json_find_member(node, "args"))
                valid = false;
            else
            {
                argc = (int)json_find_member(node, "argc")->number_;
                arr = json_find_member(node, "args");
                for (int i = 0; i < argc; i++)
                    if (!json_find_element(arr, i))
                    {
                        valid = false;
                        break;
                    }
            }
        }

        if (!valid)
        {
            char *c = "SERVER: Invalid Json Data Recieved\n";
            write(sock_fd, c, strlen(c) + 1);
            printf("%s", c);
            return 0;
        }

        if (strcmp(json_find_member(node, "dll_name")->string_, "kill") == 0)
        {
            keep_alive = 0;
            return 5;
            // exit(5);
        }

        char *args[argc];

        for (int i = 0; i < argc; i++)
            copyString(args + i, json_find_element(arr, i)->string_);

        char *c = exec_dll(json_find_member(node, "dll_name")->string_, json_find_member(node, "func_name")->string_, args, argc, &err_code);

        for (int i = 0; i < argc; i++)
            free(*(args + i));

        write(sock_fd, c, strlen(c) + 1);
        free(c);
    }
    log_msg("SERVER: thread_function: Done.\n", false);

    return err_code;
    // Thread is not closed, stays awake untill server is closed or kill request is given
}

void *threadKiller(void *nullptr)
{
    pthread_mutex_lock(&wait_kill);
    int ret;
    for (int i = 0; i < disp_pool->size; i++)
        if (ret = pthread_cancel((disp_pool->threads)[i]))
            printf("SERVER: ERROR CODE %d: ERROR CLOSING THREAD !!. IGNORING\n", ret);
    log_msg("SERVER: SUCCESSULLY CLOSED ALL THREADS. NOW EXITING", false);
    exit(0);
}