/*############ Compilation And execution
gcc client.c -o client.out -ldl -lpthread
./client.out sockfile "/lib/x86_64-linux-gnu/libm.so.6" sqrt 4
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
#include <libexplain/connect.h>

#define MAXQ_SIZE 100

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

    struct sockaddr_un name;
    /* Create the socket. */
    int sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);

    if (sock_fd < 0)
        log_msg("Failed to create socket.", true);

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
    size_t size = SUN_LEN(&name);
    // (offsetof(struct sockaddr_un, sun_path) +
    //    strlen(name.sun_path));
    if (is_client)
    {
        if (connect(sock_fd, (struct sockaddr *)&name, size) < 0)
        {
            // printf("%d\n", connect(sock_fd, (struct sockaddr *)&name, size));
            // fprintf(stderr, "%s\n", explain_connect(sock_fd, (struct sockaddr *)&name, size));
            log_msg("connect failed", 1);
        }
    }

    return sock_fd;
}

/**
 * This functions is executed in a separate thread.
 * @param sock_fd
 */
void thread_function(void *p_sock_fd)
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
        write(sock_fd, buffer, sizeof(buffer)); /* echo as confirmation */
    }
    close(sock_fd); /* break connection */
    log_msg("SERVER: thread_function: Done. Worker thread terminating.", false);
    pthread_exit(NULL); // Must be the last statement
}

/**
 * Sends a message to the server socket.
 * @param msg Message to send
 * @param socket_file Path of the server socket on localhost.
 */
void send_message_to_socket(char **request_data, char *socket_file, int argc)
{
    int sockfd = make_named_socket(socket_file, true);

    /* Write some stuff and read the echoes. */
    log_msg("CLIENT: Connect to server, about to write some stuff...", false);

    // char *num = "4";

    JsonNode *req_json = request_to_json(request_data[0], request_data[1], argc - 2, request_data + 2);
    char *json_string = json_encode(req_json);

    if (write(sockfd, json_string, strlen(json_string)) > 0)
    {
        /* get confirmation echoed from server and print */
        char buffer[5000];
        memset(buffer, '\0', sizeof(buffer));

        if (read(sockfd, buffer, sizeof(buffer)) > 0)
        {
            printf("CLIENT: Received from server:: %s\n", buffer);
            read(sockfd, buffer, sizeof(buffer));
            printf("CLIENT: Result Recieved:: %s\n", buffer);
        }
    }

    json_delete(req_json);
    free(json_string);

    log_msg("CLIENT: Processing done, about to exit...", false);
    close(sockfd);
    /* close the connection */
}

int main(int argc, char **argv)
{

    if (argc < 4)
    {
        printf("Usage: %s [Local socket file path] [Location of dll to be executed] [Function Name] .....[Function Arguments].....\n",
               argv[0]);
        exit(-1);
    }
    // for (int i = 1; i < argc; i++)
    //     argv[i][strlen(argv[i])] = '\0', printf("%s,%c| ", argv[i], argv[i][strlen(argv[i])]);

    send_message_to_socket(argv + 2, argv[1], argc - 2);
}