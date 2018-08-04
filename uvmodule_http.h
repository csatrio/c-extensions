#pragma once
#include "uvmodule_common.h"
#include "uvmodule_parser.h"

// SERVER METHOD PROTOTYPES
static void free_handle(uv_handle_t *handle);
static void on_connect(uv_stream_t* handle, int status);
static void on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
static void write_end(uv_write_t* response, int status);
static void write_response(HttpData* data);
static void write_async(uv_async_t* async);

// CLIENT METHOD PROTOTYPES
static void create_client(const char* httpmsg, const char* host, const char* port, PyObject* callback);
static void create_client_cb(uv_async_t* async);
static void on_resolved (uv_getaddrinfo_t* req, int status, struct addrinfo* res);
static void httpclient_on_connect(uv_connect_t* req, int status);
static void httpclient_write(uv_write_t *req, int status);
static void httpclient_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);

static PyObject *httpserver_callback = NULL;

static void free_handle(uv_handle_t *handle)
{
    HttpData *wrapper = (HttpData *)(handle->data);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(wrapper->dict, &pos, &key, &value))
    {
        if (!IS_STATIC(key))
            Py_DECREF(key);
        Py_DECREF(value);
    }

    PyDict_Clear(wrapper->dict);
    Py_XDECREF(wrapper->dict);
    Py_XDECREF(wrapper->callback_args);
    Py_XDECREF(wrapper->self);
    free_client_attachment(wrapper->attachment);
    free_async_handle(wrapper->async);
    free(wrapper->response.base);
    free(wrapper);
}

// HTTP SERVER PART
static void on_connect(uv_stream_t *handle, int status)
{
    HttpData *wrapper = (HttpData *)malloc(wrapper_size);
    wrapper->dict = PyDict_New();
    wrapper->response.base = NULL;
    wrapper->response.len = 0;
    wrapper->async = NULL;
    wrapper->attachment = NULL;
    wrapper->async = NULL;

    if(ASYNC){
        wrapper->async = (uv_async_t *)malloc(sizeof(uv_async_t));
        uv_async_init(UV_LOOP, wrapper->async, write_async);
        wrapper->async->data = wrapper;
    }

    // init tcp handle
    uv_tcp_init(UV_LOOP, &wrapper->handle);

    // client reference for handle data on requests
    wrapper->handle.data = wrapper;

    // accept connection passing in refernce to the client handle
    uv_accept(handle, (uv_stream_t *)&wrapper->handle);

    // allocate memory and attempt to read.
    uv_read_start((uv_stream_t *)&wrapper->handle, buffer_allocator, on_read);
}

static void on_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf)
{
    http_parser parser;
    HttpData *data = (HttpData *)tcp->data;

    // ON NO ERROR
    if (nread >= 0)
    {
        http_parser_init(&parser, HTTP_REQUEST);
        parser.data = data;
        ssize_t parsed = http_parser_execute(&parser, settings, buf->base, nread);

        // close handle on parse error
        if (parsed < nread)
        {
            println("Parse Error : closing handle");
            SAFE_CLOSE(&data->handle, free_handle);
        }

        else{
            data->callback_args = Py_BuildValue("(NO)", data->dict, encapsulate(data)); // (N) does not increment refcount
            PyEval_CallObject(httpserver_callback, data->callback_args);
            if (!ASYNC)
                write_response(data);
        }
    }

    // ON ERROR
    else
    {
        // @TODO - debug error : if (nread != UV_EOF) {}
        // close handle
        println("Read Error!!");
        SAFE_CLOSE(&data->handle, free_handle);
    }

    free(buf->base); // free the read buffer
}

static void write_response(HttpData *data)
{
    uv_write(&data->write, (uv_stream_t *)&data->handle, &data->response, 1, write_end);
}

static void write_end(uv_write_t *response, int status)
{
    SAFE_CLOSE(response->handle, free_handle);
};

static void write_async(uv_async_t *async)
{
    HttpData *data = (HttpData *)async->data;
    write_response(data);
}


// HTTP CLIENT PART

// create client using async handle to avoid allocating memory inside python function
// this will prevent segfault if python tried to reclaim memory allocated by libuv
static void create_client(const char* httpmsg, const char* host, const char* port, PyObject* callback){
    uv_async_t async_create;
    size_t data [] = {(size_t)httpmsg, (size_t)host, (size_t)port, (size_t)callback}; // pass pointers as size_t array
    uv_async_init(UV_LOOP, &async_create, create_client_cb);
    async_create.data = &data;
    uv_async_send(&async_create);
}

// client memory will be allocated here
static void create_client_cb(uv_async_t* async){
    size_t* d = (size_t*)async->data;
    char *httpmsg = (char*)d[0];
    char *host = (char*)d[1];
    char *port = (char*)d[2];
    PyObject* callback = (PyObject*)d[3];

    HttpData* data = (HttpData*)malloc(wrapper_size);
    data->dict = PyDict_New();
    data->async = NULL;

    ClientData* client = (ClientData*)malloc(client_size);
    client->host = strdup(host);
    client->port = strdup(port);
    client->req_addr.data = data;
    client->callback = callback;
    data->attachment = client;

    size_t wr_len = strlen(httpmsg);
    data->response.base = (char*)malloc(wr_len + 1);
    data->response.len = wr_len;
    memcpy(data->response.base, httpmsg, wr_len);
    data->response.base[wr_len] = '\0';

    uv_getaddrinfo(UV_LOOP, &client->req_addr, on_resolved, client->host, client->port, &AI_TCP);
    SAFE_CLOSE(async, NULL);
}

static void on_resolved(uv_getaddrinfo_t *req, int status, struct addrinfo *res)
{
    HttpData *data = (HttpData *)req->data;
    ClientData* client = (ClientData *)data->attachment;
    char addr[17] = {'\0'};

    uv_ip4_name((struct sockaddr_in *)res->ai_addr, addr, 16);
    uv_freeaddrinfo(res);

    struct sockaddr_in dest;
    uv_ip4_addr(addr, atoi(client->port), &dest);
    uv_tcp_init(UV_LOOP, &data->handle);

    data->handle.data = data;
    client->conn.data = data;

    uv_tcp_connect(
        &client->conn,
        &data->handle,
        (const struct sockaddr *)&dest,
        httpclient_on_connect);
}

static void httpclient_on_connect(uv_connect_t *connect_req, int status)
{
    HttpData *data = (HttpData *)connect_req->data;
    data->write.data = data;
    uv_write(&data->write, (uv_stream_t *)&data->handle, &data->response, 1, httpclient_write);
}

static void httpclient_write(uv_write_t *write_req, int status)
{
    HttpData *client = (HttpData *)write_req->data;
    uv_read_start((uv_stream_t *)&client->handle, buffer_allocator, httpclient_on_read);
}

static void httpclient_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
    http_parser parser;
    HttpData *data = (HttpData *)tcp->data;
    ClientData *client = (ClientData *)data->attachment;
    // ON NO ERROR
    if (nread >= 0)
    {
        http_parser_init(&parser, HTTP_RESPONSE);
        parser.data = data;
        ssize_t parsed = http_parser_execute(&parser, settings, buf->base, nread);
        data->attachment = client;
        // close handle on parse error
        if (parsed < nread)
        {
            printf("Parse Error [%ld < %ld]: closing handle\n",parsed,nread);
            SAFE_CLOSE(&data->handle, free_handle);
        }

        else{
            data->callback_args = Py_BuildValue("(NO)", data->dict, encapsulate(data)); // (N) does not increment refcount
            PyEval_CallObject(client->callback, data->callback_args);
            SAFE_CLOSE(&data->handle, free_handle);
        }
    }

    // ON ERROR
    else
    {
        // @TODO - debug error : if (nread != UV_EOF) {}
        // close handle
        println("Read Error!!");
        SAFE_CLOSE(&data->handle, free_handle);
    }

    free(buf->base); // free the read buffer
}
