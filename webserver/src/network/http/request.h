#pragma once

#include "network/network_types.inl"

// Creates a request struct with the given information. Handles allocation.
/**
 * @brief parses a request
 * @param req the reqeust structure to populate
 * @return int -1 for missing headers, 0 if unifinished, otherwise size consumed
 */
int request_parse(request_t *req);

// Frees passed request and destroys pointer.
void request_destroy(request_t *req);

/**
 * @brief Returns the value of a header, or null if not present.
 *
 * @param req
 * @param hdr_name
 * @return sv_t
 */
sv_t request_get_header_value(request_t *req, char *hdr_name);