// 159 offline (no model load): verify the "minimal tail trie" theory against the model's real trie structure and count hash entries.
// Theory: with keys sorted bytewise and compared as key+'\0', a prefix P is an explicit trie node iff >= 2 keys have prefix P (root always),
// so key K's leaf depth = max(LCP(K,prev), LCP(K,next)) + 1. Input: trie_struct's <trie>.struct.tsv (key, data, leaf_depth, data_index).
// usage: theory_check STRUCT_TSV
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef struct { const char *k; uint32_t len, data, depth; } key_t;
static size_t unesc(char *s) {
    char *r = s, *w = s;
    while (*r) { if (*r == '\\' && r[1]) { r++; *w++ = *r == 't' ? '\t' : *r == 'n' ? '\n' : *r == 'r' ? '\r' : *r == '0' ? 0 : *r; r++; } else *w++ = *r++; }
    return (size_t)(w - s);
}
static int cmpk(const void *a, const void *b) {
    const key_t *x = a, *y = b; uint32_t n = x->len < y->len ? x->len : y->len; int c = memcmp(x->k, y->k, n);
    return c ? c : (x->len < y->len ? -1 : x->len > y->len);
}
static uint32_t lcp(const key_t *a, const key_t *b) { uint32_t n = a->len < b->len ? a->len : b->len, i = 0; while (i < n && a->k[i] == b->k[i]) i++; return i; }
int main(int argc, char **argv) {
    FILE *f = fopen(argv[1], "r"); if (!f) { perror(argv[1]); return 2; }
    size_t cap = 1 << 20, n = 0; key_t *K = malloc(cap * sizeof *K); char *line = NULL; size_t lc = 0; ssize_t r; uint64_t bytes = 0, nul_in_key = 0;
    while ((r = getline(&line, &lc, f)) > 0) {
        if (line[r - 1] == '\n') line[--r] = 0;
        char *t3 = strrchr(line, '\t'); *t3 = 0; char *t2 = strrchr(line, '\t'); *t2 = 0; char *t1 = strrchr(line, '\t'); *t1 = 0;
        size_t kl = unesc(line); if (memchr(line, 0, kl)) nul_in_key++;
        if (n == cap) { cap *= 2; K = realloc(K, cap * sizeof *K); }
        char *k = malloc(kl + 1); memcpy(k, line, kl); k[kl] = 0;
        K[n].k = k; K[n].len = (uint32_t)kl; K[n].data = (uint32_t)strtoul(t1 + 1, NULL, 10); K[n].depth = (uint32_t)strtoul(t2 + 1, NULL, 10); n++; bytes += kl + 1;
    }
    qsort(K, n, sizeof *K, cmpk);
    uint32_t *L = malloc(n * sizeof *L);   // L[i] = LCP(K[i], K[i+1]) on key+'\0' (a proper prefix gives LCP = its length)
    uint64_t dup = 0; for (size_t i = 0; i + 1 < n; i++) { L[i] = lcp(&K[i], &K[i + 1]); if (L[i] == K[i].len && K[i].len == K[i + 1].len) dup++; }
    if (n) L[n - 1] = 0;
    uint64_t bad = 0, explicit_nodes = 1, e_term = 0, e_cont = 0, e_dead_sp_hi = 0, e_dead_other = 0, e_entries = 0, l_entries = 0, l_mid = 0, keys_term_explicit = 0, max_len = 0;
    enum { MAXD = 4096 }; static uint8_t ft[MAXD + 1], fc[MAXD + 1], fsh[MAXD + 1];
    uint32_t open = 0;   // explicit nodes at depths 1..open along the current key are open (depth 0 = root = search start, never an entry)
    for (size_t i = 0; i < n; i++) {
        uint32_t lp = i ? L[i - 1] : 0, ln = L[i], m = lp > ln ? lp : ln, pred = m + 1;
        if (pred != K[i].depth) { if (bad < 10) fprintf(stderr, "THEORY MISMATCH key#%zu len %u depth %u predicted %u\n", i, K[i].len, K[i].depth, pred); bad++; }
        if (K[i].len > max_len) max_len = K[i].len;
        while (open > lp) { uint32_t d = open--; e_term += ft[d]; e_cont += fc[d];   // close nodes not shared with this key
            if (!ft[d] && !fc[d]) { if (fsh[d]) e_dead_sp_hi++; else e_dead_other++; }
            if (ft[d] || fc[d] || fsh[d]) e_entries++; }
        while (open < m) { uint32_t d = ++open; explicit_nodes++; ft[d] = fc[d] = 0; uint8_t sh = 0;   // open new explicit nodes up to leaf depth - 1
            for (uint32_t j = 0; j < d && j < K[i].len; j++) { unsigned char c = (unsigned char)K[i].k[j]; if (c == ' ' || c >= 0x80) { sh = 1; break; } }
            fsh[d] = sh; }
        if (m >= K[i].len && K[i].len >= 1) { ft[K[i].len] = 1; keys_term_explicit++; }   // key ends on an explicit node ('\0' leaf)
        else if (K[i].len == 0) keys_term_explicit++;
        for (uint32_t d = 1; d <= m && d < K[i].len; d++) if (K[i].k[d] == ' ') fc[d] = 1;   // explicit node K[0..d) has a ' ' child
        // L entries: X = K[0..e) for e in [leaf depth, nsp], nsp = first index >= leaf depth with K[e]==' ', or len (none if the key ends on an explicit node)
        if (pred <= K[i].len) { uint32_t e = pred; while (e < K[i].len && K[i].k[e] != ' ') e++; l_entries += e - pred + 1; l_mid += e - pred; }
    }
    while (open > 0) { uint32_t d = open--; e_term += ft[d]; e_cont += fc[d]; if (!ft[d] && !fc[d]) { if (fsh[d]) e_dead_sp_hi++; else e_dead_other++; } if (ft[d] || fc[d] || fsh[d]) e_entries++; }
    uint64_t ent = e_entries + l_entries, slots = 1; while (slots * 3 < ent * 4) slots <<= 1;   // power-of-two table at load <= 0.75
    printf("%s: keys %zu dup_keys %llu nul_in_key %llu key_bytes(+NUL) %llu max_key_len %llu | theory mismatches %llu | explicit nodes (incl root) %llu\n",
           argv[1], n, (unsigned long long)dup, (unsigned long long)nul_in_key, (unsigned long long)bytes, (unsigned long long)max_len, (unsigned long long)bad, (unsigned long long)explicit_nodes);
    printf("  explicit: terminal %llu cont %llu dead-end with ' '/high byte %llu dead-end other (not stored) %llu -> E entries %llu | keys ending on explicit node %llu\n",
           (unsigned long long)e_term, (unsigned long long)e_cont, (unsigned long long)e_dead_sp_hi, (unsigned long long)e_dead_other, (unsigned long long)e_entries, (unsigned long long)keys_term_explicit);
    printf("  L entries %llu (midword %llu) | total entries %llu | slots %llu (load %.3f) -> 12-byte table %.1f MB, 16-byte table %.1f MB, keys %.1f MB, sorted offsets %.1f MB\n",
           (unsigned long long)l_entries, (unsigned long long)l_mid, (unsigned long long)ent, (unsigned long long)slots, (double)ent / slots, slots * 12 / 1e6, slots * 16 / 1e6, bytes / 1e6, n * 4 / 1e6);
    return bad ? 1 : 0;
}
