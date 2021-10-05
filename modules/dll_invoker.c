#include <dlfcn.h>
#include <stdlib.h>

char *exec_dll(char *dll_name, char *func_name, char **args, int argc, int *errcode)
{
    printf("%s, %s, %s,\n", dll_name, func_name, args[0]);
    *errcode = 0;
    if (access(dll_name, F_OK))
    {
        *errcode = 1;
        return NULL;
    }

    void *fhandle = dlopen(dll_name, RTLD_NOW);

    if (fhandle == NULL)
    {
        *errcode = -1;
        return NULL;
    }

    void *func_ptr = dlsym(fhandle, func_name);

    if (func_ptr == NULL)
    {
        *errcode = 2;
        return NULL;
    }
    bool valid = true;
    if (strcmp(func_name, "sqrt") == 0 || strcmp(func_name, "floor") == 0 || strcmp(func_name, "ceil") == 0 || strcmp(func_name, "cos") == 0 || strcmp(func_name, "sin") == 0 || strcmp(func_name, "sinh") == 0 || strcmp(func_name, "cosh") == 0)
    {
        if (argc != 1)
            valid = false;
        else
        {
            double (*func)(double);
            func = func_ptr;

            char *term;

            double v = strtod(args[0], &term);
            if (term == args[0])
                valid = false;
            else
            {
                double res = (*func)(v);

                printf("%lf\n", res);
                dlclose(fhandle);

                ssize_t sz = snprintf(NULL, 0, "%lf", res);

                char *buff = malloc(sz);
                snprintf(buff, sz - 1, "%lf", res);

                return buff;
            }
        }
    }
    else if (strcmp(func_name, "sleep") == 0)
    {
        if (argc != 1)
            valid = false;
        else
        {
            unsigned int (*func)(unsigned int) = func_ptr;

            char *end;
            unsigned long time_sleep = strtoul(args[0], &end, 10);
            if (end == args[0])
                valid = false;
            else
            {
                (*func)(time_sleep);
                char *mess = (char *)malloc(21 * sizeof(char));
                snprintf(mess, 21, "Succesfully Executed");

                return mess;
            }
        }
    }
    else if (strcmp(func_name, "pow") == 0 || strcmp(func_name, "fmod") == 0)
    {
        if (argc != 2)
            valid = false;
        else
        {
            double (*func)(double, double);
            func = func_ptr;

            // printf("Dunc%s\n%d\n", func_name, (int)(func_ptr != NULL));
            // printf("Args: %s, %s\n", args[0], args[1]);

            char *term_a, *term_b;

            double v_a = strtod(args[0], &term_a), v_b = strtod(args[1], &term_b);

            if (term_b == args[0] || term_b == args[1])
                valid = false;
            else
            {
                double res = (*func)(v_a, v_b);

                printf("%lf\n", res);
                dlclose(fhandle);

                ssize_t sz = snprintf(NULL, 0, "%lf", res);

                char *buff = malloc(sz);
                snprintf(buff, sz - 1, "%lf", res);

                return buff;
            }
        }
    }
    else
        *errcode = 4;

    if (!valid)
        *errcode = 3;

    return NULL;
}