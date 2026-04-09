#pragma once

#include "network/network_types.inl"

typedef enum request_parse_state {
    request_parse_state_succeded,
    request_parse_state_unfinished,
    request_parse_state_invalid,
} request_parse_state;

typedef struct request_parse_state_context
{
    request_parse_state type;
    int bytes_consumed;
} request_parse_state_context;

// Creates a request struct with the given information. Handles allocation.
/**
 * @brief parses a request
 * 
 * @param raw_req the raw request
 * @param reqlen the length of the reqeust to parse
 * @param req the reqeust structure to populate
 * @return int -1 for missing headers, -2 for fatal error, otherwise, size of body
 */
request_parse_state_context request_parse(char *raw_req, size_t reqlen, request **req);

// Frees passed request and destroys pointer.
void request_destroy(request* req);

/**
 * @brief Returns the value of a header, or null if not present.
 * 
 * @param req 
 * @param header_name 
 * @return char* 
 */
char* request_get_header_value(request* req, char* header_name);