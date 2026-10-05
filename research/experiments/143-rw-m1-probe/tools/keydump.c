// 143 M1 probe (diagnostic, no parse-path change): dump every key of the model's tries as "escaped_key<TAB>data".
// Linked against the 142 `ctl` build of the ORIGINAL library. The enumerator below is copied verbatim from cpi's feature_dict.h
// (fd_for_each_key; exhaustively checked in 006/118). usage: keydump OUTDIR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"
#include "address_parser.h"
#include "address_dictionary.h"
#include "crf.h"
#include "trie.h"
typedef bool (*fd_key_cb)(void *ctx, const char *key, size_t len, uint32_t data);

// Enumerates every key the trie can return (each leaf's path + tail, exactly as trie_get_from_index reads them).
// Returns false on any structural doubt. Counts leaves seen and leaves that no lookup can reach.
static bool fd_for_each_key(trie_t *trie, fd_key_cb cb, void *ctx, uint64_t *leaves_visited, uint64_t *skipped) {
    uint32_t nn = (uint32_t)trie->nodes->n;
    trie_node_t *nodes = trie->nodes->a;
    trie_data_node_t *data = trie->data->a;
    uint8_t *tail = trie->tail->a; size_t tail_n = trie->tail->n;
    enum { MAXD = 512 };
    uint32_t stk_node[MAXD]; uint16_t stk_ci[MAXD]; uint8_t path[MAXD + 1];
    char *keybuf = malloc(1 << 16); if (!keybuf) return false;
    int sp = 0; stk_node[0] = ROOT_NODE_ID; stk_ci[0] = 1;
    if (nodes[ROOT_NODE_ID].base < 0) { free(keybuf); return false; }
    bool ok = true;
    *leaves_visited = 0; *skipped = 0;
    while (sp >= 0 && ok) {
        uint32_t pid = stk_node[sp]; int32_t pbase = nodes[pid].base;
        unsigned ci = stk_ci[sp]; bool descended = false;
        for (; ci <= 256; ci++) {
            uint64_t idx = (uint64_t)(uint32_t)pbase + ci;
            if (idx >= nn) break;
            if (nodes[idx].check != (int32_t)pid) continue;
            uint8_t c = trie->alphabet[ci - 1];
            path[sp] = c;
            if (nodes[idx].base < 0) {                               // leaf
                (*leaves_visited)++;
                int32_t di = -nodes[idx].base;
                if ((size_t)di >= trie->data->n) { ok = false; break; }
                uint32_t tpos = data[di].tail;
                if (tpos == 0 || tpos >= tail_n) { (*skipped)++; continue; }
                size_t tl = strnlen((char *)tail + tpos, tail_n - tpos);
                if (tpos + tl >= tail_n) { (*skipped)++; continue; }   // no NUL terminator inside the array
                size_t plen = (size_t)sp + 1;
                size_t klen;
                if (c == 0) { if (tl != 0) { (*skipped)++; continue; } klen = plen - 1; }   // ended on the NUL transition: lookup needs an empty tail
                else klen = plen + tl;
                if (klen + 1 > (1 << 16) || plen > MAXD) { ok = false; break; }
                memcpy(keybuf, path, c == 0 ? plen - 1 : plen);
                if (c != 0) memcpy(keybuf + plen, tail + tpos, tl);
                keybuf[klen] = 0;
                if (!cb(ctx, keybuf, klen, data[di].data)) { ok = false; break; }
            } else {
                if (sp + 1 >= MAXD) { ok = false; break; }
                stk_ci[sp] = (uint16_t)(ci + 1);
                sp++; stk_node[sp] = (uint32_t)idx; stk_ci[sp] = 1;
                descended = true; break;
            }
        }
        if (!ok) break;
        if (!descended) sp--;
    }
    free(keybuf);
    return ok;
}

typedef struct { FILE *f; uint64_t n; } dump_ctx_t;
static bool dump_cb(void *c, const char *key, size_t len, uint32_t data) {
    dump_ctx_t *d = c; d->n++;
    for (size_t i = 0; i < len; i++) { unsigned char ch = key[i];
        if (ch == '\\') fputs("\\\\", d->f); else if (ch == '\t') fputs("\\t", d->f); else if (ch == '\n') fputs("\\n", d->f); else if (ch == '\r') fputs("\\r", d->f); else if (ch == 0) fputs("\\0", d->f); else fputc(ch, d->f); }
    fprintf(d->f, "\t%u\n", data); return true;
}
static int dump(const char *dir, const char *name, trie_t *t) {
    char p[4096]; snprintf(p, sizeof p, "%s/%s.tsv", dir, name);
    dump_ctx_t d = {fopen(p, "w"), 0}; if (!d.f) return 1;
    uint64_t leaves = 0, skipped = 0; bool ok = fd_for_each_key(t, dump_cb, &d, &leaves, &skipped); fclose(d.f);
    printf("%s ok=%d keys=%llu num_keys(header)=%u leaves=%llu skipped=%llu\n", name, ok, (unsigned long long)d.n, t->num_keys, (unsigned long long)leaves, (unsigned long long)skipped);
    return ok ? 0 : 1;
}
int main(int argc, char **argv) {
    if (argc != 2 || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    address_parser_t *p = get_address_parser(); address_dictionary_t *ad = get_address_dictionary();
    printf("num_classes=%zu\n", p->num_classes);
    int rc = dump(argv[1], "state_features", p->model.crf->state_features) | dump(argv[1], "trans_features", p->model.crf->state_trans_features)
           | dump(argv[1], "vocab", p->vocab) | dump(argv[1], "component_phrases", p->phrases) | dump(argv[1], "postal_codes", p->postal_codes)
           | dump(argv[1], "address_dictionary", ad->trie);
    return rc;
}
