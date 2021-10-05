#include "modules/json_module/json.h" //Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)
#include "modules/json_module/json.c" //Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)

#include <unistd.h>

#include "modules/dll_invoker.c"
#include "modules/queue.c"
#include "modules/thread_pool.c"
void log_msg(const char *, bool);

#include <signal.h>
#include "client_implementations.c"
#include "server_implementations.c"

void log_msg(const char *msg, bool terminate)
{
    printf("%s\n", msg);
    if (terminate)
        exit(-1); /* failure */
}

int main()
{
    // printf("\n\t\tClient Functions Tests:\n");
    printf("\n########## Make Named Function #############\n\n");
    printf("make_named_socket_client(\"Invalid Filename\")\n");
    if (fork() == 0)
        printf("Return Val:%d", make_named_socket_client("akjvn"));
    else
        wait(NULL);

    printf("\n########## Send message to socket: #############\n\n");

    printf("Expects valid arguments and count (checked in main)\n");

    printf("Invalid Arguments Return Value:\n");
    if (fork() == 0)
        send_message_to_socket(NULL, "invalidfile", 0);
    else
        wait(NULL);
    printf("##################################################\n");

    int *er = malloc(sizeof(int));
    char *c = malloc(10);

    printf("\n########## Request To Json Function #############\n\n");

    printf("request_to_json(NULL, \"advkn\", 3, &c, er)\n");
    request_to_json(NULL, "advkn", 3, &c, er);
    printf("Err Code: %d\n", *er);

    printf("##################################################\n");

    printf("\n########## DLL Execution Function #############\n\n");
    *er = 0;

    char *num = malloc(12);
    snprintf(num, 7, "%s", "10000");

    printf("Valid Call: sqrt(10000)\n");
    printf("%s\n", exec_dll("/lib/x86_64-linux-gnu/libm.so.6", "sqrt", &num, 1, er));
    printf("Err Code: %d\n", *er);

    printf("Valid Call: pow(8, 4)\n");
    char *ptr[] = {"8.0", "4"};
    exec_dll("/lib/x86_64-linux-gnu/libm.so.6", "pow", ptr, 2, er);
    printf("Err Code: %d\n", *er);

    printf("\nInvalid Dll Call: \n");
    exec_dll("/lib/x86_64-linux-gnu/libm.so.", "sqrt", &num, 1, er);
    printf("Err Code: %d\n", *er);

    printf("\nFunction Call with Invalid Arguments: \n");
    exec_dll("/lib/x86_64-linux-gnu/libm.so.6", "sqrta", &num, 1, er);
    printf("Err Code: %d\n", *er);

    printf("\nUnsupported but valid Function Call: \n");
    exec_dll("/lib/x86_64-linux-gnu/libuuid.so.1.3.0", "nanosleep", &num, 1, er);
    printf("Err Code: %d\n", *er);

    printf("##################################################\n");
}