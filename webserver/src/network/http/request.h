#pragma once

#include "network/network_types.inl"

// Creates a request struct with the given information. Handles allocation.
/**
 * @brief parses a request
 * 
 * @param raw_req the raw request
 * @param reqlen the length of the reqeust to parse
 * @param req the reqeust structure to populate
 * @return int -1 for missing headers, 0 if unifinished, otherwise size consumed
 */
int request_parse(char *raw_req, size_t reqlen, request *req);

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