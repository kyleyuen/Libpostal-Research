#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "trie.h"

static void check(trie_t *t, const char *key, size_t len, int expected_found, uint32_t expected_data) {
    uint32_t sized = 0, nul = 0;
    char copy[256];
    assert(strlen(key) < sizeof(copy));
    strcpy(copy, key);
    int a = trie_get_data(t, copy, &nul);
    int b = trie_get_data_len(t, copy, len, &sized);
    assert(a == expected_found && b == expected_found);
    if (a) assert(nul == expected_data && sized == nul);
    printf("key_bytes=%zu supplied_len=%zu nul_found=%d length_found=%d value=%u strlen_scan_bytes_avoided=%zu\n",
           strlen(key), len, a, b, sized, len + 1);
}

int main(void) {
    trie_t *t = trie_new();
    assert(t);
    char a[] = "word|avenue", b[] = "word|avenues", c[] = "word|caf\xc3\xa9";
    assert(trie_add(t, a, 101)); assert(trie_add(t, b, 102)); assert(trie_add(t, c, 103));
    check(t, "word|avenue", 11, 1, 101);
    check(t, "word|avenues", 12, 1, 102);
    check(t, "word|caf\xc3\xa9", 10, 1, 103);
    check(t, "word|avenuex", 12, 0, 0);
    check(t, "word|cafe", 9, 0, 0);
    check(t, "word|aven", 9, 0, 0);
    trie_destroy(t);
    puts("length-aware lookup selfcheck passed: 3 hits, 3 misses");
    return 0;
}
