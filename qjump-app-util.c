/* 
 * Copyright (c) 2015, Matthew P. Grosvenor
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following 
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
 * disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products 
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Many thanks to the authors of the following help on which this work is based
 *
 * - http://scaryreasoner.wordpress.com/2007/11/17/using-ld_preload-libraries-and-glibc-backtrace-function-for-debugging/
 * - http://stackoverflow.com/questions/3275015/ld-preload-affects-new-child-even-after-unsetenvld-preload
 * - http://tldp.org/HOWTO/Program-Library-HOWTO/dl-libraries.html
 *
 */


#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <netinet/ip.h>


#define FAMS 12
char* ADDRESS_FAMILIY[FAMS] =
{
        "PF_UNSPEC",
        "PF_UNIX",
        "PF_INET",
        "PF_AX25",
        "PF_IPX",
        "PF_APPLETALK",
        "PF_NETROM",
        "PF_BRIDGE",
        "PF_ATMPVC",
        "PF_X25",
        "PF_INET6",
};

#define SOCKS 7
char* SOCK_TYPE[SOCKS] =
{
        "NULL",
        "SOCK_STREAM",
        "SOCK_DGRAM",
        "SOCK_RAW",
        "SOCK_RDM"
        "SOCK_SEQPACKET",
        "SOCK_DCCP"
};


typedef struct {
    int verbosity;
    int prioirty;
    int window_size;
} params_t;

static int formatout(int level, const char* format, ...);




//parameters are passed in using environment variables, we want to make this
//look like a program when it's actually a work around
static params_t* params = NULL;
static void do_params()
{

    if(params == NULL){
        params = malloc(sizeof(params_t));

        params->verbosity = 0;
        char* qjau_verbosity = getenv("QJAU_VERBOSITY");
        if(qjau_verbosity){
            params->verbosity = strtol(qjau_verbosity,NULL,10);

            formatout(3,"QJAU_VERBOSITY=%i\n", params->verbosity);
        }

        params->prioirty = 0;
        char* qjau_priority = getenv("QJAU_PRIORITY");
        if(qjau_priority){
            params->prioirty = strtol(qjau_priority,NULL,10);

            //Clamp the priorities so that we don't go out of range
            if(params->prioirty > 7){
                params->prioirty = 7;
            }
            if(params->prioirty < 0){
                params->prioirty = 0;
            }
            
            formatout(3,"QJAU_PRIORITY=%i\n", params->prioirty);
        }

        char* qjau_window = getenv("QJAU_WINDOW");
        if(qjau_window){
            params->window_size = strtol(qjau_window,NULL,10);	    
            //params->window_size = params->window_size < 2048 ? 2048 : params->window_size;

            formatout(3,"QJAU_WINDOW=%i\n", params->window_size);
        }
    }
}


//Just wrap up the print function so that we can control verbosity
int formatout(int level, const char* format, ...)
{
    va_list args;
    va_start(args,format);
    int result = 0;

    if(params->verbosity >= level){
        fprintf(stderr, "QJAU[%i]: ", level);
        result = vfprintf(stderr,format,args);
        fflush(stderr);
    }

    va_end(args);
    return result;
}



//Find the library version of the function that we are wrapping
static void get_next_fn(void** next_fn, char* fn)
{
    char* msg;

    if(! *next_fn){

        formatout(3, "wrapping %s\n", fn);

        *next_fn = dlsym(RTLD_NEXT, fn);
        if ((msg = dlerror()) != NULL) {
            formatout(0,  "dlopen failed on %s: %s\n", fn, msg);
            exit(1);
        } else {
            formatout(3,  "next_%s = %p\n", fn, *next_fn);
        }
    }
}

//Set socket options on the given socket, this takes a priority parameter to allow
//extension to per connection priority setting later
void set_opts(int sockfd, int priority, int window_size, char* fn)
{
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
    if(ret != 0){
        formatout(0,"Failed setting priority to %i in %s() call!\n", priority, fn);
        exit(-1);
        return;
    }
    formatout(0, "FD %i set to piroirty %i using %s()\n", sockfd, priority,fn);

    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &window_size, sizeof(window_size));
    if(ret != 0){
        formatout(0,"Failed setting RCV window size to %i in %s() call!\n", window_size, fn);
        exit(-1);
        return;
    }
    formatout(0, "FD %i set RCV window size to %i using %s()\n", sockfd, window_size,fn);

    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &window_size, sizeof(window_size));
    if(ret != 0){
        formatout(0,"Failed setting SND window size to %i in %s() call!\n", window_size, fn);
        exit(-1);
        return;
    }
    formatout(0, "FD %i set SND window size to %i using %s()\n", sockfd, window_size,fn);


}


//Put the first 4 bytesof the IP in the array. Put the port in the last spot.
void get_address_bytes(int* addr_bytes, const struct sockaddr *addr)
{
    struct sockaddr_in* inaddr = (struct sockaddr_in*)addr;
    addr_bytes[0] = inaddr->sin_addr.s_addr >> 0  & 0xFF;
    addr_bytes[1] = inaddr->sin_addr.s_addr >> 8  & 0xFF;
    addr_bytes[2] = inaddr->sin_addr.s_addr >> 16 & 0xFF;
    addr_bytes[3] = inaddr->sin_addr.s_addr >> 24 & 0xFF;

    addr_bytes[4] = inaddr->sin_port;

}

//Wrap up the socket() call
static int (*next_socket)(int domain, int type, int protocol) = NULL;
int socket(int domain, int type, int protocol)
{
    char* fn_name = "socket";

    do_params();
    get_next_fn((void**)&next_socket,fn_name);

    int sockfd = next_socket(domain, type, protocol);

    char* IS_SOCK_CLOEXEC  = type & 02000000 ? " +SOCK_CLOEXEC" : "";
    char* IS_SOCK_NONBLOCK = type & 00004000 ? " +SOCK_NONBLOCK" : "";

    type = type & 02000000 ? type & ~02000000 : type;
    type = type & 00004000 ? type & ~00004000 : type;

    formatout(1, "socket(%s,%s%s%s,%i) = %i\n",
            domain < FAMS ? ADDRESS_FAMILIY[domain] : "?",
            type < SOCKS ? SOCK_TYPE[type] : "?",
            IS_SOCK_CLOEXEC,
            IS_SOCK_NONBLOCK,
            protocol,
            sockfd);

    set_opts(sockfd,params->prioirty,params->window_size,fn_name);

    return sockfd;
}


//Wrap up the bind() call
static int (*next_bind)(int sockfd, const struct sockaddr *addr, socklen_t addrlen) = NULL;
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    char* fn_name = "bind";

    do_params();
    get_next_fn((void**)&next_bind,fn_name);

    int addr_bytes[5] = {0};
    get_address_bytes(addr_bytes,addr);
    formatout(1, "bind(%i, %i.%i.%i.%i:%i, %u)\n", sockfd, addr_bytes[0], addr_bytes[1], addr_bytes[2], addr_bytes[3], addr_bytes[4], addrlen);

    set_opts(sockfd,params->prioirty,params->window_size,fn_name);

    return next_bind(sockfd,addr,addrlen);
}

//Wrap up the accept() call
static int (*next_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen) = NULL;
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    char* fn_name = "accept";

    do_params();
    get_next_fn((void**)&next_accept,fn_name);

// XXX BUG HERE!
//	printf("--->>> %u\n", __LINE__);
//    int addr_bytes[5] = {0};
//    get_address_bytes(addr_bytes,addr);
//    formatout(1, "accept(%i, %i.%i.%i.%i:%i, %u)\n", sockfd, addr_bytes[0], addr_bytes[1], addr_bytes[2], addr_bytes[3], addr_bytes[4], addrlen);


    int new_fd = next_accept(sockfd, addr, addrlen);

    if(new_fd < 0){
        fprintf(stderr, "Failed on accept\n");
        return new_fd;
    }

    set_opts(new_fd,params->prioirty,params->window_size, fn_name);

    return new_fd;
}


//Wrap up the connect() call
static int (*next_connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen) = NULL;
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{

    char* fn_name = "connect";
    do_params();

    get_next_fn((void**)&next_connect,fn_name);

    int addr_bytes[5] = {0};
    get_address_bytes(addr_bytes,addr);
    formatout(1, "connect(%i, %i.%i.%i.%i:%i, %u)\n", sockfd, addr_bytes[0], addr_bytes[1], addr_bytes[2], addr_bytes[3], addr_bytes[4], addrlen);

    set_opts(sockfd,params->prioirty, params->window_size, fn_name);

    int ret = next_connect(sockfd, addr, addrlen);

    return ret;
}


//Wrap up the connect() call
static int (*next_close)(int sockfd) = NULL;
int close(int sockfd)
{

    char* fn_name = "close";
    do_params();

    get_next_fn((void**)&next_close,fn_name);

    formatout(0, "close(%i)\n", sockfd);

    return next_close(sockfd);
}



