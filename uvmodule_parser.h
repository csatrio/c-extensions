#pragma once
#include "uvmodule_common.h"

static http_parser_settings *get_parser_settings(void);

// called after the url has been parsed.
static inline int on_url(http_parser *parser, const char *at, size_t length)
{
    HttpData *wrapper = (HttpData *)(parser->data);
    if (at && wrapper)
    {
        PyDict_SetItem(wrapper->dict, REQ_URL, PyUnicode_FromStringAndSize(at, length));
    }
    return 0;
};

// called when there are either fields or values in the request.
static inline int on_header_field(http_parser *parser, const char *at, size_t length)
{
    HttpData *wrapper = (HttpData *)(parser->data);
    wrapper->attachment = PyUnicode_FromStringAndSize(at, length);
    return 0;
};

// called when header value is given
static inline int on_header_value(http_parser *parser, const char *at, size_t length)
{
    HttpData *wrapper = (HttpData *)(parser->data);
    PyDict_SetItem(wrapper->dict, wrapper->attachment, PyUnicode_FromStringAndSize(at, length));
    return 0;
};

// called once all fields and values have been parsed.
static inline int on_headers_complete(http_parser *parser)
{
    HttpData *wrapper = (HttpData *)(parser->data);
    const char *method = http_method_str((enum http_method)parser->method);
    PyDict_SetItem(wrapper->dict, REQ_METHOD, Py_BuildValue("s", method));
    return 0;
};

// called when there is a body for the request.
static inline int on_body(http_parser *parser, const char *at, size_t length)
{
    HttpData *wrapper = (HttpData *)(parser->data);
    if (at && wrapper && (int)length > -1)
    {
        PyDict_SetItem(wrapper->dict, REQ_BODY, PyUnicode_FromStringAndSize(at, length));
    }
    return 0;
};

// called after all other events.
static inline int on_message_complete(http_parser *parser)
{
    HttpData *wrapper = (HttpData *)(parser->data);
    wrapper->attachment = NULL; //MAKE SURE THIS IS NULL, OTHERWISE IT LEADS TO CORRUPTION
    return 0;
};

static http_parser_settings *get_parser_settings(void)
{
    http_parser_settings *_settings = (http_parser_settings *)malloc(sizeof(http_parser_settings));
    http_parser_settings_init(_settings);
    _settings->on_url = on_url;
    _settings->on_header_field = on_header_field;
    _settings->on_header_value = on_header_value;
    _settings->on_headers_complete = on_headers_complete;
    _settings->on_body = on_body;
    _settings->on_message_complete = on_message_complete;
    return _settings;
}
