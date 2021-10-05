// #include <stdio.h>
// #include <stdlib.h>
#include <dlfcn.h>
#include <stdlib.h>

// gcc -ldl flag is to be enabled

char *exec_dll(char *dll_name, char *func_name, char **args, int *exit_st)
{
    *exit_st = 0;
    if (access(dll_name, F_OK) == -1)
    {
        *exit_st = 1;
        return NULL;
    }

    void *fhandle = dlopen(dll_name, RTLD_LAZY);
    void *func_ptr = dlsym(fhandle, func_name);

    if (fhandle == NULL)
    {
        *exit_st = -1;
        return NULL;
    }

    printf("%s ", func_name);
    if (func_ptr == NULL)
    {
        *exit_st = 2;
        return NULL;
    }

    if (strcmp(func_name, "sleep") == 0)
    {
        unsigned int (*func)(unsigned int) = func_ptr;

        char *end;
        unsigned int time_sleep = strtoul(args[0], &end, 10);
        (*func)(time_sleep);
        char *mess = (char *)malloc(21 * sizeof(char));
        snprintf(mess, 21, "Succesfully Executed");

        return mess;
    }

    double (*func)(double);
    func = func_ptr;

    char *term;

    double v = strtod(args[0], &term);

    double res = (*func)(v);

    printf("%lf\n", res);
    dlclose(fhandle);

    ssize_t sz = snprintf(NULL, 0, "%lf", res);

    char *buff = malloc(sz);
    snprintf(buff, sz - 1, "%lf", res);

    return buff;
}

// int main(int argc, char **argv)
// {
//     char *c = "100000.0";
//     char **arg = &c;
//     exec_dll("/lib/x86_64-linux-gnu/libm.so.6", "sqrt", arg);
// }