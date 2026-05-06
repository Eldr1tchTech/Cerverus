#include <network/http/request.h>
#include <network/server.h>
#include <network/http/response.h>
#include <core/memory/cmem.h>
#include <core/util/logger.h>
#include <core/util/util.h>
#include <network/routing/route.h>

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <math.h>
#include <fcntl.h>

#include <core/containers/hashmap.h>
#include <core/containers/doubly_linked_list.h>

int main()
{
    // DLL smoke test
    doubly_linked_list *dll = doubly_linked_list_create(sizeof(int));
    int a = 1, b = 2, c = 3;
    doubly_linked_list_push_back(dll, &a);
    doubly_linked_list_push_back(dll, &b);
    doubly_linked_list_push_front(dll, &c);
    LOG_INFO("DLL head=%d, tail=%d, length=%d", *(int *)dll->head->data, *(int *)dll->tail->data, dll->length);
    doubly_linked_list_destroy(dll);

    // Hashmap smoke test
    hashmap *hmap = hashmap_create(16, sizeof(int), NULL);
    int val = 42;
    hashmap_set(hmap, "hello", &val);
    int *result = hashmap_get(hmap, "hello");
    LOG_INFO("Hashmap get='%d'", result ? *result : -1);
    hashmap_destroy(hmap);

    // server_config s_conf = {
    //     .port = 8080,
    // };

    // server *s = server_create(&s_conf);

    // server_run(s);
}