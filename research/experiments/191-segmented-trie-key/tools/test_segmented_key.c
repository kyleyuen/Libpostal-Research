#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "trie.h"

static void check(trie_t *t, const char *key, size_t split, uint32_t expected, int should_find) {
    uint32_t full = 0, seg = 0;
    char joined[256];
    size_t n = strlen(key);
    assert(n < sizeof(joined) && split <= n);
    memcpy(joined, key, n + 1);
    int full_found = trie_get_data(t, joined, &full);
    int seg_found = trie_get_data_concat(t, key, split, key + split, n - split, &seg);
    assert(full_found == should_find && seg_found == should_find);
    if (should_find) {
        assert(full == expected && seg == expected);
        assert(trie_get(t, joined) == trie_get(t, (char *)key));
    }
    uint32_t node_id = trie_get(t, joined);
    trie_node_t node = trie_get_node(t, node_id);
    size_t tail_bytes = node.base < 0 ? strlen((char *)(t->tail->a + trie_get_data_node(t, node).tail)) + 1 : 0;
    printf("%s split=%zu/%zu found=%d value=%u segments=2 full_key_bytes=%zu segmented_key_bytes=0 matched_tail_bytes=%zu\n",
           key, split, n, seg_found, seg, n + 1, tail_bytes);
}

int main(void) {
    trie_t *t = trie_new();
    assert(t);
    char k1[] = "word|avenue", k2[] = "word|avenues", k3[] = "word|aptitude";
    char k4[] = "word|caf\xc3\xa9";
    char long_key[190] = "word|";
    memset(long_key + 5, 'x', 180);
    long_key[185] = '\0';
    char empty_prefix_key[] = "plain";
    assert(trie_add(t, k1, 101));
    assert(trie_add(t, k2, 102));
    assert(trie_add(t, k3, 103));
    assert(trie_add(t, k4, 104));
    assert(trie_add(t, long_key, 105));
    assert(trie_add(t, empty_prefix_key, 106));
    check(t, "word|avenue", 5, 101, 1);       /* terminal prefix of a longer key */
    check(t, "word|avenues", 8, 102, 1);      /* split within shared compressed tail */
    check(t, "word|aptitude", 0, 103, 1);     /* empty prefix segment */
    check(t, "word|caf\xc3\xa9", 6, 104, 1);  /* UTF-8 bytes cross segment boundary */
    check(t, long_key, 5, 105, 1);             /* long compressed tail */
    check(t, "plain", 5, 106, 1);             /* empty suffix segment */
    check(t, "word|aven", 5, 0, 0);           /* prefix-only miss */
    check(t, "word|avenuex", 7, 0, 0);        /* mismatch inside tail */
    check(t, "word|avenuesx", 11, 0, 0);      /* mismatch after exact key */
    check(t, "word|missing", 0, 0, 0);        /* transition miss */
    check(t, "word|caf\xc3\xaa", 6, 0, 0);   /* one UTF-8 byte differs */
    trie_destroy(t);
    puts("segmented-key selfcheck passed: 6 hits, 5 misses");
    return 0;
}
