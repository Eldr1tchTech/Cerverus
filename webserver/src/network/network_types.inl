#pragma once

#include "defines.h"

#include "core/containers/darray.h"
#include "core/containers/string.h"

#define MAX_HEADER_COUNT 32

typedef enum http_method {
  http_method_get,
  http_method_head,
  http_method_options,
  http_method_trace,
  http_method_put,
  http_method_delete,
  http_method_post,
  http_method_patch,
  http_method_connect,
  http_method_unknown,
} http_method;

typedef enum http_version {
  http_version_1p1,
  http_version_unknown,
} http_version;

typedef struct header {
  string name;
  string value;
} header;

typedef struct request {
  struct {
    http_method method;
    string URI;
    http_version version;
  } request_line;
  darray *headers;
  struct {
    string data;
    size_t body_size;
  } body;
} request;

typedef struct response {
  struct {
    http_version version;
    int status_code;
    string reason_phrase;
  } status_line;
  struct {
    header headers[MAX_HEADER_COUNT];
    size_t header_count;
  } headers;
  struct {
    string data;
    size_t body_size;
  } body;
} response;

typedef void (*route_callback)(request *req, int client_fd);

typedef struct route_segment {
  string path_segment;
  bool is_dynamic;
} route_segment;

typedef struct route {
  darray *segments;
  http_method method;
  route_callback callback;
} route;

typedef struct trie_node {
  darray *children;
  route_segment segment;
  route_callback callback;
} trie_node;

typedef struct trie {
  trie_node **roots;
} trie;
