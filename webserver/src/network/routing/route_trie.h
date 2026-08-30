#pragma once

#include "network/IO/async_io.h"
#include "network/network_types.inl"

#include "network/server.h"

trie *trie_create();
void trie_destroy(trie *t);

void trie_add_route(trie *t, route *rt);
async_resume_fn trie_find_handler(trie *t, http_method method, string URI);