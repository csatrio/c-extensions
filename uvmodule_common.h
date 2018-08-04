#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "uv.h"
#include "http_parser.h"

// for system types and linux sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <Python.h>

#define CRLF "\r\n"
#define MAX_WRITES 1000
#define CAPSULE_NAME "HttpData"
#define TIMER_NAME "UV_TIMER"
#define TASK_NAME "UV_TASK"
#define ASSERT_STATUS(status, msg) \
  if (status != 0) { \
    printf("%s : %s\n", msg, uv_err_name(status));\
  }
#define uv_status(msg, status){ASSERT_STATUS(status,msg);}
#define println(msg){printf("%s\n", msg);}
#define checknull(obj, name){printf("object %s is %s!\n",name, obj == NULL ? "null" : "not null");}
#define SAFE_CLOSE(handle, close_cb){if(!uv_is_closing((uv_handle_t*)handle)) uv_close((uv_handle_t*)handle, close_cb);}

#define INIT_AI_TCP(ai){\
ai.ai_family = PF_INET;\
ai.ai_socktype = SOCK_STREAM;\
ai.ai_protocol = IPPROTO_TCP;\
ai.ai_flags = 0;}

#define EMPTY_RESPONSE "HTTP/1.1 200 OK"CRLF\
"Content-Type: text/html"CRLF\
"Connection: closed"CRLF\
CRLF"SERVER SEND AN EMPTY RESPONSE : PLEASE FIX YOUR CALLBACK HANDLER !!"

#define PRINT_REFCOUNT(obj, name){printf("reference count of %s : %ld\n", name, Py_REFCNT(obj));}
#define print_refcount(obj){PRINT_REFCOUNT(obj, "object");}
#define MSG_NOT_CALLABLE "callback must be a callable function, e.g : def function_cb(): ..."
#define MSG_NOT_TUPLE "parameter must be a python tuple."
#define EXCEPTION(type, msg){printf("ERROR: %s\n", msg);PyErr_SetString(type, msg);}
#define CHECK_CALLABLE(cb, err_msg){if (!PyCallable_Check(cb)){\
    EXCEPTION(PyExc_TypeError, err_msg? err_msg : MSG_NOT_CALLABLE); Py_RETURN_NONE;}}
#define CHECK_TUPLE(tup, err_msg){if (!PyTuple_Check(tup)){\
    EXCEPTION(PyExc_TypeError, err_msg? err_msg : MSG_NOT_TUPLE); Py_RETURN_NONE;}}

typedef struct HttpData
{
    uv_tcp_t handle;           // tcp connection handle
    uv_buf_t response;         // response write buffer
    uv_async_t *async;         // this async handler is NULL if async is set to false
    uv_write_t write;          // uv write request
    PyObject *dict;            // store http header
    PyObject *self;            // pycapsule to self
    PyObject *callback_args;   // callback args to python handler
    void *attachment;          // store temp var etc
} HttpData;

typedef struct TimerTask
{
    uv_timer_t *timer;
    PyObject *task_param;
    PyObject *timer_cb;
} TimerTask;

typedef struct AsyncTask
{
    uv_work_t task;
    PyObject *task_param;
    PyObject *task_cb;
    PyObject *task_complete_cb;
} AsyncTask;

typedef struct ClientData
{
    char* host;
    char* port;
    uv_getaddrinfo_t req_addr;
    uv_connect_t conn;
    PyObject* callback;
} ClientData;

typedef struct addrinfo addrinfo_t;

// STATIC VARIABLES
static uv_loop_t *UV_LOOP;
static uv_tcp_t socket_;
static const char *ip = "0.0.0.0";
static int port = 8000;
static int BUFFER_SIZE = 65536; // default is 64kb
static http_parser_settings *settings = NULL;
static size_t wrapper_size = sizeof(HttpData);
static size_t client_size = sizeof(ClientData);
static bool ASYNC = false;
static addrinfo_t AI_TCP;

// LONG LIVED PYOBJECTS
static PyObject *EMPTY_ARGS = NULL;
static PyObject *REQ_URL = NULL;
static PyObject *REQ_BODY = NULL;
static PyObject *REQ_METHOD = NULL;

static inline void NULLIFY_REF(PyObject* obj){
    if(obj!=NULL && obj!=Py_None){
        while(Py_REFCNT(obj) > 0){
            printf("refcount : %ld\n", Py_REFCNT(obj));
            Py_DECREF(obj);
        }
    }
}

static inline bool IS_STATIC(PyObject *var)
{
    return (var == EMPTY_ARGS || var == REQ_URL || var == REQ_BODY || var == REQ_METHOD);
}

static inline void buffer_allocator(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = (char*)malloc(BUFFER_SIZE);
    buf->len = BUFFER_SIZE;
}

static inline PyObject *encapsulate(HttpData *data)
{
    data->self = PyCapsule_New(data, CAPSULE_NAME, NULL);
    return data->self;
}

static inline void free_client_attachment(ClientData* c)
{
    if(c){
        free(c->host);
        free(c->port);
        free(c);
    }
}

static void free_heap_handle(uv_handle_t* handle){
    free(handle);
}

static void free_async_handle(uv_async_t* async){
    if(async != NULL)
        uv_close((uv_handle_t*)async, free_heap_handle);
}
