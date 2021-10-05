typedef struct request request;

struct request
{
    char *dll_name;
    char *func_name;
    char **args;
    int sock_fd;
    int argc;
};

void copyString(char **dest, char *src)
{
    size_t sz = (strlen(src) + 1);
    *dest = (char *)malloc(sz * sizeof(char));
    strcpy(*dest, src);
    (*dest)[sz - 1] = '\0';
}

JsonNode *request_to_json(char *dll_name, char *func_name, int argc, char **argv, int *err)
{
    JsonNode *json = json_mkobject();
    char *dll_name_cop, *func_name_cop, *temp_cop;
    if (!dll_name || !func_name || !argv)
    {
        *err = 1;
        printf("Error! Invalid Request Data Recieved\n");
        return NULL;
    }
    copyString(&dll_name_cop, dll_name);
    copyString(&func_name_cop, func_name);

    json_append_member(json, "dll_name", json_mkstring(dll_name_cop));
    json_append_member(json, "func_name", json_mkstring(func_name_cop));
    json_append_member(json, "argc", json_mknumber(argc));

    JsonNode *arguments = json_mkarray();

    for (int i = 0; i < argc; i++)
    {
        if (!argv[i])
        {
            *err = 1;
            printf("Error! Invalid Request Data Recieved (Arguments)\n");
            return NULL;
        }
        copyString(&temp_cop, argv[i]);
        json_append_element(arguments, mkstring(temp_cop));
    }

    json_append_member(json, "args", arguments);
}

JsonNode *request_to_jsonb(request *obj)
{
    JsonNode *json = json_mkobject();
    json_append_member(json, "dll_name", json_mkstring(obj->dll_name));
    json_append_member(json, "func_name", json_mkstring(obj->func_name));
    json_append_member(json, "argc", json_mknumber(obj->argc));

    JsonNode *arguments = json_mkarray();

    for (int i = 0; i < obj->argc; i++)
        json_append_element(arguments, mkstring((obj->args)[i]));

    json_append_member(json, "args", arguments);
}

struct request *newReq(char *dll_name, char *func_name, char **args, int argc)
{
    struct request *Nreq = (struct request *)malloc(sizeof(struct request));

    copyString(&(Nreq->dll_name), dll_name);
    copyString(&(Nreq->func_name), func_name);

    Nreq->args = (char **)malloc(sizeof(char *) * argc);
    Nreq->argc = argc;
    for (int i = 0; i < argc; i++)
        copyString((Nreq->args) + i, args[i]);

    return Nreq;
}

void clearReq(struct request *req)
{
    free(req->dll_name);
    free(req->func_name);
    for (int i = 0; i < req->argc; i++)
        free((req->args)[i]);
    free(req->args);
    free(req);
    return;
}