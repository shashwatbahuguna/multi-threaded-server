# Multi-threaded-server

Multi Threaded Server with dll execution.

## Supported DLL Functions

### A. Maths

dll library: "/lib/x86_64-linux-gnu/libm.so.6"
functions: sqrt, cos, sin, cosh, sinh, floor, ceil, 
           pow, fmod (modulo)


### B. Others
dll library: "/lib/x86_64-linux-gnu/libwrap.so.0"
function: sleep 

### special:
Used to kill the server
"./client.out sockfile kill"

dll library: N.A.
function: kill

## Compilation and Running:

### For Server File:
NOTE: THREADS EQUAL TO THREAD COUNT ARE ALLOCATED INITIALLY AND ARE ALWAYS ACTIVE (can be locked by mutex or cond_wait)

Compilation
gcc server.c -o serv.out -ldl -lpthread

Execution
Usage: [filename.out] [Local socket file path] [Active Thread Count] [File Descriptor Limit] [Memory Limit MB]

./serv.out sockfile 15 20 10

###  For Client File:

Compilation
gcc client.c -o client.out -ldl -lpthread

Execution
Usage: [filename.out] [Local socket file path] [Location of dll to be executed] [Function Name] .....[Function Arguments].....
./client.out sockfile "/lib/x86_64-linux-gnu/libm.so.6" sqrt 4

#### For Unit Test

Compilation
gcc unit_test_main.c -o test.out -ldl -lpthread

Execution
./test.out

#### Bash For Testing Concurrency (LINUX)
Launches in two separate terminals a server and concurrent clients.

Execution
./test_conn.bash

( 
  Sleep for 2 sec command is given by 100 clients simultaneuosly to server with 20 threads
  Concurrency is Evident Since tasks are completed in 5 sets of 20 clients, taking a total of 10 secs.
  (Would have been 200 seconds in single threaded handling.)
)

### Working Principle

NOTE THAT ATLEAST 5 FILE DESCRIPTORS are needed always. (For: STDIN, STDOUT, STDERR, SOCKET CONN, DLOPEN LIBRARY). 
Note that since threads are always active, the total memory size should be greater than the minimum stack size of each thread * thread count. Check minimum stack size here: 
https://man7.org/linux/man-pages/man3/pthread_create.3.html#:~:text=per-architecture,BUGS .

The server components are divided into server_implementations.c (contains all except main) and server.c (which is the main file). server.c takes input from the user and after validation, enables a local socket at the given filename.

A thread pool is then created which constantly checks a queue which contains the incoming request data. (socket file descritptor).
To make the operation of popping thread safe, mutex variable are used to ensure that only one thread is popping at a time.

Further, a conditional variable is used and when the thread finds a queue to be empty, it goes to sleep, waiting on a signal to the conditional variable. This is done to prevent constant cycling by all threads, which can use upto 100% CPU cycles.

The server then listens with an infinite loop with a blocking accept call. On recieving an accept call, the client socket descritor is pushed into a queue.
When pushing into the queue, a signal is given to the conditional variable, thus, awaking exactly one thread (which previously locked te mutex before the pop call which found the queue to be empty).

The recieved data is in the form of a stringified json struct. This is using third party library for json encoding and decoding, which encodes the string at the side of the client before sending it to the server.

One of the threads reads data from the file socket and decodes it (after validation). This data is sent to the dll execution function (defined in dll_invoker.c), where it is checked against supported function and is accordingly exectuted or returns an appropriate error code.

The returned error code is then processed and accordingly a response is sent to the client.
