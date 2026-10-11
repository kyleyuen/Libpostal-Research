// 159 (diagnostic, model-derived, never timed): dump the STRUCTURE of the three phrase tries that context fill searches
// (component phrases, postal codes, address dictionary) so the hash design's flags can be derived/verified from the model:
//   per key: escaped_key <TAB> data <TAB> leaf_depth (path length to the leaf node, counting the '\0' transition if the key ends on it) <TAB> data_index
//   per trie (stdout): node array size, explicit/leaf/free counts, alphabet bijectivity, small-index node checks (garbage-read analysis).
// Walker = 143 keydump's fd_for_each_key (copied verbatim from cpi feature_dict.h) + leaf depth and node counting.
// usage: trie_struct OUTDIR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"
#include "address_parser.h"
#include "address_dictionary.h"
#include "trie.h"

static void esc(FILE *f, const char *key, size_t len) {
    for (size_t i = 0; i < len; i++) { unsigned char ch = key[i];
        if (ch == '\\') fputs("\\\\", f); else if (ch == '\t') fputs("\\t", f); else if (ch == '\n') fputs("\\n", f); else if (ch == '\r') fputs("\\r", f); else if (ch == 0) fputs("\\0", f); else fputc(ch, f); }
}
static int walk(const char *dir, const char *name, trie_t *trie) {
    char p[4096]; snprintf(p, sizeof p, "%s/%s.struct.tsv", dir, name);
    FILE *f = fopen(p, "w"); if (!f) return 1;
    uint32_t nn = (uint32_t)trie->nodes->n;
    trie_node_t *nodes = trie->nodes->a;
    trie_data_node_t *data = trie->data->a;
    uint8_t *tail = trie->tail->a; size_t tail_n = trie->tail->n;
    enum { MAXD = 512 };
    uint32_t stk_node[MAXD]; uint16_t stk_ci[MAXD]; uint8_t path[MAXD + 1];
    char *keybuf = malloc(1 << 16);
    uint64_t keys = 0, leaves = 0, skipped = 0, explicit_nodes = 1 /* root */, min_di = UINT64_MAX, small_di_leaves = 0, maxdepth = 0;
    int sp = 0; stk_node[0] = ROOT_NODE_ID; stk_ci[0] = 1; int ok = nodes[ROOT_NODE_ID].base >= 0;
    while (sp >= 0 && ok) {
        uint32_t pid = stk_node[sp]; int32_t pbase = nodes[pid].base;
        unsigned ci = stk_ci[sp]; int descended = 0;
        for (; ci <= 256; ci++) {
            uint64_t idx = (uint64_t)(uint32_t)pbase + ci;
            if (idx >= nn) break;
            if (nodes[idx].check != (int32_t)pid) continue;
            uint8_t c = trie->alphabet[ci - 1];
            path[sp] = c;
            if (nodes[idx].base < 0) {
                leaves++;
                int32_t di = -nodes[idx].base;
                if ((size_t)di >= trie->data->n) { ok = 0; break; }
                if ((uint64_t)di < min_di) min_di = di;
                if (di <= 64) small_di_leaves++;
                if (di <= 7) printf("%s small_di leaf: di=%d node=%llu char=%u depth=%d garbage_terminal_node[%d]=(%d,%d) match=%d\n", name, di, (unsigned long long)idx, c, sp + 1, 8 - di, nodes[8 - di].base, nodes[8 - di].check, nodes[8 - di].check == (int32_t)idx);
                uint32_t tpos = data[di].tail;
                if (tpos == 0 || tpos >= tail_n) { skipped++; continue; }
                size_t tl = strnlen((char *)tail + tpos, tail_n - tpos);
                if (tpos + tl >= tail_n) { skipped++; continue; }
                size_t plen = (size_t)sp + 1, klen;
                if (c == 0) { if (tl != 0) { skipped++; continue; } klen = plen - 1; } else klen = plen + tl;
                memcpy(keybuf, path, c == 0 ? plen - 1 : plen);
                if (c != 0) memcpy(keybuf + plen, tail + tpos, tl);
                esc(f, keybuf, klen); fprintf(f, "\t%u\t%zu\t%d\n", data[di].data, plen, di);
                if (plen > maxdepth) maxdepth = plen;
                keys++;
            } else {
                if (sp + 1 >= MAXD) { ok = 0; break; }
                explicit_nodes++;
                stk_ci[sp] = (uint16_t)(ci + 1);
                sp++; stk_node[sp] = (uint32_t)idx; stk_ci[sp] = 1;
                descended = 1; break;
            }
        }
        if (!ok) break;
        if (!descended) sp--;
    }
    fclose(f); free(keybuf);
    uint64_t used = 0, freen = 0;
    for (uint32_t i = ROOT_NODE_ID + 1; i < nn; i++) { if (nodes[i].check > 0) used++; else if (nodes[i].check < 0) freen++; }
    int bij = trie->alphabet_size == 256; unsigned char seen[256] = {0};
    for (uint32_t i = 0; i < trie->alphabet_size; i++) { unsigned char a = (unsigned char)trie->alphabet[i]; if (seen[a]++) bij = 0; if (trie->alpha_map[a] != i) bij = 0; }
    printf("%s ok=%d keys=%llu num_keys(header)=%u leaves=%llu skipped=%llu explicit_nodes(incl root)=%llu nodes_n=%u nodes_with_check>0(excl root)=%llu free(check<0)=%llu "
           "alphabet_size=%u bijective=%d char_index(' ')=%u char_index('\\0')=%u min_data_index=%llu leaves_with_data_index<=64=%llu max_leaf_depth=%llu tail_n=%zu data_n=%zu\n",
           name, ok, (unsigned long long)keys, trie->num_keys, (unsigned long long)leaves, (unsigned long long)skipped, (unsigned long long)explicit_nodes, nn,
           (unsigned long long)used, (unsigned long long)freen, trie->alphabet_size, bij, trie_get_char_index(trie, ' '), trie_get_char_index(trie, 0),
           (unsigned long long)min_di, (unsigned long long)small_di_leaves, (unsigned long long)maxdepth, tail_n, trie->data->n);
    printf("%s nodes[0..15] (base,check):", name);
    for (uint32_t i = 0; i < 16 && i < nn; i++) printf(" %u:(%d,%d)", i, nodes[i].base, nodes[i].check);
    printf("\n");
    return ok ? 0 : 1;
}
int main(int argc, char **argv) {
    if (argc != 2 || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    address_parser_t *p = get_address_parser(); address_dictionary_t *ad = get_address_dictionary();
    int rc = walk(argv[1], "component_phrases", p->phrases) | walk(argv[1], "postal_codes", p->postal_codes) | walk(argv[1], "address_dictionary", ad->trie);
    return rc;
}
