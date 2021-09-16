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
#include <libexplain/connect.h>

#include "modules/dll_invoker.c"
#include "modules/queue.c"

#define MAXQ_SIZE 100

void log_msg(const char *msg, bool terminate)
{
    printf("%s\n", msg);
    if (terminate)
        exit(-1); /* failure */
}

/**
 * Create a named (AF_LOCAL) socket at a given file path.
 * @param socket_file
 * @return Socket descriptor
 */
int make_named_socket(const char *socket_file)
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

    if (connect(sock_fd, (struct sockaddr *)&name, size) < 0)
    {
        // printf("%d\n", connect(sock_fd, (struct sockaddr *)&name, size));
        // fprintf(stderr, "%s\n", explain_connect(sock_fd, (struct sockaddr *)&name, size));
        log_msg("connect failed", 1);
    }

    return sock_fd;
}

/**
 * Sends a message to the server socket.
 * @param request_data Request Values, to be sent to the server.
 * @param socket_file Path of the server socket on localhost.
 *  @param argc Count of input arguments to the function which will be executed in dll invoker
 */
void send_message_to_socket(char **request_data, char *socket_file, int argc)
{
    int sockfd = make_named_socket(socket_file);

    /* Write some stuff and read the echoes. */
    log_msg("CLIENT: Connect to server, about to write some stuff...", false);

    // Converts the request string to JSON Node using third party module
    JsonNode *req_json = request_to_json(request_data[0],   // Path of Dll To be executed
                                         request_data[1],   // Function Name
                                         argc,              // Number of input arguments to be given to the function
                                         request_data + 2); // Pointer to starting point of input arguments

    // Encodes Node Data to string for sending to server
    char *json_string = json_encode(req_json);

    if (write(sockfd, json_string, strlen(json_string)) > 0)
    {
        /* get confirmation echoed from server and print */
        char buffer[5000];
        memset(buffer, '\0', sizeof(buffer));
        int byt;
        if (read(sockfd, buffer, sizeof(buffer)) > 0)
        {
            printf("CLIENT: Received from server:: %s\n", buffer);
            byt = read(sockfd, buffer, sizeof(buffer));
            buffer[byt] = '\0';
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
    char **ptr;                               // Pointer to request data to be transmitted
    if (argc > 2 && !strcmp(argv[2], "kill")) // kill (case sensitive) command needs no argument
    {
        char *arr[] = {argv[2], "0", "0"}; /* temporary, random array for successfully sending
                                             data to server after encoding as json string*/
        argc = 2;
        ptr = arr;
    }
    else if (argc < 4) // Invalid input
    {
        printf("Usage: %s [Local socket file path] [Location of dll to be executed] [Function Name] .....[Function Arguments].....\n",
               argv[0]);
        exit(-1);
    }
    else // Ignore executable name, sockfile
        ptr = argv + 2;

    send_message_to_socket(ptr, argv[1], argc - 4);
}