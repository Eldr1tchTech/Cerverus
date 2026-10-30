#pragma once

#include "network/network_types.inl"

trie *trie_create();
void trie_destroy(trie *t);

void trie_add_route(trie *t, route_t *rt);
pt_fn trie_find_handler(trie *t, http_method method, sds URI);