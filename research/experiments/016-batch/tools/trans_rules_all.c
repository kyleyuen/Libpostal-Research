// Experiment 016 T4 (copied from 011 trans_rules.c; prints every rule source in hex): enumerate the rules of the "latin-ascii-simple" transliterator (its steps, then every trie key under each ruleset namespace)
// and report which rules could apply to a pure-ASCII string.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libpostal.h"
#include "transliterate.h"
#include "trie.h"
static trie_t *T; static const char *prefix; static size_t plen;
static uint64_t total, under, ascii_only, printed; static int first_bytes[256], any_first[256];
static void emit(const uint8_t *k, size_t n) {
    total++;
    if (n < plen || memcmp(k, prefix, plen) != 0) return;
    under++; const uint8_t *src = k + plen; size_t sl = n - plen;
    int ascii = 1; 
    if (sl) any_first[src[0]]++;
    if (ascii) { ascii_only++; first_bytes[src[0]]++;
        if (1) { printf("  rule source ["); for (size_t i = 0; i < sl; i++) printf("%02x ", src[i]); printf("]\n"); } }
}
static void dfs(uint32_t pid, uint8_t *path, int depth) {
    trie_node_t *nodes = T->nodes->a; uint32_t nn = T->nodes->n;
    for (unsigned ci = 1; ci <= T->alphabet_size; ci++) {
        uint64_t idx = (uint64_t)(uint32_t)nodes[pid].base + ci; if (idx >= nn) break;
        if (nodes[idx].check != (int32_t)pid) continue;
        uint8_t c = (uint8_t)T->alphabet[ci - 1]; path[depth] = c;
        if (nodes[idx].base < 0) {
            int32_t di = -nodes[idx].base; uint32_t tp = T->data->a[di].tail; uint8_t buf[1024]; size_t n = depth + 1; memcpy(buf, path, n);
            if (c == 0) n--; else { size_t tl = strnlen((char *)T->tail->a + tp, 900); memcpy(buf + n, T->tail->a + tp, tl); n += tl; }
            emit(buf, n);
        } else if (depth < 500) dfs((uint32_t)idx, path, depth + 1);
    }
}
static transliteration_table_t *TT;
static void scan_transliterator(const char *name, int level) {
    khiter_t k = kh_get(str_transliterator, TT->transliterators, (char *)name);
    if (k == kh_end(TT->transliterators)) { printf("%*s(transliterator %s not found)\n", level * 2, "", name); return; }
    transliterator_t *tr = kh_value(TT->transliterators, k);
    printf("%*stransliterator %s: %zu steps\n", level * 2, "", tr->name, tr->steps_length);
    static uint8_t path[600]; char pref[512];
    for (size_t i = tr->steps_index; i < tr->steps_index + tr->steps_length; i++) {
        transliteration_step_t *s = TT->steps->a[i];
        printf("%*sstep %zu: name=%s type=%s\n", level * 2 + 1, "", i, s->name, s->type == STEP_RULESET ? "ruleset" : s->type == STEP_TRANSFORM ? "transform" : "unicode-normalization");
        if (s->type == STEP_TRANSFORM) { scan_transliterator(s->name, level + 1); continue; }
        if (s->type != STEP_RULESET) continue;
        snprintf(pref, sizeof pref, "%s%s%s%s", tr->name, NAMESPACE_SEPARATOR_CHAR, s->name, NAMESPACE_SEPARATOR_CHAR); prefix = pref; plen = strlen(pref);
        total = under = ascii_only = printed = 0; memset(first_bytes, 0, sizeof first_bytes); memset(any_first, 0, sizeof any_first);
        dfs(ROOT_NODE_ID, path, 0);
        printf("%*sruleset %s|%s: keys scanned=%llu under-namespace=%llu ASCII-only rule sources=%llu\n", level * 2 + 2, "", tr->name, s->name, (unsigned long long)total, (unsigned long long)under, (unsigned long long)ascii_only);
        printf("%*sfirst bytes of ASCII-only sources:", level * 2 + 2, ""); for (int b = 0; b < 256; b++) if (first_bytes[b]) printf(" %s%c(%d)", b < 32 || b == 127 ? "\\x" : "", b < 32 || b == 127 ? "0123456789abcdef"[b >> 4] : b, first_bytes[b]); printf("\n");
        int nd = 0; for (int b = 0; b < 256; b++) if (any_first[b]) nd++;
        printf("%*sdistinct first bytes over ALL sources (incl. non-ASCII): %d :", level * 2 + 2, "", nd); for (int b = 0; b < 256; b++) if (any_first[b]) printf(" 0x%02x(%d)", b, any_first[b]); printf("\n");
    }
}
int main(void) {
    if (!libpostal_setup()) return 2;
    TT = get_transliteration_table(); T = TT->trie;
    printf("transliteration trie: nodes=%zu keys(header)=%u alphabet=%u\n", (size_t)T->nodes->n, T->num_keys, T->alphabet_size);
    scan_transliterator(LATIN_ASCII_SIMPLE, 0);
    return 0;
}
