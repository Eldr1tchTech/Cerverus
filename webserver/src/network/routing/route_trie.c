#include "route_trie.h"

#include "core/memory/cmem.h"
#include "core/util/util.h"

trie_node *trie_node_create() {
  trie_node *new_node = cmem_alloc(sizeof(trie_node));
  new_node->children = darray_create(2, sizeof(trie_node));
  new_node->segment.path_segment = nullptr;
  new_node->segment.is_dynamic = false;
  new_node->callback = nullptr;

  return new_node;
}

void trie_node_destroy(trie_node *t_node) {
  for (int i = 0; i < *darray_get_length(t_node->children); i++) {
    trie_node_destroy(&t_node->children[i]);
  }

  darray_destroy(t_node->children);
  if (t_node->segment.path_segment) {
    str_destroy(t_node->segment.path_segment);
  }
  cmem_free(t_node);
}

trie *trie_create() {
  trie *new_trie = cmem_alloc(sizeof(trie));
  new_trie->roots = cmem_alloc(http_method_unknown * sizeof(trie_node *));
  for (int i = 0; i < http_method_unknown; i++) {
    new_trie->roots[i] = trie_node_create();
  }

  return new_trie;
}

void trie_destroy(trie *t) {
  for (int i = 0; i < http_method_unknown; i++) {
    trie_node_destroy(t->roots[i]);
  }
  cmem_free(t->roots);
  cmem_free(t);
}

// NOTE: For now it's just the first match, eventually it should be best match
void trie_add_route(trie *t, route *rt) {
  trie_node *current = t->roots[rt->method];

  for (int i = 0; i < *darray_get_length(rt->segments); i++) {
    trie_node *children = current->children;
    trie_node *next = nullptr;

    // Search existing children for a matching segment
    for (int j = 0; j < *darray_get_length(current->children); j++) {
      if (str_equal(rt->segments[i].path_segment,
                    children[j].segment.path_segment)) {
        next = &children[j];
        break;
      }
    }

    // Not found — create and attach a new child node
    if (!next) {
      trie_node new_node = {0};
      new_node.segment.path_segment = str_dup(rt->segments[i].path_segment);
      new_node.segment.is_dynamic = rt->segments[i].is_dynamic;
      new_node.children = darray_create(2, sizeof(trie_node));
      new_node.callback = nullptr;
      children = darray_add(current->children, &new_node);
      next = &children[*darray_get_length(current->children) - 1];
    }

    current = next;
  }

  current->callback = rt->callback;
}

route_callback trie_find_handler(trie *t, http_method method, string URI) {
  string *segment_darr = str_split_at_lit(URI, "/");

  trie_node *root = t->roots[method];

  // Find the final node
  for (int i = 0; i < *darray_get_length(segment_darr); i++) {
    for (int j = 0; j < *darray_get_length(root->children); j++) {
      // Check for static match
      if (str_equal(segment_darr[i], root->children[j].segment.path_segment)) {
        root = &root->children[j];
        break;
      }

      // Check for dynamic "match" (not really)
      if (root->children[j].segment.is_dynamic) {
        root = &root->children[j];
        break;
      }

      if (j == *darray_get_length(root->children) - 1) {
        // Not present
        darray_destroy_string_helper(segment_darr);
        return nullptr;
      }
    }
  }
  darray_destroy_string_helper(segment_darr);

  return root->callback;
}
