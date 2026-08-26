#include "route_trie.h"

#include "core/memory/cmem.h"

trie_node *trie_node_create() {
  trie_node *new_node = cmem_alloc(sizeof(trie_node));
  new_node->children = darray_create(2, sizeof(trie_node));
  new_node->segment.path_segment = nullptr;
  new_node->segment.is_dynamic = false;
  new_node->callback = nullptr;

  return new_node;
}

void trie_node_destroy(trie_node *t_node) {
  trie_node *darr_data = t_node->children->data;
  for (int i = 0; i < t_node->children->length; i++) {
    trie_node_destroy(&darr_data[i]);
  }

  darray_destroy(t_node->children);
  if (t_node->segment.path_segment) {
    string_destroy(t_node->segment.path_segment);
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

  route_segment *segments = rt->segments->data;
  for (int i = 0; i < rt->segments->length; i++) {
    trie_node *children = current->children->data;
    trie_node *next = nullptr;

    // Search existing children for a matching segment
    for (int j = 0; j < current->children->length; j++) {
      if (string_equal(segments[i].path_segment,
                       children[j].segment.path_segment)) {
        next = &children[j];
        break;
      }
    }

    // Not found — create and attach a new child node
    if (!next) {
      trie_node new_node = {0};
      new_node.segment.path_segment =
          string_duplicate(segments[i].path_segment);
      new_node.segment.is_dynamic = segments[i].is_dynamic;
      new_node.children = darray_create(2, sizeof(trie_node));
      new_node.callback = nullptr;
      darray_add(current->children, &new_node);
      // Re-fetch: darray_add may have resized, invalidating old pointer
      children = current->children->data;
      next = &children[current->children->length - 1];
    }

    current = next;
  }

  current->callback = rt->callback;
}

route_callback trie_find_handler(trie *t, http_method method, string URI) {
  darray *segment_darr = string_split_at_literal(URI, "/");

  trie_node *root = t->roots[method];

  // Find the final node
  route_segment *segment_darr_data = segment_darr->data;
  for (int i = 0; i < segment_darr->length; i++) {
    trie_node *children_darr_data = root->children->data;
    for (int j = 0; j < root->children->length; j++) {
      // Check for static match
      if (string_equal(segment_darr_data[i].path_segment,
                       children_darr_data[j].segment.path_segment)) {
        root = &children_darr_data[j];
        break;
      }

      // Check for dynamic "match" (not really)
      if (children_darr_data[j].segment.is_dynamic) {
        root = &children_darr_data[j];
        break;
      }

      if (j == root->children->length - 1) {
        // Not present
        for (int k = 0; k < segment_darr->length; k++) {
          cmem_free(segment_darr_data[k].path_segment);
        }
        darray_destroy(segment_darr);
        return nullptr;
      }
    }
  }
  for (int k = 0; k < segment_darr->length; k++) {
    cmem_free(segment_darr_data[k].path_segment);
  }
  darray_destroy(segment_darr);

  return root->callback;
}
