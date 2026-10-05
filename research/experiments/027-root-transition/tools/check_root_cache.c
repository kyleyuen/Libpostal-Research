// Differential check of cached and ordinary phrase search on a real input split.
// usage: check_root_cache phrases.dat postal_codes.dat development.tsv [--negative]
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"
#include "trie_search.h"
#include "root_transition_cache.h"

static bool same_phrases(const phrase_array *a, const phrase_array *b) {
    size_t na = a ? a->n : 0, nb = b ? b->n : 0;
    if (na != nb) return false;
    for (size_t i = 0; i < na; i++) {
        if (a->a[i].start != b->a[i].start || a->a[i].len != b->a[i].len || a->a[i].data != b->a[i].data) return false;
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc != 4 && argc != 5) return 2;
    bool negative = argc == 5 && strcmp(argv[4], "--negative") == 0;
    if (argc == 5 && !negative) return 2;
    trie_t *tries[2] = {NULL, NULL};
    trie_root_cache_t caches[2];
    for (int t = 0; t < 2; t++) {
        FILE *model = fopen(argv[t + 1], "rb");
        if (!model) return 2;
        tries[t] = trie_read(model);
        fclose(model);
        if (!tries[t] || !trie_root_cache_build(tries[t], &caches[t])) return 2;
    }

    if (negative) caches[0].index[0] ^= 1;
    size_t table_mismatches = 0;
    for (int t = 0; t < 2; t++) {
        trie_node_t root = trie_get_node(tries[t], ROOT_NODE_ID);
        for (uint32_t b = 0; b < NUM_CHARS; b++) {
            uint32_t index = trie_get_transition_index(tries[t], root, (unsigned char)b);
            trie_node_t node = trie_get_node(tries[t], index);
            if (caches[t].source != tries[t] || caches[t].index[b] != index ||
                caches[t].node[b].base != node.base || caches[t].node[b].check != node.check) table_mismatches++;
        }
    }
    if (table_mismatches) {
        printf("root_entries=512 table_mismatches=%zu\n", table_mismatches);
        return 1;
    }

    FILE *input = fopen(argv[3], "r");
    if (!input) return 2;
    char *line = NULL; size_t cap = 0, inputs = 0, comparisons = 0, mismatches = 0;
    while (getline(&line, &cap, input) >= 0) {
        char *tab = strchr(line, '\t');
        if (!tab) { mismatches++; continue; }
        char *address = tab + 1;
        address[strcspn(address, "\r\n")] = '\0';
        token_array *tokens = tokenize(address);
        if (!tokens) return 2;
        inputs++;
        for (int t = 0; t < 2; t++) {
            phrase_array *ordinary = NULL, *cached = NULL;
            bool a = trie_search_tokens_with_phrases(tries[t], address, tokens, &ordinary);
            bool b = trie_search_tokens_with_phrases_cached(tries[t], address, tokens, &cached, &caches[t]);
            if (a != b || !same_phrases(ordinary, cached)) mismatches++;
            comparisons++;
            if (ordinary) phrase_array_destroy(ordinary);
            if (cached) phrase_array_destroy(cached);
        }
        token_array_destroy(tokens);
    }
    free(line); fclose(input);
    for (int t = 0; t < 2; t++) trie_destroy(tries[t]);
    printf("root_entries=512 table_mismatches=0 inputs=%zu searches=%zu output_mismatches=%zu\n", inputs, comparisons, mismatches);
    return mismatches ? 1 : 0;
}
