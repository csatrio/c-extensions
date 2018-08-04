#include <stdio.h>
#include <Python.h>

#define println(msg){printf("%s\n", msg);}

// Module method definitions
static PyObject* hello_world(PyObject *self, PyObject *args) {
    printf("Hello, world!\n");
    Py_RETURN_NONE;
}

static PyObject* hello(PyObject *self, PyObject *args) {
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    printf("Hello, %s!\n", name);
    Py_RETURN_NONE;
}


// Python Callback Function
static PyObject *my_callback = NULL;

static PyObject *set_callback(PyObject *dummy, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp = NULL;

    if (PyArg_ParseTuple(args, "O:set_callback", &temp)) {
        if (!PyCallable_Check(temp)) {
            println("Callback method is non-callable");
            //PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            Py_RETURN_NONE;
        }
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(my_callback);  /* Dispose of previous callback */
        my_callback = temp;       /* Remember new callback */
        println("setting callback...");
    }
    if(my_callback == NULL) println("callback still null !!");
    Py_RETURN_NONE;
}

static PyObject *call_cb(PyObject *self, PyObject *args)
{
    int arg;
    PyObject *arglist;
    PyObject *result;
    arg = 123;
    /* Time to call the callback */
    arglist = Py_BuildValue("(i)", arg);
    result = PyObject_CallObject(my_callback, arglist);
    Py_DECREF(arglist);
    return result;
}

// End Python Callback Function

static PyObject* set_dict(PyObject *self, PyObject *_dict) {
    PyObject* dict = NULL;
    if (! PyArg_ParseTuple(_dict, "O!", &PyDict_Type, &dict)) return NULL;
    PyDict_SetItem(dict, Py_BuildValue("s","response"), Py_BuildValue("s","success")); 
    Py_RETURN_NONE;
}




// Method definition object for this extension, these argumens mean:
// ml_name: The name of the method
// ml_meth: Function pointer to the method implementation
// ml_flags: Flags indicating special features of this method, such as
//          accepting arguments, accepting keyword arguments, being a
//          class method, or being a static method of a class.
// ml_doc:  Contents of this method's docstring
static PyMethodDef hello_methods[] = { 
    {   
        "hello_world", hello_world, METH_NOARGS,
        "Print 'hello world' from a method defined in a C extension."
    },  
    {   
        "hello", hello, METH_VARARGS,
        "Print 'hello xxx' from a method defined in a C extension."
    }, 
    {   
        "call_cb", call_cb, METH_NOARGS,
        "Call saved callback from a method defined in a C extension."
    },
    {   
        "set_callback", set_callback, METH_VARARGS,
        "Set callback from a method defined in a C extension."
    },  
    {   
        "set_dict", set_dict, METH_VARARGS,
        "Set dict from a method defined in a C extension."
    },
    {NULL, NULL, 0, NULL}
};

// Module definition
// The arguments of this structure tell Python what to call your extension,
// what it's methods are and where to look for it's method definitions
static struct PyModuleDef hello_definition = { 
    PyModuleDef_HEAD_INIT,
    "hello",
    "A Python module that prints 'hello world' from C code.",
    -1, 
    hello_methods
};

// Module initialization
// Python calls this function when importing your extension. It is important
// that this function is named PyInit_[[your_module_name]] exactly, and matches
// the name keyword argument in setup.py's setup() call.
PyMODINIT_FUNC PyInit_hello(void) {
    Py_Initialize();
    return PyModule_Create(&hello_definition);
}