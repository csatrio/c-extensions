#pragma once
#include "uvmodule_common.h"

static PyObject* start_timer(PyObject *self, PyObject *args);
static PyObject *stop_timer(PyObject *self, PyObject *args);
static void on_timer (uv_timer_t* timer, int status);
static void on_work (uv_work_t* req);
static void on_after_work (uv_work_t* req, int status);

static void destroy_timer_capsule(PyObject* capsule){
    println("freeing timer structures");
    TimerTask *t = (TimerTask *)PyCapsule_GetPointer(capsule, TIMER_NAME);
    // Py_DECREF(capsule);
    // Py_DECREF(t->timer_cb);
    // Py_DECREF(t->task_param);
    free(t);
}

static PyObject* start_timer(PyObject *self, PyObject *args) {
    PyGILState_STATE state = PyGILState_Ensure();
    PyObject *timer_cb = NULL, *task_param = NULL;
    int start_delay = 0, every_ms = 0;
    if (!PyArg_ParseTuple(args, "OOii", &timer_cb, &task_param, &start_delay, &every_ms)) {
        EXCEPTION(PyExc_SyntaxError, "start_timer parameter must be : (timer_cb, task_param, start_delay, every_ms)");
        Py_RETURN_NONE;
    }
    CHECK_CALLABLE(timer_cb, "timer callback must be a callable function, e.g : def timer_cb(): ...");
    CHECK_TUPLE(task_param, "task_parameter must be a python tuple.");
    printf("Setting timer to start with delay %d ms, every %d ms\n", start_delay, every_ms);
    TimerTask* t = (TimerTask* )malloc(sizeof(TimerTask));
    t->timer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    t->timer_cb = timer_cb;
    t->task_param = task_param == Py_None ? NULL : task_param;
    t->timer->data = t;
    uv_timer_init(UV_LOOP, t->timer);
    uv_timer_start(t->timer, (uv_timer_cb)on_timer, start_delay, every_ms);
    PyGILState_Release(state);
    return PyCapsule_New(t, TIMER_NAME, destroy_timer_capsule);
}

static PyObject *stop_timer(PyObject *self, PyObject *args)
{
    //PyGILState_STATE state = PyGILState_Ensure();
    PyObject *capsule;
    if (!PyArg_ParseTuple(args, "O", &capsule))
    {
        EXCEPTION(PyExc_TypeError, "parameter is not a timer task");
        Py_RETURN_NONE;
    }
    println("stopping timer");
    TimerTask *t = (TimerTask *)PyCapsule_GetPointer(capsule, TIMER_NAME);
    uv_timer_stop(t->timer);
    //PyGILState_Release(state);
    Py_RETURN_NONE;
}

static PyObject* submit_task(PyObject *self, PyObject *args) {
    PyGILState_STATE state = PyGILState_Ensure();
    PyObject *task_cb, *task_complete_cb, *task_param;
    if (!PyArg_ParseTuple(args, "OOO", &task_cb, &task_complete_cb, &task_param)) {
        EXCEPTION(PyExc_SyntaxError, "task parameter must be : (task_cb, task_complete_cb, task_param)");
        Py_RETURN_NONE;
    }
    CHECK_CALLABLE(task_cb, "task_cb must be a callable function.")
    if(task_complete_cb != Py_None) CHECK_CALLABLE(task_complete_cb, "task_complete_cb must be a callable function.");
    if(task_param != Py_None) CHECK_TUPLE(task_param, "task_param must be a python tuple");
   
    AsyncTask* t = (AsyncTask* )malloc(sizeof(AsyncTask));
    t->task_cb = task_cb;
    t->task_complete_cb = task_complete_cb == Py_None ? NULL : task_complete_cb;
    t->task_param = task_param == Py_None ? NULL : task_param;
    t->task.data = t;
    uv_queue_work(UV_LOOP, &t->task, on_work, on_after_work);
    Py_RETURN_NONE;
    PyGILState_Release(state);
}

static void on_timer(uv_timer_t *timer, int status)
{
    //PyGILState_STATE state = PyGILState_Ensure();
    TimerTask *t = (TimerTask*) timer->data;
    PyEval_CallObject(t->timer_cb, t->task_param? t->task_param : EMPTY_ARGS);
    //PyGILState_Release(state);
}

// this is called by libuv threads
static void on_work (uv_work_t* req){
    //PyGILState_STATE state = PyGILState_Ensure();
    AsyncTask *t = (AsyncTask*) req->data;
    PyEval_CallObject(t->task_cb, t->task_param? t->task_param : EMPTY_ARGS);
    //PyGILState_Release(state);
}

// this one is where control is handed back to the loop thread
static void on_after_work (uv_work_t* req, int status){
    //PyGILState_STATE state = PyGILState_Ensure();
    AsyncTask *t = (AsyncTask*) req->data;
    if(t->task_complete_cb) PyEval_CallObject(t->task_complete_cb, t->task_param? t->task_param : EMPTY_ARGS);
    // Py_DECREF(t->task_cb);
    // Py_XDECREF(t->task_complete_cb);
    // Py_XDECREF(t->task_param);
    free(t);
    //PyGILState_Release(state);
}