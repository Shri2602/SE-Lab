#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "fruits.h"



#define ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))
#define TMP_CAP (8 * 1024)

typedef struct Node Node;

struct Node {
    bool end;

    Node *children[256];
};

#define NODE_POOL_CAP 1024
Node node_pool[NODE_POOL_CAP] = {0};
size_t node_pool_count = 0;

Node *alloc_node(void) {
    assert(node_pool_count < NODE_POOL_CAP);
    return &node_pool[node_pool_count++];
}

char tmp[TMP_CAP] = {0};
size_t tmp_size = 0;
char *tmp_end(void) {
    return tmp + tmp_size;
}

char *tmp_alloc(size_t size) {
    assert(tmp_size + size <= TMP_CAP);
    char *result = tmp_end();
    tmp_size += size;
    return result;
}

char *tmp_append_sized(const char *buffer, size_t buffer_sz) {
    char *result = tmp_alloc(buffer_sz);
    return memcpy(result, buffer, buffer_sz);
}

char *tmp_append_cstr(const char *cstr) {
    return tmp_append_sized(cstr, strlen(cstr));
}

void tmp_clean() {
    tmp_size = 0;
}

void tmp_rewind(char *end) {
    tmp_size = end - tmp;
}



void insert_text(Node *root, const char *text) {
    assert(root != NULL);

    if (text == NULL || *text == '\0') {
        root->end = true;
        return;
    }

    size_t index = (size_t)*text;

    if (root->children[index] == NULL) {
        root->children[index] = alloc_node();
    }

    insert_text(root->children[index], text + 1);
}

void usage(FILE *sink, const char *program) {
    fprintf(sink, "Usage: %s <SUBCOMMAND>\n", program);
    fprintf(sink, "SUBCOMMANDS:\n");
    fprintf(sink, "    complete <prefix>  Suggest prefix autocompletion based on the Trie.\n");
}

#define AC_BUFFER_CAP 1024
char ac_buffer[AC_BUFFER_CAP];
size_t ac_buffer_sz = 0;

void ac_buffer_push(char ch) {
    assert(ac_buffer_sz < AC_BUFFER_CAP);
    ac_buffer[ac_buffer_sz++] = ch;
}

void ac_buffer_pop(void) {
    assert(ac_buffer_sz > 0);
    ac_buffer_sz--;
}

Node *find_prefix(Node *root, const char *prefix) {
    if (prefix == NULL || *prefix == '\0') {
        return root;
    }

    if (root == NULL) {
        return NULL;
    }

    ac_buffer_push(*prefix);
    return find_prefix(root->children[(size_t)*prefix], prefix + 1);
}

void print_autocompletion(FILE *sink, Node *root) {
    if (root) {
        if (root->end) {
            fwrite(ac_buffer, ac_buffer_sz, 1, sink);
            fputc('\n', sink);
        }

        for (size_t i = 0; i < ARRAY_LEN(root->children); ++i) {
            if (root->children[i] != NULL) {
                ac_buffer_push((char)i);
                print_autocompletion(sink, root->children[i]);
                ac_buffer_pop();
            }
        }
    }
}

int main(int argc, char **argv) {
    const char *program = *argv++;

    if (*argv == NULL) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: no subcommand is provided\n");
        exit(1);
    }

    const char *subcommand = *argv++;

    Node *root = alloc_node();

    for (size_t i = 0; i < fruits_count; ++i) {
        insert_text(root, fruits[i]);
    }

    if (strcmp(subcommand, "complete") == 0) {
        if (*argv == NULL) {
            usage(stderr, program);
            fprintf(stderr, "ERROR: no prefix is provided\n");
            exit(1);
        }

        const char *prefix = *argv++;
        Node *subtree = find_prefix(root, prefix);
        if (subtree) {
            print_autocompletion(stdout, subtree);
        } else {
            fprintf(stderr, "No autocomplete suggestions found\n");
            exit(1);
        }
    } else {
        usage(stderr, program);
        fprintf(stderr, "ERROR: unknown subcommand `%s`\n", subcommand);
        exit(1);
    }

    return 0;
}
