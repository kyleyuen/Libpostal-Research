#ifndef ROOT_TRANSITION_CACHE_H
#define ROOT_TRANSITION_CACHE_H

#include "trie_search.h"

// Parser-owned; valid only while the loaded source trie is immutable.
// Keep the index and node from the ordinary transition path, including misses.
typedef struct trie_root_cache {
    const trie_t *source;
    uint32_t index[NUM_CHARS];
    trie_node_t node[NUM_CHARS];
} trie_root_cache_t;

bool trie_root_cache_build(trie_t *self, trie_root_cache_t *cache);
bool trie_search_tokens_with_phrases_cached(trie_t *self, char *text, token_array *tokens, phrase_array **phrases, const trie_root_cache_t *cache);

#endif
