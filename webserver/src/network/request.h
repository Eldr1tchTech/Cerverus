#pragma once

#include "network_types.inl"

typedef enum request_parse_state_type {
    request_parse_state_type_succeded,
    request_parse_state_type_unfinished,
    request_parse_state_type_invalid,
} request_parse_state_type;

typedef struct request_parse_state
{
    request_parse_state_type type;
    int bytes_consumed;
} request_parse_state;

// Creates a request struct with the given information. Handles allocation.
/**
 * @brief parses a request
 * 
 * @param raw_req the raw request
 * @param reqlen the length of the reqeust to parse
 * @param req the reqeust structure to populate
 * @return int -1 for missing headers, -2 for fatal error, otherwise, size of body
 */
request_parse_state request_parse(char *raw_req, ssize_t reqlen, request* req);

// Frees passed request and destroys pointer.
void request_destroy(request* req);