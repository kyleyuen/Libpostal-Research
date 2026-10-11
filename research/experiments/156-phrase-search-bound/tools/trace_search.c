// 156 diagnostic: trace the ORIGINAL trie_search_tokens_from_index (copied verbatim from libpostal/src/trie_search.c, log_debug -> printf)
// on given admin tokens, against the loaded parser's component-phrase trie or postal trie. Never timed.
// usage: trace_search {phrases|postal} type:token ...
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"
#include "address_parser.h"
#include "trie_search.h"
#define log_debug printf
int trie_node_search_tail_tokens(trie_t *self, trie_node_t node, char *str, token_array *tokens, size_t tail_index, int token_index);   // defined in trie_search.c
typedef enum { SEARCH_STATE_BEGIN, SEARCH_STATE_NO_MATCH, SEARCH_STATE_PARTIAL_MATCH, SEARCH_STATE_MATCH } trie_search_state_t;   // copied from trie_search.c:3
static bool traced_search(trie_t *self, char *str, token_array *tokens, uint32_t start_node_id, phrase_array **phrases) {
    if (str == NULL || tokens == NULL || tokens->n == 0) return false;

    uint32_t node_id = start_node_id, last_node_id = start_node_id;
    trie_node_t node = trie_get_node(self, node_id), last_node = node;

    uint32_t data;

    int phrase_len = 0, phrase_start = 0, last_match_index = -1;

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

        if (token.type != WHITESPACE) {
            for (int j = 0; j < token_length; j++, ptr++, last_node = node, last_node_id = node_id) {
                log_debug("Getting transition index for %d, (%d, %d)\n", node_id, node.base, node.check);
                size_t offset = j + 1;
                if (j > 0 || last_node.base >= 0) {
                    node_id = trie_get_transition_index(self, node, *ptr);
                    node = trie_get_node(self, node_id);
                    log_debug("Doing %c, got node_id=%d\n", *ptr, node_id);
                } else {
                    log_debug("Tail stored on space node, rolling back one character\n");
                    ptr--;
                    offset = j;
                    log_debug("ptr=%s\n", ptr);
                }

                if (node.check != last_node_id && last_node.base >= 0) {
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
int main(int argc, char **argv) {
    if (argc < 3 || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    address_parser_t *p = get_address_parser(); trie_t *t = !strcmp(argv[1], "postal") ? p->postal_codes : p->phrases;
    char str[4096]; size_t n = 0; token_array *tok = token_array_new();
    for (int i = 2; i < argc; i++) { char *c = strchr(argv[i], ':'); int ty = atoi(argv[i]); const char *w = c + 1; size_t l = strlen(w);
        memcpy(str + n, w, l + 1); token_array_push(tok, (token_t){n, l, (uint16_t)ty}); n += l + 1; }
    phrase_array *ph = NULL; traced_search(t, str, tok, ROOT_NODE_ID, &ph);
    printf("RESULT:"); if (ph) for (size_t i = 0; i < ph->n; i++) printf(" (%u,%u,%u)", ph->a[i].start, ph->a[i].len, ph->a[i].data); printf("\n");
    phrase_array *ph2 = NULL; trie_search_tokens_from_index(t, str, tok, ROOT_NODE_ID, &ph2);
    printf("LIBRARY:"); if (ph2) for (size_t i = 0; i < ph2->n; i++) printf(" (%u,%u,%u)", ph2->a[i].start, ph2->a[i].len, ph2->a[i].data); printf("\n");
    return 0;
}
