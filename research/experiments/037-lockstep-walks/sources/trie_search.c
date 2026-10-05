#include "trie_search.h"

typedef enum {
    SEARCH_STATE_BEGIN,
    SEARCH_STATE_NO_MATCH,
    SEARCH_STATE_PARTIAL_MATCH,
    SEARCH_STATE_MATCH
} trie_search_state_t;

bool trie_search_from_index(trie_t *self, char *text, uint32_t start_node_id, phrase_array **phrases) {
    if (text == NULL) return false;

    ssize_t len, remaining;
    int32_t unich = 0;
    unsigned char ch = '\0';

    const uint8_t *ptr = (const uint8_t *)text;
    const uint8_t *fail_ptr = ptr;

    uint32_t node_id = start_node_id;
    trie_node_t node = trie_get_node(self, node_id), last_node = node;
    uint32_t next_id;

    bool match = false;
    uint32_t index = 0;
    uint32_t phrase_len = 0;
    uint32_t phrase_start = 0;
    uint32_t data;

    trie_search_state_t state = SEARCH_STATE_BEGIN, last_state = SEARCH_STATE_BEGIN;

    bool advance_index = true;

    while(1) {
        len = utf8proc_iterate(ptr, -1, &unich);
        remaining = len;
        if (len <= 0) return false;
        if (!(utf8proc_codepoint_valid(unich))) return false;

        int cat = utf8proc_category(unich);
        bool is_letter = utf8_is_letter(cat);

        // If we're in the middle of a word and the first letter was not a match, skip the word
        if (is_letter && state == SEARCH_STATE_NO_MATCH) { 
            log_debug("skipping\n");
            ptr += len;
            index += len;
            last_state = state;
            continue; 
        }

        // Match in the middle of a word
        if (is_letter && last_state == SEARCH_STATE_MATCH) {
            log_debug("last_state == SEARCH_STATE_MATCH && is_letter\n");
            // Only set match to false so we don't callback
            match = false;
        }

        for (int i=0; remaining > 0; remaining--, i++, ptr++, last_node=node, last_state=state, node_id=next_id) {
            ch = (unsigned char) *ptr;
            log_debug("char=%c\n", ch);

            next_id = trie_get_transition_index(self, node, *ptr);
            node = trie_get_node(self, next_id);

            if (node.check != node_id) {
                state = is_letter ? SEARCH_STATE_NO_MATCH : SEARCH_STATE_BEGIN;
                if (match) {
                    log_debug("match is true and state==SEARCH_STATE_NO_MATCH\n");
                    if (*phrases == NULL) {
                        *phrases = phrase_array_new_size(1);
                    }
                    phrase_array_push(*phrases, (phrase_t){phrase_start, phrase_len, data});
                    index = phrase_start + phrase_len;
                    advance_index = false;
                    // Set the text back to the end of the last phrase
                    ptr = (const uint8_t *)text + index;
                    len = utf8proc_iterate(ptr, -1, &unich);
                    log_debug("ptr=%s\n", ptr);
                } else {
                    ptr += remaining;
                    log_debug("done with char, now at %s\n", ptr);
                }
                fail_ptr = ptr;
                node_id = start_node_id;
                last_node = node = trie_get_node(self, node_id);
                phrase_start = phrase_len = 0;
                last_state = state;
                match = false;
                break;
            } else {
                log_debug("node.check == node_id\n");
                state = SEARCH_STATE_PARTIAL_MATCH;
                if (last_state == SEARCH_STATE_NO_MATCH || last_state == SEARCH_STATE_BEGIN) {
                    log_debug("phrase_start=%u\n", index);
                    phrase_start = index;
                    fail_ptr = ptr + remaining;
                }

                if (node.base < 0) {
                    int32_t data_index = -1*node.base;
                    trie_data_node_t data_node = self->data->a[data_index];
                    unsigned char *current_tail = self->tail->a + data_node.tail;

                    size_t tail_len = strlen((char *)current_tail);
                    char *query_tail = (char *)(*ptr ? ptr + 1 : ptr);
                    size_t query_tail_len = strlen((char *)query_tail);
                    log_debug("next node tail: %s\n", current_tail);
                    log_debug("query node tail: %s\n", query_tail);

                    if (tail_len <= query_tail_len && strncmp((char *)current_tail, query_tail, tail_len) == 0) {
                        state = SEARCH_STATE_MATCH;
                        log_debug("Tail matches\n");
                        last_state = state;
                        data = data_node.data;
                        log_debug("%u, %d, %zu\n", index, phrase_len, tail_len);
                        ptr += tail_len;
                        index += tail_len;
                        advance_index = false;
                        phrase_len = index + 1 - phrase_start;
                        match = true;
                    } else if (match) {
                        log_debug("match is true and longer phrase tail did not match\n");
                        log_debug("phrase_start=%d, phrase_len=%d\n", phrase_start, phrase_len);
                        if (*phrases == NULL) {
                            *phrases = phrase_array_new_size(1);
                        }
                        phrase_array_push(*phrases, (phrase_t){phrase_start, phrase_len, data});
                        ptr = fail_ptr;
                        match = false;
                        index = phrase_start + phrase_len;
                        advance_index = false;
                    }

                } 

                if (ch != '\0') {
                    trie_node_t terminal_node = trie_get_transition(self, node, '\0');
                    if (terminal_node.check == next_id) {
                        log_debug("Transition to NUL byte matched\n");
                        state = SEARCH_STATE_MATCH;
                        match = true;
                        phrase_len = index + (uint32_t)len - phrase_start;
                        if (terminal_node.base < 0) {
                            int32_t data_index = -1*terminal_node.base;
                            trie_data_node_t data_node = self->data->a[data_index];
                            data = data_node.data;
                        }
                        log_debug("Got match with len=%d\n", phrase_len);
                        fail_ptr = ptr;
                    }
                }
            }

        }

        if (unich == 0) {
            if (last_state == SEARCH_STATE_MATCH) {
                log_debug("Found match at the end\n");
                if (*phrases == NULL) {
                    *phrases = phrase_array_new_size(1);
                }
                phrase_array_push(*phrases, (phrase_t){phrase_start, phrase_len, data});
            }
            break;
        }

        if (advance_index) index += len;

        advance_index = true;
        log_debug("index now %u\n", index);
    } // while

    return true;
}

inline bool trie_search_with_phrases(trie_t *self, char *str, phrase_array **phrases) {
    return trie_search_from_index(self, str, ROOT_NODE_ID, phrases);
}

inline phrase_array *trie_search(trie_t *self, char *text) {
    phrase_array *phrases = NULL;
    if (!trie_search_with_phrases(self, text, &phrases)) {
        return false;
    }
    return phrases;
}

int trie_node_search_tail_tokens(trie_t *self, trie_node_t node, char *str, token_array *tokens, size_t tail_index, int token_index) {
    int32_t data_index = -1*node.base;
    trie_data_node_t old_data_node = self->data->a[data_index];
    uint32_t current_tail_pos = old_data_node.tail;

    log_debug("tail_index = %zu\n", tail_index);

    unsigned char *tail_ptr = self->tail->a + current_tail_pos + tail_index;
    
    if (!(*tail_ptr)) {
        log_debug("tail matches!\n");
        return token_index - 1;
    }

    log_debug("Searching tail: %s\n", tail_ptr);
    size_t num_tokens = tokens->n;
    for (int i = token_index; i < num_tokens; i++) {
        token_t token = tokens->a[i];

        char *ptr = str + token.offset;
        size_t token_length = token.len;

        if (!(*tail_ptr)) {
            log_debug("tail matches!\n");
            return i - 1;
        }

        if (token.type == WHITESPACE && *tail_ptr == ' ') continue;

        if (*tail_ptr == ' ') {
            tail_ptr++;
            log_debug("Got space, advancing pointer, tail_ptr=%s\n", tail_ptr);
        }

        log_debug("Tail string compare: %s with %.*s\n", tail_ptr, (int)token_length, ptr);

        if (strncmp((char *)tail_ptr, ptr, token_length) == 0) {
            tail_ptr += token_length;

            if (i == num_tokens - 1 && !(*tail_ptr)) {
                return i;
            }
        } else {
            return -1;
        }
    }
    return -1;

}


#ifdef EXP_PHRASE_PREFILTER
// Experiment 016 T6 (P1). For a (trie, start node): which bytes have a transition from the start node (first[]) and which byte pairs (b0, b1) can be the
// first two bytes walked (pair[]): b1 is a transition of an internal child, or the first byte of a leaf child's tail ('\0' for an empty tail).
// Built from the double array itself (every existing node, not only valid keys), so it is a superset of what any walk can reach.
#ifdef EXP_PREFIX34
// Experiment 024: exact sets of every 3- and 4-byte prefix walkable from the start node (all existing double-array paths, leaf tails appended,
// '\0' transitions excluded since token bytes are never NUL). Open addressing over packed keys (bytes | length << 32); 0 = empty.
typedef struct { uint64_t *k; uint32_t mask; } tpf_set_t;
#endif
typedef struct { const trie_t *t; uint32_t start; uint8_t first[32]; uint8_t pair[8192];
#ifdef EXP_PREFIX34
    tpf_set_t p34; bool p34_ok;
#endif
} tpf_t;
static tpf_t *tpf_cache[256]; static int tpf_n = 0;
int trie_prefilter_test_disable = 0;   // differential tests only
static tpf_t *tpf_get(const trie_t *self, uint32_t start) {
    for (int k = 0; k < tpf_n; k++) if (tpf_cache[k]->t == self && tpf_cache[k]->start == start) return tpf_cache[k];
    if (tpf_n >= 256) return NULL;
    trie_node_t *nodes = self->nodes->a; uint32_t nn = (uint32_t)self->nodes->n;
    if (start >= nn || nodes[start].base < 0) return NULL;
    tpf_t *f = calloc(1, sizeof *f); if (!f) return NULL;
    f->t = self; f->start = start;
    for (unsigned ci = 1; ci <= self->alphabet_size; ci++) {
        uint64_t i1 = (uint64_t)(uint32_t)nodes[start].base + ci; if (i1 >= nn) break;
        if (nodes[i1].check != (int32_t)start) continue;
        uint8_t c0 = (uint8_t)self->alphabet[ci - 1];
        f->first[c0 >> 3] |= (uint8_t)(1u << (c0 & 7));
        if (nodes[i1].base < 0) {                                   // leaf: the second byte walked is the tail's first byte
            int32_t di = -nodes[i1].base;
            if ((size_t)di >= self->data->n) { for (unsigned b = 0; b < 256; b++) f->pair[((c0 << 8) | b) >> 3] |= (uint8_t)(1u << (b & 7)); continue; }   // doubt: allow all
            uint32_t tp = self->data->a[di].tail; uint8_t c1 = tp < self->tail->n ? self->tail->a[tp] : 0;
            f->pair[((c0 << 8) | c1) >> 3] |= (uint8_t)(1u << (c1 & 7));
            continue;
        }
        for (unsigned cj = 1; cj <= self->alphabet_size; cj++) {
            uint64_t i2 = (uint64_t)(uint32_t)nodes[i1].base + cj; if (i2 >= nn) break;
            if (nodes[i2].check != (int32_t)i1) continue;
            uint8_t c1 = (uint8_t)self->alphabet[cj - 1];
            f->pair[((c0 << 8) | c1) >> 3] |= (uint8_t)(1u << (c1 & 7));
        }
    }
#ifdef EXP_PREFIX34
    {   // collect prefixes of length 3 and 4 by DFS to depth 4
        size_t cap = 1 << 16, n = 0; uint64_t *keys = malloc(cap * sizeof *keys); bool ok = keys != NULL;
        uint32_t stk_node[5]; unsigned stk_ci[5]; uint8_t path[8]; int sp = 0; stk_node[0] = start; stk_ci[0] = 1;
#define P34_ADD(len) do { uint64_t kk = 0; for (int q = 0; q < (len); q++) kk |= (uint64_t)path[q] << (8 * q); kk |= (uint64_t)(len) << 32; \
            if (n == cap) { cap *= 2; uint64_t *nk = realloc(keys, cap * sizeof *nk); if (!nk) { ok = false; break; } keys = nk; } keys[n++] = kk; } while (0)
        while (sp >= 0 && ok) {
            uint32_t pid = stk_node[sp]; bool descended = false;
            for (unsigned ci = stk_ci[sp]; ci <= self->alphabet_size && ok; ci++) {
                uint64_t idx = (uint64_t)(uint32_t)nodes[pid].base + ci; if (idx >= nn) break;
                if (nodes[idx].check != (int32_t)pid) continue;
                uint8_t c = (uint8_t)self->alphabet[ci - 1]; if (c == 0) continue;
                path[sp] = c; int d = sp + 1;
                if (nodes[idx].base < 0) {                              // leaf: extend with the tail, up to 4 bytes
                    int32_t di = -nodes[idx].base; int dd = d;
                    if ((size_t)di < self->data->n) { uint32_t tp = self->data->a[di].tail; while (dd < 4 && tp < self->tail->n && self->tail->a[tp]) path[dd++] = self->tail->a[tp++]; }
                    else { ok = false; break; }
                    if (dd >= 3) P34_ADD(3); if (dd >= 4) P34_ADD(4);
                } else {
                    if (d >= 3) P34_ADD(3);
                    if (d == 4) { P34_ADD(4); continue; }
                    stk_ci[sp] = ci + 1; sp++; stk_node[sp] = (uint32_t)idx; stk_ci[sp] = 1; descended = true; break;
                }
            }
            if (!descended) sp--;
        }
#undef P34_ADD
        if (ok) {
            uint32_t sz = 1024; while (sz < n * 2) sz <<= 1;
            f->p34.k = calloc(sz, sizeof(uint64_t)); f->p34.mask = sz - 1;
            if (f->p34.k) { for (size_t q = 0; q < n; q++) { uint64_t kk = keys[q], h = (kk * 0x9E3779B97F4A7C15ULL) >> 32; uint32_t i = (uint32_t)h & f->p34.mask;
                    while (f->p34.k[i] && f->p34.k[i] != kk) i = (i + 1) & f->p34.mask; f->p34.k[i] = kk; } f->p34_ok = true; }
        }
        free(keys);
    }
#endif
    tpf_cache[tpf_n++] = f;
    return f;
}
#ifdef EXP_PREFIX34
static inline bool tpf_has(const tpf_t *f, const uint8_t *p, int len) {
    uint64_t kk = 0; for (int q = 0; q < len; q++) kk |= (uint64_t)p[q] << (8 * q); kk |= (uint64_t)len << 32;
    uint64_t h = (kk * 0x9E3779B97F4A7C15ULL) >> 32; uint32_t i = (uint32_t)h & f->p34.mask;
    for (;;) { uint64_t v = f->p34.k[i]; if (!v) return false; if (v == kk) return true; i = (i + 1) & f->p34.mask; }
}
#endif
#endif

#ifdef EXP_PHRASE_PREFILTER_VERIFY
#define trie_search_tokens_from_index tsti_impl
#endif
#ifdef EXP_SEARCH_STATS
// Measurement only: for tokens walked from a fresh state, how many fall off inside the token and after how many bytes.
static unsigned long long ss_fresh = 0, ss_fall[8] = {0};
__attribute__((destructor)) static void ss_report(void) { if (!ss_fresh) return; fprintf(stderr, "SEARCH_STATS fresh_tokens=%llu fall_after_bytes:", ss_fresh); for (int k = 0; k < 8; k++) fprintf(stderr, " %d:%llu", k, ss_fall[k]); fprintf(stderr, "\n"); }
#endif
#ifdef EXP_LOCKSTEP
// Experiment 037. Every token that the search below begins from the start node in a fresh state walks the same bytes through the same nodes,
// independent of the other tokens. Those walks are computed first, all tokens in lockstep (one byte position at a time), so their node reads
// overlap in memory instead of forming one long dependent chain. The search then reads the precomputed (node id, node) pairs instead of loading
// them; every decision it makes is unchanged. A walk stops exactly where the search's byte loop stops (fell off, or reached a leaf).
int trie_lockstep_test_disable = 0;   // differential tests only
static uint32_t *ls_id = NULL; static trie_node_t *ls_node = NULL; static size_t ls_cap = 0;
static uint32_t *ls_off = NULL, *ls_cur = NULL; static trie_node_t *ls_curn = NULL; static uint8_t *ls_alive = NULL; static size_t ls_tcap = 0;
static bool ls_prepare(trie_t *self, char *str, token_array *tokens, uint32_t start_node_id, trie_node_t start) {
    size_t nt = tokens->n, total = 0;
    if (nt > ls_tcap) { size_t m = ls_tcap ? ls_tcap : 64; while (m < nt) m *= 2;
        uint32_t *a = realloc(ls_off, m * sizeof *a); if (!a) return false; ls_off = a; uint32_t *b = realloc(ls_cur, m * sizeof *b); if (!b) return false; ls_cur = b;
        trie_node_t *c = realloc(ls_curn, m * sizeof *c); if (!c) return false; ls_curn = c; uint8_t *d = realloc(ls_alive, m); if (!d) return false; ls_alive = d; ls_tcap = m; }
    size_t maxlen = 0;
    for (size_t i = 0; i < nt; i++) { token_t t = tokens->a[i]; ls_off[i] = (uint32_t)total;
        bool alive = t.type != WHITESPACE && t.len > 0; ls_alive[i] = alive; if (alive) { total += t.len; if (t.len > maxlen) maxlen = t.len; }
        ls_cur[i] = start_node_id; ls_curn[i] = start; }
    if (total > ls_cap) { size_t m = ls_cap ? ls_cap : 256; while (m < total) m *= 2;
        uint32_t *a = realloc(ls_id, m * sizeof *a); if (!a) return false; ls_id = a; trie_node_t *b = realloc(ls_node, m * sizeof *b); if (!b) return false; ls_node = b; ls_cap = m; }
    for (size_t j = 0; j < maxlen; j++) {
        for (size_t i = 0; i < nt; i++) {
            if (!ls_alive[i]) continue;
            token_t t = tokens->a[i];
            if (j >= t.len) { ls_alive[i] = 0; continue; }
            uint32_t id = trie_get_transition_index(self, ls_curn[i], (unsigned char)str[t.offset + j]);
            trie_node_t nd = trie_get_node(self, id);
            ls_id[ls_off[i] + j] = id; ls_node[ls_off[i] + j] = nd;
            if (nd.check != (int32_t)ls_cur[i] || nd.base < 0) { ls_alive[i] = 0; continue; }   // the search's byte loop breaks here too
            ls_cur[i] = id; ls_curn[i] = nd;
        }
    }
    return true;
}
#endif
#ifdef EXP_LOCKSTEP_VERIFY
#define trie_search_tokens_from_index lsti_impl
#endif
bool trie_search_tokens_from_index(trie_t *self, char *str, token_array *tokens, uint32_t start_node_id, phrase_array **phrases) {
    if (str == NULL || tokens == NULL || tokens->n == 0) return false;
#ifdef EXP_PHRASE_PREFILTER
    const tpf_t *tpf = trie_prefilter_test_disable ? NULL : tpf_get(self, start_node_id);
#endif

    uint32_t node_id = start_node_id, last_node_id = start_node_id;
    trie_node_t node = trie_get_node(self, node_id), last_node = node;
#ifdef EXP_LOCKSTEP
    const trie_node_t ls_start = node;
    bool ls_ok = !trie_lockstep_test_disable && ls_start.base >= 0 && ls_prepare(self, str, tokens, start_node_id, ls_start);
#endif

    uint32_t data;

    int phrase_len = 0, phrase_start = 0, last_match_index = -1;

#ifdef EXP_SEARCH_PREFETCH
    // Pure hint (no effect on results): most tokens restart the walk at start_node_id and leave the trie after one or two characters, and those walks are independent
    // of each other, so touch the second-level node (or the leaf's data entry) of every token up front to overlap the cache misses.
    if (node.base >= 0) {
        uint32_t nn = (uint32_t)self->nodes->n;
        for (size_t k = 0; k < tokens->n; k++) {
            token_t tk = tokens->a[k];
            if (tk.type == WHITESPACE || tk.len == 0) continue;
            uint32_t i1 = node.base + trie_get_char_index(self, (unsigned char)str[tk.offset]);
            if (i1 >= nn) continue;
            trie_node_t n1 = self->nodes->a[i1];
            if (n1.check != start_node_id) continue;
            if (n1.base >= 0) {
                if (tk.len > 1) {
                    uint32_t i2 = n1.base + trie_get_char_index(self, (unsigned char)str[tk.offset + 1]);
                    if (i2 < nn) __builtin_prefetch(&self->nodes->a[i2]);
                }
            } else {
                __builtin_prefetch(&self->data->a[-n1.base]);
            }
        }
    }
#endif

    trie_search_state_t state = SEARCH_STATE_BEGIN, last_state = SEARCH_STATE_BEGIN;

    token_t token;
    size_t token_length;

    log_debug("num_tokens: %zu\n", tokens->n);
    for (int i = 0; i < tokens->n; i++, last_state = state) {
        token = tokens->a[i];
        token_length = token.len;
        
        char *ptr = str + token.offset;
        log_debug("On %d, token=%.*s\n", i, (int)token_length, ptr);

        bool check_continuation = true;
#ifdef EXP_SEARCH_STATS
        bool ss_is_fresh = token.type != WHITESPACE && token_length > 0 && node_id == start_node_id && last_node_id == start_node_id && last_match_index == -1 &&
            (last_state == SEARCH_STATE_NO_MATCH || last_state == SEARCH_STATE_BEGIN);
        if (ss_is_fresh) ss_fresh++;
#endif
#ifdef EXP_PHRASE_PREFILTER
        // Fresh state (no walk or match in progress): a token whose first byte (or, for tokens of >= 2 bytes, first byte pair) cannot be walked from the
        // start node falls off the trie inside the token, and the original code then ends in exactly this state.
        if (tpf != NULL && token.type != WHITESPACE && token_length > 0 && node_id == start_node_id && last_node_id == start_node_id && last_match_index == -1 &&
            (last_state == SEARCH_STATE_NO_MATCH || last_state == SEARCH_STATE_BEGIN)) {
            uint8_t b0 = (uint8_t)ptr[0];
            bool can = (tpf->first[b0 >> 3] >> (b0 & 7)) & 1;
            if (can && token_length >= 2) { uint8_t b1 = (uint8_t)ptr[1]; can = (tpf->pair[((b0 << 8) | b1) >> 3] >> (b1 & 7)) & 1; }
#ifdef EXP_PREFIX34
            if (can && token_length >= 3 && tpf->p34_ok) { can = tpf_has(tpf, (const uint8_t *)ptr, 3); if (can && token_length >= 4) can = tpf_has(tpf, (const uint8_t *)ptr, 4); }
#endif
            if (!can) {
                state = SEARCH_STATE_NO_MATCH;
                phrase_start = phrase_len = 0;
                node_id = last_node_id = start_node_id;
                node = last_node = trie_get_node(self, start_node_id);
                continue;
            }
        }
#endif

#ifdef EXP_LOCKSTEP
        // fresh state: the walk below is exactly the precomputed one (same start node, same bytes)
        bool ls_use = ls_ok && token.type != WHITESPACE && node_id == start_node_id && last_node_id == start_node_id &&
            node.base == ls_start.base && node.check == ls_start.check && last_node.base == ls_start.base && last_node.check == ls_start.check;
        const uint32_t *ls_tid = ls_use ? ls_id + ls_off[i] : NULL; const trie_node_t *ls_tnode = ls_use ? ls_node + ls_off[i] : NULL;
#endif
        if (token.type != WHITESPACE) {
            for (int j = 0; j < token_length; j++, ptr++, last_node = node, last_node_id = node_id) {
                log_debug("Getting transition index for %d, (%d, %d)\n", node_id, node.base, node.check);
                size_t offset = j + 1;
                if (j > 0 || last_node.base >= 0) {
#ifdef EXP_LOCKSTEP
                    if (ls_tid != NULL) { node_id = ls_tid[j]; node = ls_tnode[j]; } else {
#endif
                    node_id = trie_get_transition_index(self, node, *ptr);
                    node = trie_get_node(self, node_id);
#ifdef EXP_LOCKSTEP
                    }
#endif
                    log_debug("Doing %c, got node_id=%d\n", *ptr, node_id);
                } else {
                    log_debug("Tail stored on space node, rolling back one character\n");
                    ptr--;
                    offset = j;
                    log_debug("ptr=%s\n", ptr);
                }

                if (node.check != last_node_id && last_node.base >= 0) {
#ifdef EXP_SEARCH_STATS
                    if (ss_is_fresh) ss_fall[j < 7 ? j : 7]++;
#endif
                    log_debug("Fell off trie. last_node_id=%d and node.check=%d\n", last_node_id, node.check);
                    node_id = last_node_id = start_node_id;
                    node = last_node = trie_get_node(self, node_id);
                    break;
                } else if (node.base < 0) {
                    log_debug("Searching tail at index %d\n", i);

                    uint32_t data_index = -1*node.base;
                    trie_data_node_t data_node = self->data->a[data_index];
                    uint32_t current_tail_pos = data_node.tail;

                    unsigned char *current_tail = self->tail->a + current_tail_pos;

                    log_debug("token_length = %zu, j=%d\n", token_length, j);

                    size_t ptr_len = token_length - offset;
                    log_debug("next node tail: %s vs %.*s\n", current_tail, (int)ptr_len, ptr + 1);

                    if (last_state == SEARCH_STATE_NO_MATCH || last_state == SEARCH_STATE_BEGIN) {
                        log_debug("phrase start at %d\n", i);
                        phrase_start = i;
                    }
                    if (strncmp((char *)current_tail, ptr + 1, ptr_len) == 0) {
                        log_debug("node tail matches first token\n");
                        int tail_search_result = trie_node_search_tail_tokens(self, node, str, tokens, ptr_len, i + 1);
                        log_debug("tail_search_result=%d\n", tail_search_result);
                        node_id = start_node_id;
                        node = trie_get_node(self, node_id);
                        check_continuation = false;

                        if (tail_search_result != -1) {
                            phrase_len = tail_search_result - phrase_start + 1;
                            last_match_index = i = tail_search_result;
                            last_state = SEARCH_STATE_MATCH;
                            data = data_node.data;
                        }
                        break;

                    } else {
                        node_id = start_node_id;
                        node = trie_get_node(self, node_id);
                        break;
                    }
                }
            }
        } else {
            check_continuation = false;
            if (state == SEARCH_STATE_BEGIN || state == SEARCH_STATE_NO_MATCH) {
                continue;
            }
        }


        if (node.check <= 0 || node_id == start_node_id) {
            log_debug("state = SEARCH_STATE_NO_MATCH\n");
            state = SEARCH_STATE_NO_MATCH;
            // check
            if (last_match_index != -1) {
                log_debug("last_match not NULL and state==SEARCH_STATE_NO_MATCH, data=%d\n", data);
                if (*phrases == NULL) {
                    *phrases = phrase_array_new_size(1);
                }
                phrase_array_push(*phrases, (phrase_t){phrase_start, last_match_index - phrase_start + 1, data});
                i = last_match_index;
                last_match_index = -1;
                phrase_start = phrase_len = 0;
                node_id = last_node_id = start_node_id;
                node = last_node = trie_get_node(self, start_node_id);
                continue;
            } else if (last_state == SEARCH_STATE_PARTIAL_MATCH) {
                // we're in the middle of a phrase that did not fully
                // match the trie. Imagine a trie that contains "a b c" and "b"
                // and our string is "a b d". When we get to "d", we've matched
                // the prefix "a b" and then will fall off the trie. So we'll actually 
                // need to go back to token "b" and start searching from the root

                // Note: i gets incremented with the next iteration of the for loop
                i = phrase_start;
                log_debug("last_state == SEARCH_STATE_PARTIAL_MATCH, i = %d\n", i);
                last_match_index = -1;
                phrase_start = phrase_len = 0;
                node_id = last_node_id = start_node_id;
                node = last_node = trie_get_node(self, start_node_id);
                continue;
            } else {
                phrase_start = phrase_len = 0;
                // this token was not a phrase
                log_debug("Plain token=%.*s\n", (int)token.len, str + token.offset);
            }
            node_id = last_node_id = start_node_id;
            node = last_node = trie_get_node(self, start_node_id);
        } else {

            state = SEARCH_STATE_PARTIAL_MATCH;
            if (!(node.base < 0) && (last_state == SEARCH_STATE_NO_MATCH || last_state == SEARCH_STATE_BEGIN)) {
                log_debug("phrase_start=%d, node.base = %d, last_state=%d\n", i, node.base, last_state);
                phrase_start = i;
            }

            trie_node_t terminal_node = trie_get_transition(self, node, '\0');
            if (terminal_node.check == node_id) {
                log_debug("node match at %d\n", i);
                state = SEARCH_STATE_MATCH;
                int32_t data_index = -1*terminal_node.base;
                trie_data_node_t data_node = self->data->a[data_index];
                data = data_node.data;
                log_debug("data = %d\n", data);

                log_debug("phrase_start = %d\n", phrase_start);

                last_match_index = i;
                log_debug("last_match_index = %d\n", i);
            }

            if (i == tokens->n - 1) {
                if (last_match_index == -1) {
                    log_debug("At last token\n");
                    break;
                } else {
                    if (*phrases == NULL) {
                        *phrases = phrase_array_new_size(1);
                    }
                    phrase_array_push(*phrases, (phrase_t){phrase_start, last_match_index - phrase_start + 1, data});
                    i = last_match_index;
                    last_match_index = -1;
                    phrase_start = phrase_len = 0;
                    node_id = last_node_id = start_node_id;
                    node = last_node = trie_get_node(self, start_node_id);
                    state = SEARCH_STATE_NO_MATCH;
                    continue;
                }
            }

            if (check_continuation) {

                // Check continuation
                uint32_t continuation_id = trie_get_transition_index(self, node, ' ');
                log_debug("transition_id: %u\n", continuation_id);
                trie_node_t continuation = trie_get_node(self, continuation_id);

                if (token.type == IDEOGRAPHIC_CHAR && continuation.check != node_id) {
                    log_debug("Ideographic character\n");
                    last_node_id = node_id;
                    last_node = node;
                } else if (continuation.check != node_id && last_match_index != -1) {
                    log_debug("node->match no continuation\n");
                    if (*phrases == NULL) {
                        *phrases = phrase_array_new_size(1);
                    }
                    phrase_array_push(*phrases, (phrase_t){phrase_start, last_match_index - phrase_start + 1, data});
                    i = last_match_index;
                    last_match_index = -1;
                    phrase_start = phrase_len = 0;
                    node_id = last_node_id = start_node_id;
                    node = last_node = trie_get_node(self, start_node_id);
                    state = SEARCH_STATE_BEGIN;
                } else if (continuation.check != node_id) {
                    log_debug("No continuation for phrase with start=%d, yielding tokens\n", phrase_start);
                    state = SEARCH_STATE_NO_MATCH;
                    phrase_start = phrase_len = 0;
                    node_id = last_node_id = start_node_id;
                    node = last_node = trie_get_node(self, start_node_id);
                } else {
                    log_debug("Has continuation, node_id=%d\n", continuation_id);
                    last_node = node = continuation;
                    last_node_id = node_id = continuation_id;
                }            
            }
        }

    }

    if (last_match_index != -1) {
        if (*phrases == NULL) {
            *phrases = phrase_array_new_size(1);
        }
        log_debug("adding phrase, last_match_index=%d\n", last_match_index);
        phrase_array_push(*phrases, (phrase_t){phrase_start, last_match_index - phrase_start + 1, data});
   }

    return true;
}
#ifdef EXP_PHRASE_PREFILTER_VERIFY
#undef trie_search_tokens_from_index
unsigned long long tpf_verified = 0;
__attribute__((destructor)) static void tpf_report(void) { fprintf(stderr, "EXP_PHRASE_PREFILTER_VERIFY: %llu searches checked, 0 mismatches\n", tpf_verified); }
// Runs every search without the prefilter (reference) and with it, and aborts unless the appended phrases and the return value are identical.
bool trie_search_tokens_from_index(trie_t *self, char *str, token_array *tokens, uint32_t start_node_id, phrase_array **phrases) {
    phrase_array *ref = NULL;
    trie_prefilter_test_disable = 1; bool r1 = tsti_impl(self, str, tokens, start_node_id, &ref); trie_prefilter_test_disable = 0;
    size_t n0 = *phrases ? (*phrases)->n : 0;
    bool r2 = tsti_impl(self, str, tokens, start_node_id, phrases);
    size_t n1 = *phrases ? (*phrases)->n : 0, nr = ref ? ref->n : 0;
    bool ok = r1 == r2 && n1 - n0 == nr;
    for (size_t k = 0; ok && k < nr; k++) { phrase_t a = ref->a[k], b = (*phrases)->a[n0 + k]; ok = a.start == b.start && a.len == b.len && a.data == b.data; }
    if (!ok) { fprintf(stderr, "EXP_PHRASE_PREFILTER_VERIFY MISMATCH start_node=%u str='%s' ref=%zu got=%zu\n", start_node_id, str, nr, n1 - n0); abort(); }
    if (ref) phrase_array_destroy(ref);
    tpf_verified++;
    return r2;
}
#endif

#ifdef EXP_LOCKSTEP_VERIFY
#undef trie_search_tokens_from_index
unsigned long long ls_verified = 0;
__attribute__((destructor)) static void ls_report(void) { fprintf(stderr, "EXP_LOCKSTEP_VERIFY: %llu searches checked, 0 mismatches\n", ls_verified); }
// Runs every search without the lockstep walks (reference) and with them, and aborts unless the appended phrases and the return value are identical.
bool trie_search_tokens_from_index(trie_t *self, char *str, token_array *tokens, uint32_t start_node_id, phrase_array **phrases) {
    phrase_array *ref = NULL;
    trie_lockstep_test_disable = 1; bool r1 = lsti_impl(self, str, tokens, start_node_id, &ref); trie_lockstep_test_disable = 0;
    size_t n0 = *phrases ? (*phrases)->n : 0;
    bool r2 = lsti_impl(self, str, tokens, start_node_id, phrases);
    size_t n1 = *phrases ? (*phrases)->n : 0, nr = ref ? ref->n : 0;
    bool ok = r1 == r2 && n1 - n0 == nr;
    for (size_t k = 0; ok && k < nr; k++) { phrase_t a = ref->a[k], b = (*phrases)->a[n0 + k]; ok = a.start == b.start && a.len == b.len && a.data == b.data; }
    if (!ok) { fprintf(stderr, "EXP_LOCKSTEP_VERIFY MISMATCH start_node=%u str='%s' ref=%zu got=%zu\n", start_node_id, str, nr, n1 - n0); abort(); }
    if (ref) phrase_array_destroy(ref);
    ls_verified++;
    return r2;
}
#endif

inline bool trie_search_tokens_with_phrases(trie_t *self, char *str, token_array *tokens, phrase_array **phrases) {
    return trie_search_tokens_from_index(self, str, tokens, ROOT_NODE_ID, phrases);
}

inline phrase_array *trie_search_tokens(trie_t *self, char *str, token_array *tokens) {
    phrase_array *phrases = NULL;
    if (!trie_search_tokens_with_phrases(self, str, tokens, &phrases)) {
        return NULL;
    }
    return phrases;
}

phrase_t trie_search_suffixes_from_index(trie_t *self, char *word, size_t len, uint32_t start_node_id) {
    uint32_t last_node_id = start_node_id;
    trie_node_t last_node = trie_get_node(self, last_node_id);
    uint32_t node_id = last_node_id;
    trie_node_t node = last_node;

    uint32_t value = 0, phrase_start = 0, phrase_len = 0;

    ssize_t char_len;

    int32_t unich = 0;

    ssize_t index = len;
    const uint8_t *ptr = (const uint8_t *)word;
    const uint8_t *char_ptr;

    bool in_tail = false;
    unsigned char *current_tail = (unsigned char *)"";
    size_t tail_remaining = 0;

    uint32_t tail_value = 0;

    while(index > 0) {
        char_len = utf8proc_iterate_reversed(ptr, index, &unich);

        if (char_len <= 0) return NULL_PHRASE;
        if (!(utf8proc_codepoint_valid(unich))) return NULL_PHRASE;

        index -= char_len;
        char_ptr = ptr + index;

        if (in_tail && tail_remaining >= char_len && strncmp((char *)current_tail, (char *)char_ptr, char_len) == 0) {
            tail_remaining -= char_len;
            current_tail += char_len;
            phrase_start = (uint32_t)index;

            log_debug("tail matched at char %.*s (len=%zd)\n", (int)char_len, char_ptr, char_len);
            log_debug("tail_remaining = %zu\n", tail_remaining);

            if (tail_remaining == 0) {
                log_debug("tail match! tail_value=%u\n",tail_value);
                phrase_len = (uint32_t)(len - index);
                value = tail_value;
                index = 0;
                break;
            }
            continue;
        } else if (in_tail) {
            break;
        }

        for (int i=0; i < char_len; i++, char_ptr++, last_node = node, last_node_id = node_id) {
            log_debug("char=%c\n", (unsigned char)*char_ptr);

            node_id = trie_get_transition_index(self, node, *char_ptr);
            node = trie_get_node(self, node_id);
    
            if (node.check != last_node_id) {
                log_debug("node.check = %d and last_node_id = %d\n", node.check, last_node_id);
                index = 0;
                break;
            } else if (node.base < 0) {
                log_debug("Searching tail\n");

                uint32_t data_index = -1*node.base;
                trie_data_node_t data_node = self->data->a[data_index];
                uint32_t current_tail_pos = data_node.tail;

                tail_value = data_node.data;

                current_tail = self->tail->a + current_tail_pos;

                tail_remaining = strlen((char *)current_tail);
                log_debug("tail_remaining=%zu\n", tail_remaining);
                in_tail = true;

                size_t remaining_char_len = char_len - i - 1;
                log_debug("remaining_char_len = %zu\n", remaining_char_len);

                if (remaining_char_len > 0 && strncmp((char *)char_ptr + 1, (char *)current_tail, remaining_char_len) == 0) {
                    log_debug("tail string comparison successful\n");
                    tail_remaining -= remaining_char_len;
                    current_tail += remaining_char_len;
                } else if (remaining_char_len > 0) {
                    log_debug("tail comparison unsuccessful, %s vs %s\n", char_ptr, current_tail);
                    index = 0;
                    break;
                }

                if (tail_remaining == 0) {
                    phrase_start = (uint32_t)index;
                    phrase_len = (uint32_t)(len - index);
                    log_debug("phrase_start = %d, phrase_len=%d\n", phrase_start, phrase_len);
                    value = tail_value;
                    index = 0;
                }
                break;
            } else if (i == char_len - 1) {
                trie_node_t terminal_node = trie_get_transition(self, node, '\0');
                if (terminal_node.check == node_id) {
                    int32_t data_index = -1 * terminal_node.base;
                    trie_data_node_t data_node = self->data->a[data_index];
                    value = data_node.data;
                    phrase_start = (uint32_t)index;
                    phrase_len = (uint32_t)(len - index);
                }
            }

        }

    }

    return (phrase_t) {phrase_start, phrase_len, value};
}

inline phrase_t trie_search_suffixes_from_index_get_suffix_char(trie_t *self, char *word, size_t len, uint32_t start_node_id) {
    if (word == NULL || len == 0) return NULL_PHRASE;
    trie_node_t node = trie_get_node(self, start_node_id);
    unsigned char suffix_char = TRIE_SUFFIX_CHAR[0];
    uint32_t node_id = trie_get_transition_index(self, node, suffix_char);
    node = trie_get_node(self, node_id);

    if (node.check != start_node_id) {
        log_debug("node.check != start_node_id\n");
        return NULL_PHRASE;
    }

    return trie_search_suffixes_from_index(self, word, len, node_id);
}

inline phrase_t trie_search_suffixes(trie_t *self, char *word, size_t len) {
    if (word == NULL || len == 0) return NULL_PHRASE;
    return trie_search_suffixes_from_index_get_suffix_char(self, word, len, ROOT_NODE_ID);
}


phrase_t trie_search_prefixes_from_index(trie_t *self, char *word, size_t len, uint32_t start_node_id) {
    log_debug("Call to trie_search_prefixes_from_index\n");
    uint32_t node_id = start_node_id, last_node_id = node_id;
    trie_node_t node = trie_get_node(self, node_id), last_node = node;

    log_debug("last_node_id = %d\n", last_node_id);

    uint32_t value = 0, phrase_start = 0, phrase_len = 0;

    uint8_t *ptr = (uint8_t *)word;

    ssize_t char_len = 0;

    uint32_t idx = 0;

    size_t separator_char_len = 0;

    int32_t codepoint = 0;

    bool first_char = true;

    trie_data_node_t data_node;
    trie_node_t terminal_node;

    bool phrase_at_hyphen = false;

    while (idx < len) {
        char_len = utf8proc_iterate(ptr, len, &codepoint);
        log_debug("char_len = %zu, char=%d\n", char_len, codepoint);
        if (char_len <= 0) break;
        if (!(utf8proc_codepoint_valid(codepoint))) break;

        bool is_hyphen = utf8_is_hyphen(codepoint);

        int cat = utf8proc_category(codepoint);
        bool is_space = utf8_is_separator(cat);

        uint8_t *char_ptr = ptr;
        size_t i = 0;

        bool skip_char = false;
        bool break_out = false;

        for (i = 0; i < char_len; i++) {
            node_id = trie_get_transition_index(self, last_node, *char_ptr);
            node = trie_get_node(self, node_id);
            log_debug("At idx=%u, i=%zu, char=%.*s\n", idx, i, (int)char_len, char_ptr);

            if (node.check != last_node_id) {
                log_debug("node.check = %d and last_node_id = %d\n", node.check, last_node_id);

                if (is_hyphen || (is_space && *ptr != ' ')) {
                    log_debug("Got hyphen or other separator, trying space instead\n");
                    node_id = trie_get_transition_index(self, last_node, ' ');
                    node = trie_get_node(self, node_id);
                }

                if (is_hyphen && node.check != last_node_id) {
                    log_debug("No space transition, phrase_len=%zu\n", phrase_len);
                    if (phrase_len > 0 && phrase_len == idx) {
                        log_debug("phrase_at_hyphen\n");
                        phrase_at_hyphen = true;
                    }

                    ptr += char_len;
                    idx += char_len;
                    separator_char_len = char_len;
                    node_id = last_node_id;
                    node = trie_get_node(self, node_id);
                    skip_char = true;
                    break;
                } else if (node.check != last_node_id) {
                    break_out = true;
                    log_debug("Breaking\n");
                    break;
                }
                break;
            }

            if (first_char) {
                phrase_start = idx;
                first_char = false;
            }

            if (node.base < 0) {
                log_debug("Searching tail\n");

                data_node = trie_get_data_node(self, node);
                uint32_t current_tail_pos = data_node.tail;

                unsigned char *current_tail = self->tail->a + current_tail_pos;

                log_debug("comparing tail: %s vs %s\n", current_tail, char_ptr + 1);
                size_t current_tail_len = strlen((char *)current_tail);

                size_t match_len = i + 1;
                size_t offset = i + 1;
                size_t tail_pos = 0;
                log_debug("offset=%zu\n", offset);

                if (char_len > 1) {
                    log_debug("char_len = %zu\n", char_len);
                    log_debug("Doing strncmp: (%zu) %s vs %s\n", char_len - offset, current_tail, char_ptr + 1);

                    if (strncmp((char *)ptr + offset, (char *)current_tail, char_len - offset) == 0) {
                        match_len += char_len - offset;
                        tail_pos = char_len - offset;
                        log_debug("in char match_len = %zu\n", match_len);
                    } else {
                        return NULL_PHRASE;
                    }
                }

                size_t tail_match_len = utf8_common_prefix_len((char *)ptr + char_len, (char *)current_tail + tail_pos, current_tail_len - tail_pos);
                match_len += tail_match_len;
                log_debug("match_len=%zu\n", match_len);
                
                if (tail_match_len == current_tail_len - tail_pos) {
                    if (phrase_at_hyphen) {
                        char_len = utf8proc_iterate(ptr + char_len, len, &codepoint);
                        if (char_len > 0 && utf8proc_codepoint_valid(codepoint)) {
                            int cat = utf8proc_category(codepoint);

                            if (codepoint != 0 && !utf8_is_hyphen(codepoint) && !utf8_is_separator(cat) && !utf8_is_punctuation(cat)) {
                                return (phrase_t){phrase_start, phrase_len, value};
                            }
                        }

                        // Note: don't need to check the < 0 case because we're returning from this branch.
                    }
                    if (first_char) phrase_start = idx;
                    phrase_len = (uint32_t)(idx + match_len) - phrase_start;

                    log_debug("tail match! phrase_len=%u, len=%zu\n", phrase_len, len);
                    value = data_node.data;
                    return (phrase_t){phrase_start, phrase_len, value};
                } else {
                    return NULL_PHRASE;
                }

            } else if (node.check == last_node_id) {
                terminal_node = trie_get_transition(self, node, '\0');
                log_debug("Trying link from %d to terminal node\n", last_node_id);

                if (terminal_node.check == node_id) {
                    log_debug("Transition to NUL byte matched\n");
                    if (terminal_node.base < 0) {
                        phrase_len = (uint32_t)(idx + char_len) - phrase_start;
                        data_node = trie_get_data_node(self, terminal_node);
                        value = data_node.data;
                    }
                    log_debug("Got match with len=%d\n", phrase_len);
                }
            }

            last_node = node;
            last_node_id = node_id;
            log_debug("last_node_id = %d\n", last_node_id);
            char_ptr++;
        }


        if (break_out) {
            break;
        } else if (skip_char) {
            continue;
        }

        log_debug("Incrementing index\n");

        idx += char_len;
        ptr += char_len;
    }

    log_debug("exited while loop\n");

    if (phrase_len == 0) return NULL_PHRASE;
 
    return (phrase_t) {phrase_start, phrase_len, value};
}

inline phrase_t trie_search_prefixes_from_index_get_prefix_char(trie_t *self, char *word, size_t len, uint32_t start_node_id) {
    trie_node_t node = trie_get_node(self, start_node_id);
    unsigned char prefix_char = TRIE_PREFIX_CHAR[0];
    uint32_t node_id = trie_get_transition_index(self, node, prefix_char);
    node = trie_get_node(self, node_id);

    if (node.check != start_node_id) {
        return NULL_PHRASE;
    }

    return trie_search_prefixes_from_index(self, word, len, node_id);
}

inline phrase_t trie_search_prefixes(trie_t *self, char *word, size_t len) {
    if (word == NULL || len == 0) return NULL_PHRASE;
    return trie_search_prefixes_from_index_get_prefix_char(self, word, len, ROOT_NODE_ID);
}

bool token_phrase_memberships(phrase_array *phrases, int64_array *phrase_memberships, size_t len) {
    if (phrases == NULL || phrase_memberships == NULL) {
        return false;
    }

    int64_t i = 0;
    for (int64_t j = 0; j < phrases->n; j++) {
        phrase_t phrase = phrases->a[j];

        for (; i < phrase.start; i++) {
            int64_array_push(phrase_memberships, NULL_PHRASE_MEMBERSHIP);
            log_debug("token i=%" PRId64 ", null phrase membership\n", i);
        }

        for (i = phrase.start; i < phrase.start + phrase.len; i++) {
            log_debug("token i=%" PRId64 ", phrase membership=%" PRId64 "\n", i, j);
            int64_array_push(phrase_memberships, j);
        }
    }

    for (; i < len; i++) {
        log_debug("token i=%" PRId64 ", null phrase membership\n", i);
        int64_array_push(phrase_memberships, NULL_PHRASE_MEMBERSHIP);
    }

    return true;
}

inline char *cstring_array_get_phrase(cstring_array *str, char_array *phrase_tokens, phrase_t phrase) {
    char_array_clear(phrase_tokens);

    size_t phrase_end = phrase.start + phrase.len;

    for (int k = phrase.start; k < phrase_end; k++) {
        char *w = cstring_array_get_string(str, k);
        char_array_append(phrase_tokens, w);
        if (k < phrase_end - 1) {
            char_array_append(phrase_tokens, " ");
        }
    }
    char_array_terminate(phrase_tokens);

    return char_array_get_string(phrase_tokens);
}

