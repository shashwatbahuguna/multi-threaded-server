/*############ Compilation And execution
gcc client.c -o client.out -ldl -lpthread
./client.out sockfile "/lib/x86_64-linux-gnu/libm.so.6" sqrt 4
//############*/

#include "modules/json_module/json.h" //Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)
#include "modules/json_module/json.c" //Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)

#include <unistd.h>

#include "modules/dll_invoker.c"
#include "modules/queue.c"
#include "modules/thread_pool.c"

void log_msg(const char *msg, bool terminate)
{
    printf("%s\n", msg);
    if (terminate)
        exit(-1); /* failure */
}

#include "client_implementations.c"

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