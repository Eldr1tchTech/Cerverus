#pragma once

#include "core/containers/sv.h"
#include "core/util/protothread.h"
#include "core/vendor/sds.h"

#define MAX_HEADER_COUNT 32

// Use string literals for performance boosts
#define HTTP_VERSION_1P1_LITERAL "HTTP/1.1"

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
  http_method_unknown, // NOTE: doubles as number of root trie_node's
} http_method;

typedef enum http_version {
  http_version_1p1,
  http_version_unknown,
} http_version;

typedef struct header_t {
  union {
    struct {
      sv_t name;
      sv_t value;
    } sv;
    struct {
      sds name;
      sds value;
    } sds;
  };
} header_t;

typedef struct request_t {
  sds _raw;
  struct {
    http_method method;
    sv_t URI;
    http_version version;
  } status_line;
  header_t *headers;
  sv_t body;
} request_t;

typedef struct response_t {
  struct {
    http_version version;
    int status_code;
    sds reason_phrase;
  } status_line;
  header_t *headers;
  sds body;
} response_t;

typedef struct route_segment_t {
  sds path_segment;
  bool is_dynamic;
} route_segment_t;

typedef struct route_t {
  route_segment_t *segments_darr;
  http_method method;
  pt_fn callback;
} route_t;

typedef struct trie_node_t {
  struct trie_node_t *children;
  route_segment_t segment;
  pt_fn callback;
} trie_node_t;

typedef struct trie_t {
  trie_node_t **roots;
} trie;

typedef struct minimal_locals {
  int client_fd;
  request_t *req;
} minimal_locals;