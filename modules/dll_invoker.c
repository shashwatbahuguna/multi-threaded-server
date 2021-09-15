// #include <stdio.h>
// #include <stdlib.h>
#include <dlfcn.h>
#include <stdlib.h>

// gcc -ldl flag is to be enabled

char *exec_dll(char *dll_name, char *func_name, char **args)
{
    void *fhandle;
    double (*func)(double);

    fhandle = dlopen(dll_name, RTLD_LAZY);

    func = dlsym(fhandle, func_name);
    char *term;

    double v = strtod(args[0], &term);
    if (func == NULL)
        printf("%s", "\nNULL\n");

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