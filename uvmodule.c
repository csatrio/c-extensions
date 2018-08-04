#include "uvmodule_http.h"
#include "uvmodule_timertask.h"

// METHOD PROTOTYPES
static void on_shutdown(void);

// Python Callback Function
static PyObject *set_callback(PyObject *dummy, PyObject *args)
{
    PyObject *temp = NULL;

    if (PyArg_ParseTuple(args, "O:set_callback", &temp)) {
        CHECK_CALLABLE(temp, "server callback must be a callable function, e.g : def server_cb(req, handle): ...");
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(httpserver_callback);  /* Dispose of previous callback */
        httpserver_callback = temp;       /* Remember new callback */
        println("setting callback...");
    }
    if(httpserver_callback == NULL) println("callback still null !!");
    Py_RETURN_NONE;
}
// End Python Callback Function


static PyObject* set_async(PyObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "i", &ASYNC)) {
        EXCEPTION(PyExc_TypeError, "set_async argument must be an integer !!");
    }
    printf("server write response in %s mode!\n", ASYNC? "asynchronous(deferred)" : "synchronous");
    Py_RETURN_NONE;
}

static PyObject* set_buffer_size(PyObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, "i", &BUFFER_SIZE)) {
        EXCEPTION(PyExc_TypeError, "set_buffer_size argument must be an integer !!");
    }
    printf("http server new buffer size : %d byte(s)\n", BUFFER_SIZE);
    Py_RETURN_NONE;
}

static PyObject *listen_request(PyObject *self, PyObject *args)
{
    int _port, status;
    const char *_ip;
    struct sockaddr_in address;

    if (PyArg_ParseTuple(args, "si", &_ip, &_port))
    {
        ip = _ip;
        port = _port;
        printf("Starting libuv http server at host %s : %d\n", ip, port);
    }
    EMPTY_ARGS = Py_BuildValue("()");
    REQ_URL = Py_BuildValue("s", "url");
    REQ_BODY = Py_BuildValue("s", "body");
    REQ_METHOD = Py_BuildValue("s", "method");
    settings = get_parser_settings();

    // initialize tcp address info
    INIT_AI_TCP(AI_TCP);

    uv_tcp_init(UV_LOOP, &socket_);

    status = uv_ip4_addr(ip, port, &address);
    ASSERT_STATUS(status, "Resolve Address");

    status = uv_tcp_bind(&socket_, (const struct sockaddr *)&address, 0);
    ASSERT_STATUS(status, "Bind");

    status = uv_listen((uv_stream_t *)&socket_, MAX_WRITES, on_connect);
    ASSERT_STATUS(status, "Listen");

    uv_run(UV_LOOP, UV_RUN_DEFAULT);
    Py_RETURN_NONE;
}

static inline PyObject *send_response(PyObject *self, PyObject *args)
{
    const char *_response;
    PyObject *capsule;
    if (PyArg_ParseTuple(args, "sO", &_response, &capsule))
    {
        HttpData *data = (HttpData *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        size_t wr_len = strlen(_response);
        data->response.base = (char*) malloc(wr_len + 1);
        memcpy(data->response.base, _response, wr_len);
        data->response.len = wr_len;
        data->response.base[wr_len] = '\0';
        if (ASYNC)
            uv_async_send(data->async);
    }
    else
    {
        EXCEPTION(PyExc_SyntaxError, "Invalid Response Argument : Correct one is (response, handle)");
    }
    Py_RETURN_NONE;
}

static inline PyObject *send_request(PyObject *self, PyObject *args)
{
    PyObject* callback;
    const char *httpmsg, *host, *port;
    if (PyArg_ParseTuple(args, "sssO", &httpmsg, &host, &port, &callback))
    {
        create_client(httpmsg, host, port, callback);
    }
    else
    {
        EXCEPTION(PyExc_SyntaxError, "Invalid Request Argument : Correct one is (httpmsg, host, port, callback)");
    }
    Py_RETURN_NONE;
}

static void on_shutdown(void)
{
    printf("Shutting down uvmodule at %s : %d\n", ip, port);
    uv_stop(UV_LOOP);
}

// Method definition object for this extension, these argumens mean:
// ml_name: The name of the method
// ml_meth: Function pointer to the method implementation
// ml_flags: Flags indicating special features of this method, such as
//          accepting arguments, accepting keyword arguments, being a
//          class method, or being a static method of a class.
// ml_doc:  Contents of this method's docstring
static PyMethodDef uv_methods[] = { 
    {   
        "listen_request", listen_request, METH_VARARGS,
        "Run libuv from a method defined in a C extension."
    },
    {   
        "set_callback", set_callback, METH_VARARGS,
        "Set callback from a method defined in a C extension."
    },
    {   
        "send_response", send_response, METH_VARARGS,
        "Send Http Response from a method defined in a C extension."
    },
    {   
        "send_request", send_request, METH_VARARGS,
        "Send Http Response from a method defined in a C extension."
    },
    {   
        "set_async", set_async, METH_VARARGS,
        "Set server deferred write capability mode."
    },
    {   
        "set_buffer_size", set_buffer_size, METH_VARARGS,
        "Set server request buffer size."
    },
    {   
        "start_timer", start_timer, METH_VARARGS,
        "Start a timer that will be run by event loop."
    },
    {   
        "stop_timer", stop_timer, METH_VARARGS,
        "Stop a timer."
    },
    {   
        "submit_task", submit_task, METH_VARARGS,
        "Submit asynchronous task to be run by libuv thread pool, this bypass the GIL."
    },
    {NULL, NULL, 0, NULL}
};

// Module definition
// The arguments of this structure tell Python what to call your extension,
// what it's methods are and where to look for it's method definitions
static struct PyModuleDef uv_definition = { 
    PyModuleDef_HEAD_INIT,
    "uvmodule",
    "A Python module that use LibUv from C code.",
    -1, 
    uv_methods
};

// Module initialization
// Python calls this function when importing your extension. It is important
// that this function is named PyInit_[[your_module_name]] exactly, and matches
// the name keyword argument in setup.py's setup() call.
PyMODINIT_FUNC PyInit_uvmodule(void) {
    Py_Initialize();
    Py_AtExit(on_shutdown);
    UV_LOOP = uv_loop_new();
    return PyModule_Create(&uv_definition);
}