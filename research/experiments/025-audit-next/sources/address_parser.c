#include "address_parser.h"
#include "address_dictionary.h"
#include "features.h"
#include "ngrams.h"
#include "scanner.h"

#include "graph_builder.h"

#include "klib/ksort.h"
#include "log/log.h"

#define ADDRESS_PARSER_MODEL_FILENAME "address_parser.dat"
#define ADDRESS_PARSER_MODEL_FILENAME_CRF "address_parser_crf.dat"
#ifdef EXP_VOCAB_DICT
#include "feature_dict.h"
static feature_dict_t *vocab_dict = NULL;
#endif
#ifdef EXP_NORM_TABLE
static int nt_state;
static void nt_init(void);
#endif
#ifdef EXP_INT_KEYS
bool crf_int_keys_enable(crf_t *crf, const feature_dict_t *vocab);   // crf.c, experiment 015
#endif
#ifdef EXP_CANON_CONST
// Experiment 017 step 1: canon() of the two constant placeholder strings, computed once at load (canon depends only on the bytes).
static uint32_t ap_canon_unknown = 0xFFFFFFFFu, ap_canon_unknown_numeric = 0xFFFFFFFFu;
static bool ap_canon_const_ready = false;
#endif
#define ADDRESS_PARSER_VOCAB_FILENAME "address_parser_vocab.trie"
#define ADDRESS_PARSER_PHRASE_FILENAME "address_parser_phrases.dat"
#define ADDRESS_PARSER_POSTAL_CODES_FILENAME "address_parser_postal_codes.dat"

#define UNKNOWN_WORD "UNKNOWN"
#define UNKNOWN_NUMERIC "UNKNOWN_NUMERIC"

#define DEFAULT_RARE_WORD_THRESHOLD 50

static address_parser_t *parser = NULL;

typedef enum {
    ADDRESS_PARSER_NULL_PHRASE,
    ADDRESS_PARSER_DICTIONARY_PHRASE,
    ADDRESS_PARSER_COMPONENT_PHRASE,
    ADDRESS_PARSER_PREFIX_PHRASE,
    ADDRESS_PARSER_SUFFIX_PHRASE
} address_parser_phrase_type_t;

static parser_options_t PARSER_DEFAULT_OPTIONS = {
    .rare_word_threshold = DEFAULT_RARE_WORD_THRESHOLD,
    .print_features = false
};

address_parser_t *address_parser_new_options(parser_options_t options) {
    address_parser_t *parser = calloc(1, sizeof(address_parser_t));
    parser->options = options;
    return parser;
}

address_parser_t *address_parser_new(void) {
    return address_parser_new_options(PARSER_DEFAULT_OPTIONS);
}

address_parser_t *get_address_parser(void) {
    return parser;
}

bool address_parser_print_features(bool print_features) {
    if (parser == NULL) return false;

    parser->options.print_features = print_features;
    return true;
}

bool address_parser_save(address_parser_t *self, char *output_dir) {
    if (self == NULL || output_dir == NULL) return false;

    char *model_filename = NULL;
    if (self->model_type == ADDRESS_PARSER_TYPE_GREEDY_AVERAGED_PERCEPTRON) {
        model_filename = ADDRESS_PARSER_MODEL_FILENAME;
    } else if (self->model_type == ADDRESS_PARSER_TYPE_CRF) {
        model_filename = ADDRESS_PARSER_MODEL_FILENAME_CRF;
    } else {
        return false;
    }

    char_array *path = char_array_new_size(strlen(output_dir));

    char_array_add_joined(path, PATH_SEPARATOR, true, 2, output_dir, model_filename);
    char *model_path = char_array_get_string(path);

    if (self->model_type == ADDRESS_PARSER_TYPE_GREEDY_AVERAGED_PERCEPTRON) {
        if (!averaged_perceptron_save(self->model.ap, model_path)) {
            log_info("Error in averaged_perceptron_save\n");
            char_array_destroy(path);
            return false;
        }
    } else if (self->model_type == ADDRESS_PARSER_TYPE_CRF) {
        if (!crf_save(self->model.crf, model_path)) {
            log_info("Error in crf_save\n");
            char_array_destroy(path);
            return false;
        }
    }

    char_array_clear(path);

    char_array_add_joined(path, PATH_SEPARATOR, true, 2, output_dir, ADDRESS_PARSER_VOCAB_FILENAME);
    char *vocab_path = char_array_get_string(path);

    if (!trie_save(self->vocab, vocab_path)) {
        return false;
    }

    char_array_clear(path);

    char_array_add_joined(path, PATH_SEPARATOR, true, 2, output_dir, ADDRESS_PARSER_PHRASE_FILENAME);
    char *phrases_path = char_array_get_string(path);

    FILE *phrases_file = fopen(phrases_path, "w+");
    if (phrases_file == NULL || self->phrases == NULL) {
        return false;
    }

    if (!trie_write(self->phrases, phrases_file)) {
        return false;
    }

    if (self->phrase_types == NULL) {
        return false;
    }

    size_t num_phrase_types = self->phrase_types->n;
    if (!file_write_uint64(phrases_file, num_phrase_types)) {
        return false;
    }

    for (size_t i = 0; i < self->phrase_types->n; i++) {
        address_parser_types_t phrase_type_value = self->phrase_types->a[i];
        if (!file_write_uint32(phrases_file, phrase_type_value.value)) {
            return false;
        }
    }

    fclose(phrases_file);

    char_array_clear(path);

    char_array_add_joined(path, PATH_SEPARATOR, true, 2, output_dir, ADDRESS_PARSER_POSTAL_CODES_FILENAME);
    char *postal_codes_path = char_array_get_string(path);

    FILE *postal_codes_file = fopen(postal_codes_path, "w+");
    if (postal_codes_file == NULL || self->postal_codes == NULL) {
        return false;
    }

    if (!trie_write(self->postal_codes, postal_codes_file)) {
        return false;
    }

    if (self->postal_code_contexts == NULL) {
        return false;
    }

    if (!graph_write(self->postal_code_contexts, postal_codes_file)) {
        return false;
    }

    fclose(postal_codes_file);

    char_array_destroy(path);

    return true;
}

static bool postal_code_context_exists(address_parser_t *self, uint32_t postal_code_id, uint32_t admin_id) {
    graph_t *g = self->postal_code_contexts;

    return graph_has_edge(g, postal_code_id, admin_id);
}

bool address_parser_load(char *dir) {
    if (parser != NULL) return false;
    if (dir == NULL) {
        dir = LIBPOSTAL_ADDRESS_PARSER_DIR;
    }

    char_array *path = char_array_new_size(strlen(dir));

    char_array_add_joined(path, PATH_SEPARATOR, true, 2, dir, ADDRESS_PARSER_MODEL_FILENAME);
    char *model_path = char_array_get_string(path);

    if (file_exists(model_path)) {
        averaged_perceptron_t *ap_model = averaged_perceptron_load(model_path);
        if (ap_model != NULL) {
            parser = address_parser_new();
            parser->model_type = ADDRESS_PARSER_TYPE_GREEDY_AVERAGED_PERCEPTRON;
            parser->model.ap = ap_model;
        } else {
            char_array_destroy(path);
            log_error("Averaged perceptron model could not be loaded\n");
            return false;
        }
    } else {
        model_path = NULL;
    }

    if (model_path == NULL) {
        char_array_clear(path);
        char_array_add_joined(path, PATH_SEPARATOR, true, 2, dir, ADDRESS_PARSER_MODEL_FILENAME_CRF);
        model_path = char_array_get_string(path);

        if (file_exists(model_path)) {
            crf_t *crf_model = crf_load(model_path);
            if (crf_model != NULL) {
                parser = address_parser_new();
                parser->model_type = ADDRESS_PARSER_TYPE_CRF;
                parser->model.crf = crf_model;
            } else {
                char_array_destroy(path);
                log_error("Averaged perceptron model could not be loaded\n");
                return false;
            }
        } else {
            model_path = NULL;
        }
    }

    if (parser == NULL) {
        char_array_destroy(path);
        log_error("Could not find parser model file of known type\n");
        return false;
    }

    char_array_clear(path);

    char_array_add_joined(path, PATH_SEPARATOR, true, 2, dir, ADDRESS_PARSER_VOCAB_FILENAME);

    char *vocab_path = char_array_get_string(path);

    trie_t *vocab = trie_load(vocab_path);

    if (vocab == NULL) {
        goto exit_address_parser_created;
    }

    parser->vocab = vocab;
#ifdef EXP_VOCAB_DICT
    if (getenv("CRF_DICT_DISABLE") == NULL) {   // experiment switch; any doubt keeps the trie
        feature_dict_t *vd = fd_build_ex(vocab, false, true);
        if (vd != NULL) {
            if (getenv("CRF_DICT_VERBOSE") != NULL) fprintf(stderr, "vocab dictionary: keys=%llu build=%.2fs dict=%.1f MB replaces trie=%.1f MB\n", (unsigned long long)vd->num_keys, vd->build_seconds,
                (vd->size * 8 + vd->arena_used) / 1048576.0, (vocab->nodes->n * sizeof(trie_node_t) + vocab->tail->n + vocab->data->n * sizeof(trie_data_node_t)) / 1048576.0);
            vocab_dict = vd;
            trie_destroy(vocab);
            parser->vocab = NULL;
        }
    }
#endif
#ifdef EXP_INT_KEYS
    if (vocab_dict != NULL && parser->model_type == ADDRESS_PARSER_TYPE_CRF) {
        crf_int_keys_enable(parser->model.crf, vocab_dict);
#ifdef EXP_CANON_CONST
        ap_canon_unknown = fd_canon(vocab_dict, UNKNOWN_WORD, strlen(UNKNOWN_WORD));
        ap_canon_unknown_numeric = fd_canon(vocab_dict, UNKNOWN_NUMERIC, strlen(UNKNOWN_NUMERIC));
        ap_canon_const_ready = true;
#endif
    }
#endif
#ifdef EXP_NORM_TABLE
    if (nt_state == 0) nt_init();   // at setup, not in the first parse
#endif

    char_array_clear(path);

    char_array_add_joined(path, PATH_SEPARATOR, true, 2, dir, ADDRESS_PARSER_PHRASE_FILENAME);
    char *phrases_path = char_array_get_string(path);

    FILE *phrases_file = fopen(phrases_path, "rb");
    if (phrases_file == NULL) {
        goto exit_address_parser_created;
    }

    parser->phrases = trie_read(phrases_file);
    if (parser->phrases == NULL) {
        goto exit_address_parser_created;
    }

    uint64_t num_phrase_types;

    if (!file_read_uint64(phrases_file, &num_phrase_types)) {
        goto exit_address_parser_created;
    }

    parser->phrase_types = address_parser_types_array_new_size(num_phrase_types);

    uint32_array *phrase_type_values = uint32_array_new_size(num_phrase_types);
    if (!file_read_uint32_array(phrases_file, phrase_type_values->a, num_phrase_types)) {
        uint32_array_destroy(phrase_type_values);
        goto exit_address_parser_created;
    }
    phrase_type_values->n = num_phrase_types;

    for (size_t i = 0; i < phrase_type_values->n; i++) {
        uint32_t phrase_type_value = phrase_type_values->a[i];
        address_parser_types_t phrase_type = {.value = phrase_type_value};
        address_parser_types_array_push(parser->phrase_types, phrase_type);
    }

    uint32_array_destroy(phrase_type_values);

    fclose(phrases_file);

    char_array_clear(path);

    char_array_add_joined(path, PATH_SEPARATOR, true, 2, dir, ADDRESS_PARSER_POSTAL_CODES_FILENAME);

    char *postal_codes_path = char_array_get_string(path);

    FILE *postal_codes_file = fopen(postal_codes_path, "rb");
    if (postal_codes_file == NULL) {
        goto exit_address_parser_created;
    }

    parser->postal_codes = trie_read(postal_codes_file);
    if (parser->postal_codes == NULL) {
        goto exit_address_parser_created;
    }

    parser->postal_code_contexts = graph_read(postal_codes_file);

    if (parser->postal_code_contexts == NULL) {
        goto exit_address_parser_created;
    }

    fclose(postal_codes_file);

    parser->context = address_parser_context_new();
    if (parser->context == NULL) {
        goto exit_address_parser_created;
    }

    char_array_destroy(path);
    return true;

exit_address_parser_created:
    address_parser_destroy(parser);
    char_array_destroy(path);
    return false;
}

void address_parser_destroy(address_parser_t *self) {
    if (self == NULL) return;

    if (self->model_type == ADDRESS_PARSER_TYPE_GREEDY_AVERAGED_PERCEPTRON && self->model.ap != NULL) {
        averaged_perceptron_destroy(self->model.ap);
    } else if (self->model_type == ADDRESS_PARSER_TYPE_CRF && self->model.crf != NULL) {
        crf_destroy(self->model.crf);
    }

    if (self->context != NULL) {
        address_parser_context_destroy(self->context);
    }

#ifdef EXP_VOCAB_DICT
    if (self == parser) { fd_destroy(vocab_dict); vocab_dict = NULL; }
#endif
    if (self->vocab != NULL) {
        trie_destroy(self->vocab);
    }

    if (self->phrases != NULL) {
        trie_destroy(self->phrases);
    }

    if (self->phrase_types != NULL) {
        address_parser_types_array_destroy(self->phrase_types);
    }

    if (self->postal_codes != NULL) {
        trie_destroy(self->postal_codes);
    }

    if (self->postal_code_contexts != NULL) {
        graph_destroy(self->postal_code_contexts);
    }

    free(self);
}

static inline uint32_t word_vocab_frequency(address_parser_t *parser, char *word) {   
    uint32_t count = 0;
#ifdef EXP_VOCAB_DICT
    if (vocab_dict != NULL) {
        if (!fd_lookup(vocab_dict, word, strlen(word), &count)) count = 0;
        return count;
    }
#endif
    bool has_key = trie_get_data(parser->vocab, word, &count);
    return count;
}

inline void address_parser_normalize_token(cstring_array *array, char *str, token_t token) {
    normalize_token(array, str, token, ADDRESS_PARSER_NORMALIZE_TOKEN_OPTIONS);
}

static inline void address_parser_normalize_phrase_token(cstring_array *array, char *str, token_t token) {
    normalize_token(array, str, token, ADDRESS_PARSER_NORMALIZE_ADMIN_TOKEN_OPTIONS);
}

#ifdef EXP_ASCII_NORM
// Experiment 011. For a string made only of bytes 0x20-0x7E other than '&', normalize_string_latin(COMPOSE|LOWERCASE|SIMPLE_LATIN_ASCII) is exactly ASCII lowercasing:
//  - the transliterator "latin-ascii-simple" has two steps; "html-escape" has 65,788 rules that ALL start with '&', and ruleset "1" has 138 rules whose first bytes are all
//    UTF-8 lead bytes (0xC2,0xC3,0xCB,0xE2,0xE3,0xEF), so no rule can start at any other ASCII byte and transliteration leaves the string unchanged;
//  - the UTF-8 step (compose + lowercase) maps A-Z to a-z and leaves other ASCII unchanged.
// Verified by differential test against the library (839,514 exhaustive short strings, 60M random strings, corpus lines; 0 mismatches). Anything else falls back to the library.
static inline char *address_parser_normalize_ascii_fast(char *str) {
    size_t len = strlen(str);
    if (len == 0) return NULL;
#ifdef EXP_ASCII_BORROW
    bool changed = false;
#endif
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c < 0x20 || c > 0x7e || c == '&') return NULL;
#ifdef EXP_ASCII_BORROW
        changed |= c >= 'A' && c <= 'Z';
#endif
    }
#ifdef EXP_ASCII_BORROW
    // The parser already treats a NULL normalization result as a borrowed input.
    if (!changed) return str;
#endif
    char *out = malloc(len + 1);
    if (out == NULL) return NULL;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        out[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    out[len] = '\0';
    return out;
}
#endif

#ifdef EXP_NORM_TABLE
// Experiment 016 T5 (N5/N6). normalize_string_latin(COMPOSE|LOWERCASE|SIMPLE_LATIN_ASCII) = NFC-map(IGNORE, NLF2LF, STRIPCC) of the per-codepoint lowercase of
// transliterate("latin-ascii-simple", s). For a valid UTF-8 string with no '&' and none of the 138 step-1 rule codepoints, transliteration is the identity
// (T4). If in addition every codepoint is "context-free" — it, its lowercase and the first codepoint of that lowercase's canonical decomposition are
// starters (ccc 0) that cannot be the second element of a composition, not ignorable, not controls/format/separators/unassigned/surrogates/private,
// and not Hangul (algorithmic composition) — then nothing composes, reorders, is stripped or converted across codepoint boundaries, and the result is
// the concatenation of each codepoint's own result. Each codepoint's result is computed at load by the library itself (normalize_string_latin).
int transliterate_simple_identity_ready(void);
int transliterate_simple_cp_has_rule(int32_t cp);
typedef struct { uint8_t len; char b[7]; } nt_entry_t;
static nt_entry_t *nt_table = NULL;
static int nt_state = 0;
int norm_table_test_disable = 0;
static bool nt_bad_cat(int cat) {
    return cat == UTF8PROC_CATEGORY_CN || cat == UTF8PROC_CATEGORY_CC || cat == UTF8PROC_CATEGORY_CF || cat == UTF8PROC_CATEGORY_CS ||
           cat == UTF8PROC_CATEGORY_CO || cat == UTF8PROC_CATEGORY_ZL || cat == UTF8PROC_CATEGORY_ZP;
}
static bool nt_hangul(int32_t c) { return (c >= 0x1100 && c < 0x1200) || (c >= 0x3130 && c < 0x3190) || (c >= 0xA960 && c < 0xA980) || (c >= 0xAC00 && c < 0xD800); }
static bool nt_starter_ok(int32_t u) {
    if (u < 0 || nt_hangul(u)) return false;
    const utf8proc_property_t *p = utf8proc_get_property(u);
    return p->combining_class == 0 && p->comb2nd_index < 0 && !p->ignorable && !p->control_boundary && !nt_bad_cat(p->category);
}
static void nt_init(void) {
    nt_state = -1;
    if (getenv("NORM_TABLE_DISABLE") != NULL || !transliterate_simple_identity_ready()) return;
    for (int32_t c = 0x20; c < 0x7F; c++) if (!nt_starter_ok(c) || !nt_starter_ok(utf8proc_tolower(c))) return;   // the ASCII part of the argument
    nt_entry_t *t = calloc(65536, sizeof *t); if (t == NULL) return;
    uint32_t n_ok = 0;
    for (int32_t c = 0x80; c < 65536; c++) {
#ifdef EXP_NORM_HANGUL
        if (c >= 0xAC00 && c <= 0xD7A3) {   // syllables: the generic starter test would reject them via nt_hangul; checked separately below
            const utf8proc_property_t *hp = utf8proc_get_property(c);
            if (transliterate_simple_cp_has_rule(c) || hp->combining_class != 0 || hp->comb2nd_index >= 0 || hp->ignorable || nt_bad_cat(hp->category)) continue;
        } else
#endif
        if ((c >= 0xD800 && c < 0xE000) || transliterate_simple_cp_has_rule(c) || !nt_starter_ok(c)) continue;
        int32_t L = utf8proc_tolower(c);
#ifdef EXP_NORM_HANGUL
        // Experiment 022: precomposed Hangul syllables U+AC00..U+D7A3. NFC can change one only by appending a FOLLOWING trailing jamo (LV + T), and every
        // jamo stays excluded (nt_hangul), so a fast-path string never contains one; nothing composes backward into a syllable (its decomposition starts with
        // a leading jamo, which is never the second element of a composition). Lowercasing leaves Hangul unchanged. The result is still computed by the library.
        bool hangul_syllable = c >= 0xAC00 && c <= 0xD7A3 && L == c;
#else
        bool hangul_syllable = false;
#endif
        if (!hangul_syllable) {
            if (!nt_starter_ok(L)) continue;
            int32_t dec[32]; int bc = 0;
            ssize_t nd = utf8proc_decompose_char(L, dec, 32, UTF8PROC_DECOMPOSE, &bc);
            if (nd <= 0 || nd > 32 || !nt_starter_ok(dec[0])) continue;
        }
        uint8_t enc[8]; ssize_t el = utf8proc_encode_char(c, enc); if (el <= 0) continue; enc[el] = 0;
        char *r = normalize_string_latin((char *)enc, (size_t)el, ADDRESS_PARSER_NORMALIZE_STRING_OPTIONS);
        if (r == NULL) continue;
        size_t rl = strlen(r);
        if (rl > 0 && rl <= 7) { t[c].len = (uint8_t)rl; memcpy(t[c].b, r, rl); n_ok++; }
        free(r);
    }
    nt_table = t; nt_state = 1;
    if (getenv("NORM_TABLE_VERBOSE") != NULL) fprintf(stderr, "norm table: %u context-free BMP codepoints (%.1f KB)\n", n_ok, 65536 * sizeof *t / 1024.0);
}
// Returns the normalized string, or NULL if any codepoint is not in the table (the caller then runs the library).
char *norm_table_fast(const char *str) {
    if (norm_table_test_disable) return NULL;
    if (nt_state == 0) nt_init();
    if (nt_state != 1) return NULL;
    size_t len = strlen(str);
    if (len == 0) return NULL;
    char *out = malloc(len * 4 + 1);   // a non-ASCII codepoint takes >= 2 input bytes and yields <= 7 output bytes
    if (out == NULL) return NULL;
    size_t o = 0, i = 0;
    const uint8_t *p = (const uint8_t *)str;
    while (i < len) {
        uint8_t b = p[i];
        if (b < 0x80) {
            if (b < 0x20 || b > 0x7E || b == '&') { free(out); return NULL; }
            out[o++] = (b >= 'A' && b <= 'Z') ? (char)(b + 32) : (char)b; i++;
            continue;
        }
        int32_t cp; ssize_t l = utf8proc_iterate(p + i, (ssize_t)(len - i), &cp);
        if (l <= 0 || cp >= 65536 || nt_table[cp].len == 0) { free(out); return NULL; }
        memcpy(out + o, nt_table[cp].b, nt_table[cp].len); o += nt_table[cp].len; i += (size_t)l;
    }
    out[o] = '\0';
    return out;
}
#endif

inline char *address_parser_normalize_string(char *str) {
#ifdef EXP_ASCII_NORM
    char *fast = address_parser_normalize_ascii_fast(str);
    if (fast != NULL) {
#ifdef EXP_ASCII_BORROW
        return fast == str ? NULL : fast;
#else
        return fast;
#endif
    }
#endif
#ifdef EXP_NORM_TABLE
    char *tfast = norm_table_fast(str);
    if (tfast != NULL) return tfast;
#endif
    return normalize_string_latin(str, strlen(str), ADDRESS_PARSER_NORMALIZE_STRING_OPTIONS);
}


void address_parser_context_destroy(address_parser_context_t *self) {
    if (self == NULL) return;

    if (self->phrase != NULL) {
        char_array_destroy(self->phrase);
    }

    if (self->context_phrase != NULL) {
        char_array_destroy(self->context_phrase);
    }

    if (self->long_context_phrase != NULL) {
        char_array_destroy(self->long_context_phrase);
    }

    if (self->component_phrase != NULL) {
        char_array_destroy(self->component_phrase);
    }

    if (self->context_component_phrase != NULL) {
        char_array_destroy(self->context_component_phrase);
    }

    if (self->long_context_component_phrase != NULL) {
        char_array_destroy(self->long_context_component_phrase);
    }

    if (self->prefix_phrase != NULL) {
        char_array_destroy(self->prefix_phrase);
    }

    if (self->context_prefix_phrase != NULL) {
        char_array_destroy(self->context_prefix_phrase);
    }

    if (self->long_context_prefix_phrase != NULL) {
        char_array_destroy(self->long_context_prefix_phrase);
    }

    if (self->suffix_phrase != NULL) {
        char_array_destroy(self->suffix_phrase);
    }

    if (self->context_suffix_phrase != NULL) {
        char_array_destroy(self->context_suffix_phrase);
    }

    if (self->long_context_suffix_phrase != NULL) {
        char_array_destroy(self->long_context_suffix_phrase);
    }

    if (self->ngrams != NULL) {
        cstring_array_destroy(self->ngrams);
    }

    if (self->sub_token != NULL) {
        char_array_destroy(self->sub_token);
    }

    if (self->sub_tokens != NULL) {
        token_array_destroy(self->sub_tokens);
    }

    if (self->separators != NULL) {
        uint32_array_destroy(self->separators);
    }

    if (self->normalized != NULL) {
        cstring_array_destroy(self->normalized);
    }

    if (self->normalized_tokens != NULL) {
        token_array_destroy(self->normalized_tokens);
    }

    if (self->normalized_admin != NULL) {
        cstring_array_destroy(self->normalized_admin);
    }

    if (self->normalized_admin_tokens != NULL) {
        token_array_destroy(self->normalized_admin_tokens);
    }

    if (self->features != NULL) {
        cstring_array_destroy(self->features);
    }

    if (self->prev_tag_features != NULL) {
        cstring_array_destroy(self->prev_tag_features);
    }

    if (self->prev2_tag_features != NULL) {
        cstring_array_destroy(self->prev2_tag_features);
    }

    if (self->tokenized_str != NULL) {
        tokenized_string_destroy(self->tokenized_str);
    }

    if (self->address_dictionary_phrases != NULL) {
        phrase_array_destroy(self->address_dictionary_phrases);
    }

    if (self->address_phrase_memberships != NULL) {
        int64_array_destroy(self->address_phrase_memberships);
    }

    if (self->component_phrases != NULL) {
        phrase_array_destroy(self->component_phrases);
    }

    if (self->component_phrase_memberships != NULL) {
        int64_array_destroy(self->component_phrase_memberships);
    }

    if (self->postal_code_phrases != NULL) {
        phrase_array_destroy(self->postal_code_phrases);
    }

    if (self->postal_code_phrase_memberships != NULL) {
        int64_array_destroy(self->postal_code_phrase_memberships);
    }

    if (self->prefix_phrases != NULL) {
        phrase_array_destroy(self->prefix_phrases);
    }

    if (self->suffix_phrases != NULL) {
        phrase_array_destroy(self->suffix_phrases);
    }

    free(self);
}

address_parser_context_t *address_parser_context_new(void) {
    address_parser_context_t *context = malloc(sizeof(address_parser_context_t));

    if (context == NULL) return NULL;

    context->language = NULL;
    context->country = NULL;

    context->phrase = char_array_new();
    if (context->phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->context_phrase = char_array_new();
    if (context->context_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->long_context_phrase = char_array_new();
    if (context->long_context_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->component_phrase = char_array_new();
    if (context->component_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->context_component_phrase = char_array_new();
    if (context->context_component_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->long_context_component_phrase = char_array_new();
    if (context->long_context_component_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->prefix_phrase = char_array_new();
    if (context->prefix_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->context_prefix_phrase = char_array_new();
    if (context->context_prefix_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->long_context_prefix_phrase = char_array_new();
    if (context->long_context_prefix_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->suffix_phrase = char_array_new();
    if (context->suffix_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->context_suffix_phrase = char_array_new();
    if (context->context_suffix_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->long_context_suffix_phrase = char_array_new();
    if (context->long_context_suffix_phrase == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->ngrams = cstring_array_new();
    if (context->ngrams == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->sub_token = char_array_new();
    if (context->sub_token == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->sub_tokens = token_array_new();
    if (context->sub_tokens == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->separators = uint32_array_new();
    if (context->separators == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->normalized = cstring_array_new();
    if (context->normalized == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->normalized_tokens = token_array_new();
    if (context->normalized_tokens == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->normalized_admin = cstring_array_new();
    if (context->normalized_admin == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->normalized_admin_tokens = token_array_new();
    if (context->normalized_admin_tokens == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->features = cstring_array_new();
    if (context->features == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->prev_tag_features = cstring_array_new();
    if (context->prev_tag_features == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->prev2_tag_features = cstring_array_new();
    if (context->prev2_tag_features == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->tokenized_str = tokenized_string_new();
    if (context->tokenized_str == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->address_dictionary_phrases = phrase_array_new();
    if (context->address_dictionary_phrases == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->address_phrase_memberships = int64_array_new();
    if (context->address_phrase_memberships == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->component_phrases = phrase_array_new();
    if (context->component_phrases == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->component_phrase_memberships = int64_array_new();
    if (context->component_phrase_memberships == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->postal_code_phrases = phrase_array_new();
    if (context->postal_code_phrases == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->postal_code_phrase_memberships = int64_array_new();
    if (context->postal_code_phrase_memberships == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->prefix_phrases = phrase_array_new();
    if (context->prefix_phrases == NULL) {
        goto exit_address_parser_context_allocated;
    }

    context->suffix_phrases = phrase_array_new();
    if (context->suffix_phrases == NULL) {
        goto exit_address_parser_context_allocated;
    }

    return context;

exit_address_parser_context_allocated:
    address_parser_context_destroy(context);
    return NULL;
}

bool is_valid_component_phrase(cstring_array *strings, phrase_t phrase) {
    bool valid = false;
    for (uint32_t i = phrase.start; i < phrase.start + phrase.len; i++) {
        char *s = cstring_array_get_string(strings, i);
        if (!string_is_digit(s, strlen(s))) {
            valid = true;
            break;
        }
    }
    return valid;
}

void address_parser_context_fill(address_parser_context_t *context, address_parser_t *parser, tokenized_string_t *tokenized_str, char *language, char *country) {
    uint32_t token_index;
    char *word;
    phrase_t phrase;

    context->language = language;
    context->country = country;

    cstring_array *normalized = context->normalized;
    token_array *normalized_tokens = context->normalized_tokens;
    cstring_array_clear(normalized);
    token_array_clear(normalized_tokens);

    cstring_array *normalized_admin = context->normalized_admin;
    token_array *normalized_admin_tokens = context->normalized_admin_tokens;
    cstring_array_clear(normalized_admin);
    token_array_clear(normalized_admin_tokens);

    char *str = tokenized_str->str;
    token_array *tokens = tokenized_str->tokens;

    cstring_array_foreach(tokenized_str->strings, token_index, word, {
        token_t token = tokens->a[token_index];

        size_t token_offset = normalized->str->n;
        address_parser_normalize_token(normalized, str, token);
        size_t token_len;
        if (normalized->str->n > token_offset) {
           token_len = normalized->str->n - 1 - token_offset;
        } else {
            token_len = 0;
        }
        token_t normalized_token;
        normalized_token.offset = token_offset;
        normalized_token.len = token_len;
        normalized_token.type = token.type;
        token_array_push(normalized_tokens, normalized_token);

        size_t admin_token_offset = normalized_admin->str->n;
#ifdef EXP_ADMIN_SHARE
        // The admin options equal the standard token options minus NORMALIZE_TOKEN_REPLACE_DIGITS, which only acts on digit characters (normalize.c). For an
        // all-ASCII token without digits both normalizations therefore produce identical bytes: reuse the first result (differential test: 0 mismatches).
        bool share_admin = true;
        for (size_t b = 0; b < token.len; b++) {
            unsigned char c = (unsigned char)(str + token.offset)[b];
            if (c >= 0x80 || (c >= '0' && c <= '9')) { share_admin = false; break; }
        }
        if (share_admin) {
            cstring_array_start_token(normalized_admin);
            char_array_append_len(normalized_admin->str, normalized->str->a + token_offset, normalized->str->n - 1 - token_offset);
            char_array_terminate(normalized_admin->str);
        } else
#endif
        address_parser_normalize_phrase_token(normalized_admin, str, token);
        size_t admin_token_len;
        if (normalized_admin->str->n > admin_token_offset) {
           admin_token_len = normalized_admin->str->n - 1 - admin_token_offset;
        } else {
            admin_token_len = 0;
        }
        token_t normalized_admin_token;
        normalized_admin_token.offset = admin_token_offset;
        normalized_admin_token.len = admin_token_len;
        normalized_admin_token.type = token.type;
        token_array_push(normalized_admin_tokens, normalized_admin_token);
    })

    char *normalized_str = normalized->str->a;
    char *normalized_str_admin = normalized_admin->str->a;

    /*
    Address dictionary phrases
    --------------------------
    Recognizing phrases that occur in libpostal's dictionaries.

    Note: if the dictionaries are updates to try to improve the parser,
    we'll need to retrain. This can be done without rebuilding the
    training data (a long-running process which can take up to a week),
    but will require running address_parser_train, the main training script.
    */

    phrase_array_clear(context->address_dictionary_phrases);
    int64_array_clear(context->address_phrase_memberships);

    phrase_array *address_dictionary_phrases = context->address_dictionary_phrases;
    int64_array *address_phrase_memberships = context->address_phrase_memberships;

    size_t num_tokens = tokens->n;

    bool have_address_phrases = search_address_dictionaries_tokens_with_phrases(normalized_str, normalized_tokens, NULL, &address_dictionary_phrases);
    token_phrase_memberships(address_dictionary_phrases, address_phrase_memberships, num_tokens);

    phrase_array_clear(context->prefix_phrases);
    phrase_array_clear(context->suffix_phrases);

    for (size_t i = 0; i < num_tokens; i++) {
        token_t token = tokens->a[i];
        char *word_pre_norm = tokenized_string_get_token(tokenized_str, i);

        phrase_t prefix_phrase = search_address_dictionaries_prefix(word_pre_norm, token.len, NULL);
        phrase_array_push(context->prefix_phrases, prefix_phrase);

        phrase_t suffix_phrase = search_address_dictionaries_suffix(word_pre_norm, token.len, NULL);
        phrase_array_push(context->suffix_phrases, suffix_phrase);
    }

    /*
    Component phrases
    -----------------
    Precomputed phrases for cities, states, countries, etc. from the training data

    Note: if the training data has lots of mislabeled examples (e.g. Brooklyn as city
    instead of a city_district), this may cause the parser to get confused. It will
    penalize itself for getting the wrong answer when really the underlying data
    is simply ambiguous. In the OSM training data a lot of work has been done to
    ensure that there's little or no systematic mislabeling. As such, other data
    sets shouldn't be added willy-nilly unless the labels are consistent.
    */

    phrase_array_clear(context->component_phrases);
    int64_array_clear(context->component_phrase_memberships);

    phrase_array *component_phrases = context->component_phrases;
    int64_array *component_phrase_memberships = context->component_phrase_memberships;

    bool have_component_phrases = trie_search_tokens_with_phrases(parser->phrases, normalized_str_admin, normalized_admin_tokens, &component_phrases);
    token_phrase_memberships(component_phrases, component_phrase_memberships, num_tokens);

    for (size_t i = 0; i < component_phrases->n; i++) {
        phrase_t phrase = component_phrases->a[i];
        if (!is_valid_component_phrase(context->normalized_admin, phrase)) {
            for (size_t j = phrase.start; j < phrase.start + phrase.len; j++) {
                component_phrase_memberships->a[j] = NULL_PHRASE_MEMBERSHIP;
            }
        }
    }

    phrase_array_clear(context->postal_code_phrases);
    int64_array_clear(context->postal_code_phrase_memberships);

    phrase_array *postal_code_phrases = context->postal_code_phrases;
    int64_array *postal_code_phrase_memberships = context->postal_code_phrase_memberships;

    bool have_postal_code_phrases = trie_search_tokens_with_phrases(parser->postal_codes, normalized_str_admin, normalized_admin_tokens, &postal_code_phrases);
    token_phrase_memberships(postal_code_phrases, postal_code_phrase_memberships, num_tokens);

}

static inline phrase_t phrase_at_index(phrase_array *phrases, int64_array *phrase_memberships, uint32_t i) {
    if (phrases == NULL || phrase_memberships == NULL || i > phrase_memberships->n - 1) {
        return NULL_PHRASE;
    }

    int64_t phrase_index = phrase_memberships->a[i];
    if (phrase_index != NULL_PHRASE_MEMBERSHIP) {
        phrase_t phrase = phrases->a[phrase_index];
        return phrase;
    }

    return NULL_PHRASE;
}

char *phrase_prefix(char *word, size_t len, phrase_t prefix_phrase, char_array *prefix_phrase_array) {
    char_array_clear(prefix_phrase_array);
    size_t prefix_len = prefix_phrase.len;
    char_array_add_len(prefix_phrase_array, word, prefix_len);
    char *prefix = char_array_get_string(prefix_phrase_array);
    return prefix;
}

char *phrase_suffix(char *word, size_t len, phrase_t suffix_phrase, char_array *suffix_phrase_array) {
    char_array_clear(suffix_phrase_array);
    size_t suffix_len = suffix_phrase.len;
    char_array_add_len(suffix_phrase_array, word + (len - suffix_len), suffix_len);
    char *suffix = char_array_get_string(suffix_phrase_array);
    return suffix;
}

bool is_valid_dictionary_phrase(phrase_t phrase) {
    uint32_t expansion_index = phrase.data;
    address_expansion_value_t *expansion_value = address_dictionary_get_expansions(expansion_index);

    if (expansion_value == NULL) {
        log_warn("expansion_value is NULL for index %u\n", expansion_index);
        return false;
    }
    uint32_t address_phrase_types = expansion_value->components;

    if (address_phrase_types & (LIBPOSTAL_ADDRESS_STREET | LIBPOSTAL_ADDRESS_HOUSE_NUMBER | LIBPOSTAL_ADDRESS_NAME | LIBPOSTAL_ADDRESS_CATEGORY | LIBPOSTAL_ADDRESS_NEAR | LIBPOSTAL_ADDRESS_UNIT | LIBPOSTAL_ADDRESS_LEVEL | LIBPOSTAL_ADDRESS_ENTRANCE | LIBPOSTAL_ADDRESS_STAIRCASE | LIBPOSTAL_ADDRESS_POSTAL_CODE | LIBPOSTAL_ADDRESS_PO_BOX)) {
        for (size_t i = 0; i < expansion_value->expansions->n; i++) {
            address_expansion_t expansion = expansion_value->expansions->a[i];
            if (!address_expansion_in_dictionary(expansion, DICTIONARY_TOPONYM)) {
                return true;
            }
        }
    }

    return false;
}

typedef struct address_parser_phrase {
    char *str;
    address_parser_phrase_type_t type;
    phrase_t phrase;
} address_parser_phrase_t;

static inline bool is_plain_word_phrase_type(address_parser_phrase_type_t type) {
    return type == ADDRESS_PARSER_NULL_PHRASE || type == ADDRESS_PARSER_SUFFIX_PHRASE || type == ADDRESS_PARSER_PREFIX_PHRASE;
}

static address_parser_phrase_t word_or_phrase_at_index(address_parser_t *parser, tokenized_string_t *tokenized, address_parser_context_t *context, uint32_t i, bool long_context) {
    phrase_t phrase;
    address_parser_phrase_t response;
    char *phrase_string = NULL;

    phrase = phrase_at_index(context->address_dictionary_phrases, context->address_phrase_memberships, i);
    
    phrase_t component_phrase = phrase_at_index(context->component_phrases, context->component_phrase_memberships, i);

    if (phrase.len > 0 && is_valid_dictionary_phrase(phrase) && component_phrase.len <= phrase.len) {
        phrase_string = cstring_array_get_phrase(context->normalized, long_context ? context->long_context_phrase : context->context_phrase, phrase),

        response = (address_parser_phrase_t){
            phrase_string,
            ADDRESS_PARSER_DICTIONARY_PHRASE,
            phrase
        };
        return response;
    }

    phrase = component_phrase;

    if (phrase.len > 0) {
        phrase_string = cstring_array_get_phrase(context->normalized_admin, long_context ? context->long_context_component_phrase : context->context_component_phrase, phrase);

        response = (address_parser_phrase_t){
            phrase_string,
            ADDRESS_PARSER_COMPONENT_PHRASE,
            phrase
        };
        return response;
    }

    phrase_t prefix_phrase = context->prefix_phrases->a[i];
    phrase_t suffix_phrase = context->suffix_phrases->a[i];

    uint32_t expansion_index;
    address_expansion_value_t *expansion_value;

    cstring_array *normalized = context->normalized;

    char *word = cstring_array_get_string(normalized, i);
    token_t token = tokenized->tokens->a[i];

    // Suffixes like straße, etc.
    if (suffix_phrase.len > 0) {
        expansion_index = suffix_phrase.data;
        expansion_value = address_dictionary_get_expansions(expansion_index);

        if (expansion_value->components & LIBPOSTAL_ADDRESS_STREET) {
            response = (address_parser_phrase_t){
                word,
                ADDRESS_PARSER_SUFFIX_PHRASE,
                suffix_phrase
            };
            return response;
        }
    }

    // Prefixes like hinter, etc.
    if (prefix_phrase.len > 0) {
        expansion_index = prefix_phrase.data;
        expansion_value = address_dictionary_get_expansions(expansion_index);

        // Don't include elisions like l', d', etc. which are in the LIBPOSTAL_ADDRESS_ANY category
        if (expansion_value->components ^ LIBPOSTAL_ADDRESS_ANY) {
            response = (address_parser_phrase_t){
                word,
                ADDRESS_PARSER_PREFIX_PHRASE,
                prefix_phrase
            };
            return response;
        }
    }

    response = (address_parser_phrase_t){
        word,
        ADDRESS_PARSER_NULL_PHRASE,
        NULL_PHRASE
    };
    return response;

}

#ifdef EXP_WORD_MEMO
// Experiment 016 T7 (F1). word_or_phrase_at_index(..., long_context = false) depends only on the address's phrase arrays and normalized tokens, which
// do not change while one address's features are generated, but it is called twice per token (as the next word of t-1 and the previous word of t+1).
// The first result per token is kept. Plain-word results point at the token's normalized string (kept as the same pointer); phrase results point into
// scratch buffers that the next call overwrites, so their bytes are copied into a buffer owned by this memo entry. Only used in accumulate mode.
typedef struct { address_parser_phrase_t r; uint8_t filled; char_array *buf; uint32_t canon[2]; uint8_t canon_ok[2]; } ap_wm_t;   // canon[nonlast]: EXP_WM_CANON
static ap_wm_t *ap_wm = NULL;
static size_t ap_wm_cap = 0, ap_wm_n = 0;
static bool ap_wm_on = false;
static void ap_wm_reset(size_t n) {
    if (n > ap_wm_cap) {
        size_t m = ap_wm_cap ? ap_wm_cap : 64; while (m < n) m *= 2;
        ap_wm_t *p = realloc(ap_wm, m * sizeof *p);
        if (p == NULL) { ap_wm_on = false; return; }
        memset(p + ap_wm_cap, 0, (m - ap_wm_cap) * sizeof *p);
        ap_wm = p; ap_wm_cap = m;
    }
    for (size_t k = 0; k < n; k++) ap_wm[k].filled = 0;
    ap_wm_n = n; ap_wm_on = true;
}
static address_parser_phrase_t word_or_phrase_at_index(address_parser_t *parser, tokenized_string_t *tokenized, address_parser_context_t *context, uint32_t i, bool long_context);
static address_parser_phrase_t word_or_phrase_memo(address_parser_t *parser, tokenized_string_t *tokenized, address_parser_context_t *context, uint32_t i) {
    if (!ap_wm_on || i >= ap_wm_n) return word_or_phrase_at_index(parser, tokenized, context, i, false);
    ap_wm_t *e = &ap_wm[i];
    if (e->filled) return e->r;
    address_parser_phrase_t r = word_or_phrase_at_index(parser, tokenized, context, i, false);
    if (r.type == ADDRESS_PARSER_DICTIONARY_PHRASE || r.type == ADDRESS_PARSER_COMPONENT_PHRASE) {
        if (e->buf == NULL) e->buf = char_array_new();
        if (e->buf == NULL) return r;              // no memo for this entry
        char_array_clear(e->buf);
        char_array_add(e->buf, r.str);
        r.str = char_array_get_string(e->buf);
    }
    e->r = r; e->filled = 1; e->canon_ok[0] = e->canon_ok[1] = 0;
    return r;
}
#define AP_WORD_OR_PHRASE(i) word_or_phrase_memo(parser, tokenized, context, (i))
#else
#define AP_WORD_OR_PHRASE(i) word_or_phrase_at_index(parser, tokenized, context, (i), false)
#endif

static inline int64_t phrase_index(int64_array *phrase_memberships, size_t start, int8_t direction) {
    if (phrase_memberships == NULL) {
        return -1;
    }

    int64_t *memberships = phrase_memberships->a;
    int64_t membership;

    if (direction == -1) {
        for (ssize_t idx = start; idx >= 0; idx--) {
            if (memberships[idx] != NULL_PHRASE_MEMBERSHIP) {
                return (int64_t)idx;
            }
        }
    } else if (direction == 1) {
        size_t n = phrase_memberships->n;
        for (size_t idx = start; idx < n; idx++) {
            if (memberships[idx] != NULL_PHRASE_MEMBERSHIP) {
                return (int64_t)idx;
            }
        }
    }

    return -1;
}


static inline int64_t next_numeric_token_index(tokenized_string_t *tokenized, address_parser_context_t *context, size_t start) {
    if (context == NULL) return -1;

    token_array *tokens = tokenized->tokens;

    if (tokens == NULL || start > tokens->n - 1) return -1;

    phrase_t phrase;

    for (size_t i = start; i < tokens->n; i++) {
        if (context->address_phrase_memberships->a[i] == NULL_PHRASE_MEMBERSHIP &&
            context->component_phrase_memberships->a[i] == NULL_PHRASE_MEMBERSHIP) {
            token_t token = tokens->a[i];
            if (token.type != NUMERIC && token.type != IDEOGRAPHIC_NUMBER) {
                return i;
            }
        }
    }

    return -1;
}


static inline void add_phrase_features(cstring_array *features, uint32_t phrase_types, uint32_t component, char *phrase_type, char *phrase_string) {
    if (phrase_types == component) {
        log_debug("phrase=%s, phrase_types=%d\n", phrase_string, phrase_types);
        feature_array_add(features, 2, "unambiguous phrase type", phrase_type);
        feature_array_add(features, 3, "unambiguous phrase type+phrase", phrase_type, phrase_string);
    } else if (phrase_types & component) {
        feature_array_add(features, 3, "phrase type+phrase", phrase_type, phrase_string);
    }
}

static bool add_ngram_features(cstring_array *features, char *feature_prefix, cstring_array *ngrams, char *str, size_t n, size_t prefix_len, size_t suffix_len) {
    if (features == NULL || ngrams == NULL) return false;

    size_t len = strlen(str);

    if (n == 0 || n > len - 1) return false;

    size_t ngram_num_chars_len = INT64_MAX_STRING_SIZE;
    char ngram_num_chars[ngram_num_chars_len];
    sprintf(ngram_num_chars, "%zu", n);

    bool known_prefix = prefix_len > 0;
    bool known_suffix = suffix_len > 0;

    cstring_array_clear(ngrams);
    if (!add_ngrams(ngrams, n, str + prefix_len, len - suffix_len - prefix_len, !known_prefix, !known_suffix)) {
        return false;
    }
    
    uint32_t idx;
    char *ngram;

    if (feature_prefix != NULL) {
        cstring_array_foreach(ngrams, idx, ngram, {
            feature_array_add(features, 4, feature_prefix, "ngrams", ngram_num_chars, ngram);
        })
    } else {
        cstring_array_foreach(ngrams, idx, ngram, {
            feature_array_add(features, 3, "ngrams", ngram_num_chars, ngram);
        })
    }

    return true;
}

/*
address_parser_features
-----------------------

This is a feature function similar to those found in MEMM and CRF models.

Follows the signature of a tagger_feature_function so it can be called
as a function pointer by the averaged perceptron or CRF model.

Parameters:

address_parser_t *self: a pointer to the address_parser struct, which contains
word frequencies and perhaps other useful corpus-wide statistics.

address_parser_context_t *context: The context struct containing:
- phrase dictionary memberships for all the tokens
- country (if knkown)
- language (if known)
- features array

tokenized_string_t *tokenized: the sequence of tokens for parsing
uint32_t i: the current token index
char *prev: the predicted tag at index i - 1
char *prev2: the predicted tag at index i - 2

*/

#ifdef EXP_DIRECT_KEYS
extern int crf_features_accumulate;   // defined in crf.c
#endif
#ifdef EXP_NAME_LEN
// Experiment 019: string-literal feature names get their length at compile time; a char* name (one call site) still uses strlen.
void feature_array_add_named(cstring_array *features, size_t count, const char *name, size_t name_len, ...);
#define FA_NAME_LEN(name) (__builtin_types_compatible_p(__typeof__(name), char *) || __builtin_types_compatible_p(__typeof__(name), const char *) ? strlen((const char *)(name)) : sizeof(name) - 1)
#define feature_array_add(f, count, name, ...) feature_array_add_named((f), (count), (name), FA_NAME_LEN(name), ##__VA_ARGS__)
#endif

#ifdef EXP_VOCAB_MEMO
// Experiment 015: per-address memo of the vocabulary lookup of each token's normalized string (count and canonical id). Used only while
// crf_tagger_score_direct accumulates one address's features (tokens are visited 0..T-1). An entry is used only when the queried pointer IS
// the normalized string of that token, whose content does not change during the address, so the memo returns what a fresh lookup would.
typedef struct { uint32_t count, canon, len; uint8_t filled; } ap_vmemo_t;
static ap_vmemo_t *ap_vm = NULL;
static size_t ap_vm_cap = 0, ap_vm_n = 0;
static bool ap_vm_on = false;

static void ap_vm_reset(size_t n) {
    if (n > ap_vm_cap) {
        size_t m = ap_vm_cap ? ap_vm_cap : 64; while (m < n) m *= 2;
        ap_vmemo_t *p = realloc(ap_vm, m * sizeof *p);
        if (p == NULL) { ap_vm_on = false; return; }
        ap_vm = p; ap_vm_cap = m;
    }
    memset(ap_vm, 0, n * sizeof *ap_vm);
    ap_vm_n = n; ap_vm_on = true;
}

static inline ap_vmemo_t *ap_vm_get(address_parser_context_t *context, ssize_t j, const char *s) {
    if (!ap_vm_on || j < 0 || (size_t)j >= ap_vm_n || s != cstring_array_get_string(context->normalized, (uint32_t)j)) return NULL;
    ap_vmemo_t *m = &ap_vm[j];
    if (!m->filled) {
        size_t len = strlen(s); uint64_t slot;
        const uint8_t *e = fd_find_slot(vocab_dict, 0, s, len, &slot);
        m->len = (uint32_t)len; m->count = 0; m->canon = 0xFFFFFFFFu;
        if (e != NULL) {
            memcpy(&m->count, e + 2 + len, sizeof(uint32_t));                     // wide value, as fd_lookup reads it
            if (len > 0 && memchr(s, '|', len) == NULL) m->canon = (uint32_t)slot;  // == fd_canon(vocab_dict, s, len)
        }
        m->filled = 1;
    }
    return m;
}

static inline uint32_t ap_vocab_freq(address_parser_t *parser, address_parser_context_t *context, ssize_t j, char *s) {
    ap_vmemo_t *m = ap_vm_get(context, j, s);
    return m ? m->count : word_vocab_frequency(parser, s);
}
#define AP_VOCAB_FREQ(j, s) ap_vocab_freq(parser, context, (j), (s))
#else
#define AP_VOCAB_FREQ(j, s) word_vocab_frequency(parser, (s))
#endif

#ifdef EXP_INT_KEYS
extern int crf_ik_active;
extern int crf_ik_name[2][6];
#ifdef EXP_ROLE_IDS
extern uint32_t *crf_role_ids;
#endif
#ifdef EXP_CONST_IDS
extern uint32_t crf_const_ids[2]; extern int crf_const_ready;
#define AP_CONST(a, q) (crf_ik_active && crf_features_accumulate && crf_const_ready ? (crf_ik_push((a), 0, (1ULL << 63) | crf_const_ids[(q)]), true) : false)
#else
#define AP_CONST(a, q) false
#endif
void crf_ik_push(cstring_array *a, int which, uint64_t key);
#ifdef EXP_INT_KEYS_VERIFY
extern cstring_array *crf_ik_verify_str[2];
#endif

// canon() of one value part as feature_array_add will write it: a non-last part loses one trailing '|' (strip_separator).
static inline uint32_t ap_canon_part(address_parser_context_t *context, const char *s, ssize_t j, bool nonlast) {
#ifdef EXP_VOCAB_MEMO
    ap_vmemo_t *m = ap_vm_get(context, j, s);
    if (m != NULL) {
        if (m->canon != FDI_NONE || !nonlast || m->len == 0 || s[m->len - 1] != '|') return m->canon;
        return fd_canon(vocab_dict, s, m->len - 1);
    }
#endif
#if defined(EXP_WM_CANON) && defined(EXP_WORD_MEMO)
    // Experiment 023b: canon of a memoized phrase result (the memo owns a stable copy of its bytes for the whole address) is computed once per entry.
    if (ap_wm_on && j >= 0 && (size_t)j < ap_wm_n && ap_wm[j].filled && s == ap_wm[j].r.str) {
        ap_wm_t *e = &ap_wm[j]; int k = nonlast ? 1 : 0;
        if (!e->canon_ok[k]) { size_t len = strlen(s); if (nonlast && len > 0 && s[len - 1] == '|') len--; e->canon[k] = fd_canon(vocab_dict, s, len); e->canon_ok[k] = 1; }
        return e->canon[k];
    }
#endif
#ifdef EXP_CANON_CONST
    if (ap_canon_const_ready && s[0] == 'U') {          // the placeholders end without '|', so the non-last strip cannot change them
        if (strcmp(s, UNKNOWN_WORD) == 0) return ap_canon_unknown;
        if (strcmp(s, UNKNOWN_NUMERIC) == 0) return ap_canon_unknown_numeric;
    }
#endif
    size_t len = strlen(s);
    if (nonlast && len > 0 && s[len - 1] == '|') len--;
    return fd_canon(vocab_dict, s, len);
}

static inline bool ap_ik_emit1(address_parser_context_t *context, cstring_array *a, int which, int role, char *name, char *s, ssize_t j) {
    if (!crf_ik_active || !crf_features_accumulate || vocab_dict == NULL) return false;
    int nm = crf_ik_name[which][role];
    if (nm < 0) return false;
    uint32_t c = ap_canon_part(context, s, j, false);
    if (c == FDI_NONE) return false;
#ifdef EXP_ROLE_IDS
    if (crf_role_ids != NULL) crf_ik_push(a, which, (1ULL << 63) | crf_role_ids[(size_t)c * 6 + (which == 0 ? role : 4 + role)]);
    else
#endif
    crf_ik_push(a, which, fdi_key((uint32_t)nm, c, FDI_WNONE));
#ifdef EXP_INT_KEYS_VERIFY
    feature_array_add(crf_ik_verify_str[which], 2, name, s);
#endif
    return true;
}

static inline bool ap_ik_emit2(address_parser_context_t *context, cstring_array *a, int which, int role, char *name, char *s1, ssize_t j1, char *s2, ssize_t j2) {
    if (!crf_ik_active || !crf_features_accumulate || vocab_dict == NULL) return false;
    int nm = crf_ik_name[which][role];
    if (nm < 0) return false;
    uint32_t c1 = ap_canon_part(context, s1, j1, true);
    if (c1 == FDI_NONE) return false;
    uint32_t c2 = ap_canon_part(context, s2, j2, false);
    if (c2 == FDI_NONE) return false;
    crf_ik_push(a, which, fdi_key((uint32_t)nm, c1, c2));
#ifdef EXP_INT_KEYS_VERIFY
    feature_array_add(crf_ik_verify_str[which], 3, name, s1, s2);
#endif
    return true;
}
#define AP_IK1(a, w, r, name, s, j) ap_ik_emit1(context, (a), (w), (r), (name), (s), (j))
#define AP_IK2(a, w, r, name, s1, j1, s2, j2) ap_ik_emit2(context, (a), (w), (r), (name), (s1), (j1), (s2), (j2))
#else
#define AP_IK1(a, w, r, name, s, j) false
#define AP_IK2(a, w, r, name, s1, j1, s2, j2) false
#endif

bool address_parser_features(void *self, void *ctx, tokenized_string_t *tokenized, uint32_t idx) {
    if (self == NULL || ctx == NULL) return false;

    address_parser_t *parser = (address_parser_t *)self;
    address_parser_context_t *context = (address_parser_context_t *)ctx;

    cstring_array *features = context->features;
    cstring_array *prev_tag_features = context->prev_tag_features;
    cstring_array *prev2_tag_features = context->prev2_tag_features;
    char *language = context->language;
    char *country = context->country;

    phrase_array *address_dictionary_phrases = context->address_dictionary_phrases;
    int64_array *address_phrase_memberships = context->address_phrase_memberships;
    phrase_array *component_phrases = context->component_phrases;
    int64_array *component_phrase_memberships = context->component_phrase_memberships;
    phrase_array *postal_code_phrases = context->postal_code_phrases;
    int64_array *postal_code_phrase_memberships = context->postal_code_phrase_memberships;
    cstring_array *normalized = context->normalized;

    uint32_array *separators = context->separators;

#ifdef EXP_DIRECT_KEYS
    // crf_tagger_score accumulates every token's features in these arrays (no per-token copy) and clears them itself.
    if (!crf_features_accumulate) {
#endif
    cstring_array_clear(features);
    cstring_array_clear(prev_tag_features);
    cstring_array_clear(prev2_tag_features);
#ifdef EXP_DIRECT_KEYS
    }
#endif
#ifdef EXP_VOCAB_MEMO
    if (crf_features_accumulate && vocab_dict != NULL) {
        if (idx == 0) ap_vm_reset(tokenized->tokens->n);
    } else {
        ap_vm_on = false;
    }
#endif
#ifdef EXP_WORD_MEMO
    if (crf_features_accumulate) {
        if (idx == 0) ap_wm_reset(tokenized->tokens->n);
    } else {
        ap_wm_on = false;
    }
#endif

    token_array *tokens = tokenized->tokens;

    token_t token = tokens->a[idx];

    ssize_t last_index = (ssize_t)idx - 1;
    ssize_t next_index = (ssize_t)idx + 1;

    char *word_pre_norm = tokenized_string_get_token(tokenized, idx);

    char *word = cstring_array_get_string(normalized, idx);
    if (word == NULL) {
        log_error("got NULL word at %d\n", idx);
        return false;
    }

    size_t word_len = strlen(word);

    log_debug("word=%s\n", word);

    phrase_t phrase = NULL_PHRASE;
    phrase_t component_phrase = NULL_PHRASE;

    char *phrase_string = NULL;
    char *component_phrase_string = NULL;

    int64_t address_phrase_index = address_phrase_memberships->a[idx];
    int64_t component_phrase_index = component_phrase_memberships->a[idx];
    
    if (address_phrase_index != NULL_PHRASE_MEMBERSHIP) {
        phrase = address_dictionary_phrases->a[address_phrase_index];
    }

    if (component_phrase_index != NULL_PHRASE_MEMBERSHIP) {
        component_phrase = component_phrases->a[component_phrase_index];
    }

    char_array *phrase_tokens = context->phrase;
    char_array *component_phrase_tokens = context->component_phrase;

    uint32_t expansion_index;
    address_expansion_value_t *expansion_value;

    bool add_word_feature = true;

    size_t num_tokens = tokenized->tokens->n;

    // Address dictionary phrases
    if (phrase.len > 0 && phrase.len >= component_phrase.len) {
        log_debug("phrase\n");

        last_index = (ssize_t)phrase.start - 1;
        next_index = (ssize_t)phrase.start + phrase.len;

        if(is_valid_dictionary_phrase(phrase)) {
            uint32_t expansion_index = phrase.data;
            address_expansion_value_t *expansion_value = address_dictionary_get_expansions(expansion_index);

            if (expansion_value == NULL) {
                log_warn("expansion_value is NULL for index %u\n", expansion_index);
                return false;
            }
            uint32_t address_phrase_types = expansion_value->components;

            phrase_string = cstring_array_get_phrase(context->normalized, phrase_tokens, phrase);

            add_word_feature = false;
            log_debug("phrase_string=%s\n", phrase_string);

            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_STREET, "street", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_NAME, "name", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_CATEGORY, "category", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_UNIT, "unit", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_PO_BOX, "po_box", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_LEVEL, "level", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_ENTRANCE, "entrance", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_STAIRCASE, "staircase", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_HOUSE_NUMBER, "house_number", phrase_string);
            add_phrase_features(features, address_phrase_types, LIBPOSTAL_ADDRESS_POSTAL_CODE, "postal_code", phrase_string);
        }
    }


    address_parser_types_t types;

    // Component phrases
    if (component_phrase.len > 0 && component_phrase.len >= phrase.len) {
        component_phrase = component_phrases->a[component_phrase_index];

        component_phrase_string = cstring_array_get_phrase(context->normalized_admin, component_phrase_tokens, component_phrase);
        
        uint32_t component_phrase_index = component_phrase.data;
        if (component_phrase_index > parser->phrase_types->n) {
            log_error("Invalid component_phrase_index: %u (parser->phrase_types->n=%zu)\n", component_phrase_index, parser->phrase_types->n);
            return false;
        }

        types = parser->phrase_types->a[component_phrase_index];

        uint32_t component_phrase_types = types.components;
        uint32_t most_common = types.most_common;

        if (last_index >= (ssize_t)component_phrase.start - 1) {
            last_index = (ssize_t)component_phrase.start - 1;
        }

        if (next_index < (ssize_t)component_phrase.start + component_phrase.len) {
            next_index = (ssize_t)component_phrase.start + component_phrase.len;
        }

        if (component_phrase_string != NULL && component_phrase_types > 0) {
            feature_array_add(features, 2, "phrase", component_phrase_string);
            add_word_feature = false;
        }

        if (component_phrase_types > 0) {
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_SUBURB, "suburb", component_phrase_string);
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_CITY, "city", component_phrase_string);
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_CITY_DISTRICT, "city_district", component_phrase_string);
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_ISLAND, "island", component_phrase_string);
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_STATE_DISTRICT, "state_district", component_phrase_string);
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_STATE, "state", component_phrase_string);
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_COUNTRY_REGION, "country_region", component_phrase_string);
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_COUNTRY, "country", component_phrase_string);
            add_phrase_features(features, component_phrase_types, ADDRESS_COMPONENT_WORLD_REGION, "world_region", component_phrase_string);
        }

        if (component_phrase_types != most_common) {
            if (most_common == ADDRESS_PARSER_BOUNDARY_CITY) {
                feature_array_add(features, 2, "commonly city", component_phrase_string);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_COUNTRY) {
                feature_array_add(features, 2, "commonly country", component_phrase_string);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_SUBURB) {
                feature_array_add(features, 2, "commonly suburb", component_phrase_string);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_CITY_DISTRICT) {
                feature_array_add(features, 2, "commonly city_district", component_phrase_string);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_STATE) {
                feature_array_add(features, 2, "commonly state", component_phrase_string);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_COUNTRY_REGION) {
                feature_array_add(features, 2, "commonly country_region", component_phrase_string);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_STATE_DISTRICT) {
                feature_array_add(features, 2, "commonly state_district", component_phrase_string);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_ISLAND) {
                feature_array_add(features, 2, "commonly island", component_phrase_string);
            }
        }
    }

    bool possible_postal_code = false;
    bool postal_code_have_admin = false;
    int64_t postal_code_phrase_index = postal_code_phrase_memberships->a[idx];
    phrase_t postal_code_phrase = NULL_PHRASE;

    if (postal_code_phrase_index != NULL_PHRASE_MEMBERSHIP) {
        postal_code_phrase = postal_code_phrases->a[postal_code_phrase_index];

        uint32_t postal_code_id = postal_code_phrase.data;

        possible_postal_code = true;

        if (last_index >= (ssize_t)postal_code_phrase.start - 1) {
            last_index = (ssize_t)postal_code_phrase.start - 1;
        }

        if (next_index < (ssize_t)postal_code_phrase.start + postal_code_phrase.len) {
            next_index = (ssize_t)postal_code_phrase.start + postal_code_phrase.len;
        }

        uint32_t admin_id;
        uint64_t postal_code_context;

        khiter_t k;

        if (last_index >= 0) {
            int64_t last_component_phrase_index = component_phrase_memberships->a[last_index];
            if (last_component_phrase_index != NULL_PHRASE_MEMBERSHIP) {
                phrase_t last_component_phrase = component_phrases->a[last_component_phrase_index];
                admin_id = last_component_phrase.data;

                if (postal_code_context_exists(parser, postal_code_id, admin_id)) {
                    postal_code_have_admin = true;
                }
            }
        }

        if (!postal_code_have_admin && next_index < num_tokens) {
            int64_t next_component_phrase_index = component_phrase_memberships->a[next_index];
            if (next_component_phrase_index != NULL_PHRASE_MEMBERSHIP) {
                phrase_t next_component_phrase = component_phrases->a[next_component_phrase_index];
                admin_id = next_component_phrase.data;
                if (postal_code_context_exists(parser, postal_code_id, admin_id)) {
                    postal_code_have_admin = true;
                }
            }
        }

    }

    if (possible_postal_code) {
        if (postal_code_have_admin) {
            if (!AP_CONST(features, 1)) feature_array_add(features, 1, "postcode have context");
            feature_array_add(features, 2, "postcode have context", word);
        } else {
            feature_array_add(features, 2, "postcode no context", word);
        }
    }

    uint32_t word_freq = AP_VOCAB_FREQ(idx, word);

    bool is_word = is_word_token(token.type);

    bool is_unknown_word = false;
    bool is_unknown = false;

    bool known_prefix = false;
    bool known_suffix = false;

    size_t prefix_len = 0;
    size_t suffix_len = 0;

    char *prefix = NULL;
    char *suffix = NULL;

    if (add_word_feature) {
        // Bias unit, acts as an intercept
        if (!AP_CONST(features, 0)) feature_array_add(features, 1, "bias");

        phrase_t prefix_phrase = context->prefix_phrases->a[idx];
        phrase_t suffix_phrase = context->suffix_phrases->a[idx];

        // Prefixes like hinter, etc.
        if (prefix_phrase.len > 0) {
            expansion_index = prefix_phrase.data;
            expansion_value = address_dictionary_get_expansions(expansion_index);

            // Don't include elisions like l', d', etc. which are in the LIBPOSTAL_ADDRESS_ANY category
            if (expansion_value->components ^ LIBPOSTAL_ADDRESS_ANY) {
                known_prefix = true;
                char_array_clear(phrase_tokens);
                prefix_len = prefix_phrase.len;
                char_array_add_len(phrase_tokens, word_pre_norm, prefix_len);
                prefix = char_array_get_string(phrase_tokens);
                log_debug("got prefix: %s\n", prefix);
                feature_array_add(features, 2, "prefix", prefix);
            }
        }

        // Suffixes like straße, etc.
        if (suffix_phrase.len > 0) {
            expansion_index = suffix_phrase.data;
            expansion_value = address_dictionary_get_expansions(expansion_index);

            if (expansion_value->components & LIBPOSTAL_ADDRESS_STREET) {
                known_suffix = true;
                char_array_clear(context->suffix_phrase);
                suffix_len = suffix_phrase.len;
                size_t word_pre_norm_len = cstring_array_token_length(tokenized->strings, idx);
                size_t suffix_offset = word_pre_norm_len - suffix_len;
                char_array_add_len(context->suffix_phrase, word_pre_norm + suffix_offset, suffix_len);
                suffix = char_array_get_string(context->suffix_phrase);
                log_debug("got suffix: %s\n", suffix);
                feature_array_add(features, 2, "suffix", suffix);
            }
        }

        bool is_hyphenated = false;

        // For rare words and unknown words (so unknown words can benefit from statistics of known but super common words)
        if (word_freq <= parser->options.rare_word_threshold && is_word) {
            log_debug("rare word: %s\n", word);
            bool ngrams_added = false;
            size_t hyphenated_word_offset = 0;
            bool first_sub_token = true;
            bool last_sub_token = true;

            ssize_t next_hyphen_index;

            token_array_clear(context->sub_tokens);

            do {
                next_hyphen_index = string_next_hyphen_index(word + hyphenated_word_offset, word_len - hyphenated_word_offset);
                char *sub_word = word;
                size_t sub_word_len = word_len;

                if (next_hyphen_index >= 0) {
                    is_hyphenated = true;
                    char_array_clear(context->sub_token);
                    char_array_add_len(context->sub_token, word + hyphenated_word_offset, next_hyphen_index);
                    token_array_push(context->sub_tokens, (token_t){hyphenated_word_offset, next_hyphen_index, token.type});
                    sub_word = char_array_get_string(context->sub_token);
                    sub_word_len = context->sub_token->n;
                    last_sub_token = false;
                } else if (is_hyphenated) {
                    char_array_clear(context->sub_token);
                    char_array_add_len(context->sub_token, word + hyphenated_word_offset, word_len - hyphenated_word_offset);
                    sub_word = char_array_get_string(context->sub_token);
                    sub_word_len = context->sub_token->n;
                    last_sub_token = true;
                }

                bool add_prefix = first_sub_token && prefix_len < sub_word_len;
                bool add_suffix = last_sub_token && suffix_len < sub_word_len;

                uint32_t sub_word_freq = word_freq;
                if (is_hyphenated) {
                    sub_word_freq = word_vocab_frequency(parser, sub_word);
                    if (sub_word_freq > 0) {
                        feature_array_add(features, 2, "sub_word", sub_word);
                    }

                }

                if (sub_word_freq <= parser->options.rare_word_threshold) {
                    // prefix/suffix features from 3-6 characters
                    for (size_t ng = 3; ng <= 6; ng++) {
                        ngrams_added = add_ngram_features(features, is_hyphenated ? "sub_word" : "word", context->ngrams, sub_word, ng, add_prefix ? prefix_len : 0, add_suffix ? suffix_len : 0);
                    }
                }

                hyphenated_word_offset += next_hyphen_index + 1;
                first_sub_token = false;

                log_debug("next_hyphen_index=%zd\n", next_hyphen_index);
            } while(next_hyphen_index >= 0);

        }

        if (word_freq > 0) {
            // The individual word
            if (!AP_IK1(features, 0, 0, "word", word, idx)) feature_array_add(features, 2, "word", word);
        } else {
            log_debug("word not in vocab: %s\n", word);

            is_unknown = true;
            word = (token.type != NUMERIC && token.type != IDEOGRAPHIC_NUMBER) ? UNKNOWN_WORD : UNKNOWN_NUMERIC;

            if (is_word_token(token.type)) {
                is_unknown_word = true;
            }
        }

        if (idx == 0 && !is_unknown_word) {
            if (!AP_IK1(features, 0, 3, "first word", word, idx)) feature_array_add(features, 2, "first word", word);
            //feature_array_add(features, 3, "first word+next word", word, next_word);
        }

    } else if (component_phrase_string != NULL) {
        word = component_phrase_string;
    } else if (phrase_string != NULL) {
        word = phrase_string;
    }

    if (last_index == idx - 1) {
        // Previous tag and current word
        if (!AP_IK1(prev_tag_features, 1, 0, "word", word, idx)) feature_array_add(prev_tag_features, 2, "word", word);

        // Previous two tags and current word
        if (parser->model_type == ADDRESS_PARSER_TYPE_GREEDY_AVERAGED_PERCEPTRON) {
            // In the CRF this is accounted for by the transition weights
            // so only need it for the averaged perceptron
            feature_array_add(prev_tag_features, 1, "trans");

            // Averaged perceptron uses two tags of history, CRF uses one
            feature_array_add(prev2_tag_features, 2, "word", word);
            feature_array_add(prev2_tag_features, 1, "trans");
        }
    }

    if (last_index >= 0) {
        address_parser_phrase_t prev_word_or_phrase = AP_WORD_OR_PHRASE(last_index);
        char *prev_word = prev_word_or_phrase.str;

        if (is_plain_word_phrase_type(prev_word_or_phrase.type)) {
            uint32_t prev_word_freq = AP_VOCAB_FREQ(last_index, prev_word);
            token_t prev_token = tokenized->tokens->a[last_index];
            bool prev_token_numeric = is_numeric_token(prev_token.type);
            if (prev_word_freq == 0) {
                prev_word = !prev_token_numeric ? UNKNOWN_WORD : UNKNOWN_NUMERIC;
            }
        }

        // Previous word
        if (!AP_IK1(features, 0, 1, "prev word", prev_word, last_index)) feature_array_add(features, 2, "prev word", prev_word);


        if (last_index == idx - 1) {
            if (!AP_IK1(prev_tag_features, 1, 1, "prev word", prev_word, last_index)) feature_array_add(prev_tag_features, 2, "prev word", prev_word);
        }

        // Previous word and current word
        if (!AP_IK2(features, 0, 4, "prev word+word", prev_word, last_index, word, idx)) feature_array_add(features, 3, "prev word+word", prev_word, word);
    }

    if (next_index < num_tokens) {
        address_parser_phrase_t next_word_or_phrase = AP_WORD_OR_PHRASE(next_index);
        char *next_word = next_word_or_phrase.str;
        size_t next_word_len = 1;

        if (is_plain_word_phrase_type(next_word_or_phrase.type)) {
            uint32_t next_word_freq = AP_VOCAB_FREQ(next_index, next_word);
            token_t next_token = tokenized->tokens->a[next_index];
            bool next_token_numeric = is_numeric_token(next_token.type);
            if (next_word_freq == 0) {
                next_word = !next_token_numeric ? UNKNOWN_WORD : UNKNOWN_NUMERIC;
            }
        } else {
            next_word_len = next_word_or_phrase.phrase.len;
        }

        // Next word e.g. if the current word is unknown and the next word is "street"
        if (!AP_IK1(features, 0, 2, "next word", next_word, next_index)) feature_array_add(features, 2, "next word", next_word);

        // Current word and next word
        if (!AP_IK2(features, 0, 5, "word+next word", word, idx, next_word, next_index)) feature_array_add(features, 3, "word+next word", word, next_word);

        // Prev tag, current word and next word
        //feature_array_add(features, 4, "prev tag+word+next word", prev || "START", word, next_word);

        // Venue names ("house") are almost always at the beginning of the string
        // and often contain out-of-vocabulary words. Consider a case like "Barboncino 781 Franklin Ave".
        // The features available to classify "Barboncino" are going to be unknown word featuers (n-grams),
        // next word features (unknown word where next word=DDD is just as likely to be a street)
        // and no previous tags of history since it's the first word. If the parser predicts the
        // first token correctly, it's going to have an easier time getting the rest of the sequence
        // correct (unknown word + prev tag was "house" is probably still part of the venue, etc.) so
        // we're only really worried about that first token.  This group of features, called
        // "long-context features" finds the relative position of the next numeric token as well
        // as the next street-level phrase (words like "ave", "street", etc.) in the right context.
        // In an English or French address, if we know there's a number somewhere to our right,
        // and that a word like "Ave" appears to the right of the number, it's very likely that
        // the current unknown word is part of a venue name. Similarly, if a venue-word like "Pizzeria"
        // occurred prior to the number, that would also be strong evidence that we're in a venue name.
        // Conversely, if we're in a Spanish address and a word like "Calle" comes before the first number
        // to our right, it's also likely that we're in a venue name, but we'd need to note that the
        // phrase we saw was "Calle" and not an English thoroughfare type.

        if (idx == 0 && add_word_feature && is_unknown_word) {
            bool seen_number = false;
            bool seen_phrase = false;
            for (uint32_t right_idx = idx + 1; right_idx < num_tokens; right_idx++) {
                token_t right_token = tokens->a[right_idx];

                /* Check */
                address_parser_phrase_t right_context_word_or_phrase = word_or_phrase_at_index(parser, tokenized, context, right_idx, true);
                address_parser_phrase_type_t right_context_phrase_type = right_context_word_or_phrase.type;
                if (right_context_phrase_type != ADDRESS_PARSER_NULL_PHRASE &&
                    right_context_phrase_type != ADDRESS_PARSER_DICTIONARY_PHRASE &&
                    right_context_phrase_type != ADDRESS_PARSER_SUFFIX_PHRASE &&
                    right_context_phrase_type != ADDRESS_PARSER_PREFIX_PHRASE) {
                    continue;
                }
                char *right_context_word = right_context_word_or_phrase.str;
                phrase_t right_context_phrase = right_context_word_or_phrase.phrase;

                phrase_t suffix_phrase = context->suffix_phrases->a[right_idx];

                uint32_t right_context_expansion_index;
                address_expansion_value_t *right_context_expansion_value;

                uint32_t right_context_components = 0;
                bool right_context_name = false;
                bool right_context_street = false;

                if (right_context_phrase.len > 0) {
                    right_context_expansion_index = right_context_phrase.data;
                    right_context_expansion_value = address_dictionary_get_expansions(right_context_expansion_index);
                    right_context_components = right_context_expansion_value->components;

                    char *right_affix_type = NULL;
                    char *right_context_affix = NULL;

                    char *relation_to_number = seen_number ? "after number" : "before number";

                    seen_phrase = true;

                    char *right_context_word_pre_norm;

                     if (right_context_phrase_type == ADDRESS_PARSER_SUFFIX_PHRASE) {
                        right_affix_type = "suffix";
                        right_context_word_pre_norm = tokenized_string_get_token(tokenized, right_idx);
                        right_context_affix = phrase_suffix(right_context_word, strlen(right_context_word_pre_norm), right_context_phrase, context->long_context_suffix_phrase);
                    } else if (right_context_word_or_phrase.type == ADDRESS_PARSER_PREFIX_PHRASE) {
                        right_affix_type = "prefix";
                        right_context_word_pre_norm = tokenized_string_get_token(tokenized, right_idx);
                        right_context_affix = phrase_prefix(right_context_word, strlen(right_context_word_pre_norm), right_context_phrase, context->long_context_suffix_phrase);
                    }

                    if (right_context_components & LIBPOSTAL_ADDRESS_STREET && !(right_context_components & LIBPOSTAL_ADDRESS_NAME)) {
                        feature_array_add(features, 2, "first word unknown+street phrase right", relation_to_number);
                        feature_array_add(features, 3, "first word unknown+street phrase right", relation_to_number, right_context_word);
                        if (right_context_affix != NULL && right_affix_type != NULL) {
                            feature_array_add(features, 4, "first word unknown+street affix right", relation_to_number, right_affix_type, right_context_affix);
                        }
                        break;
                    } else if (right_context_components & LIBPOSTAL_ADDRESS_NAME && !(right_context_components & LIBPOSTAL_ADDRESS_STREET)) {
                        feature_array_add(features, 2, "first word unknown+venue phrase right", relation_to_number);
                        feature_array_add(features, 3, "first word unknown+venue phrase right", relation_to_number, right_context_word);
                        if (right_context_affix != NULL && right_affix_type != NULL) {
                            feature_array_add(features, 4, "first word unknown+venue affix right", relation_to_number, right_affix_type, right_context_affix);
                        }
                    } else if (right_context_components & (LIBPOSTAL_ADDRESS_NAME | LIBPOSTAL_ADDRESS_STREET)) {
                        if (seen_number) {
                            feature_array_add(features, 1, "first word unknown+number+ambiguous phrase right");
                            feature_array_add(features, 2, "first word unknown+number+ambiguous phrase right", right_context_word);
                            if (right_context_affix != NULL && right_affix_type != NULL) {
                                feature_array_add(features, 3, "first word unknown+number+ambiguous affix right", right_affix_type, right_context_affix);
                            }
                            break;
                        } else {
                            continue;
                        }
                    }

                    if (seen_number) break;
                }

                if (is_numeric_token(right_token.type)) {
                    seen_number = true;
                    char *relation_to_phrase = seen_phrase ? "after phrase" : "before phrase";
                    feature_array_add(features, 2, "first word unknown+number right", relation_to_phrase);
                    feature_array_add(features, 3, "first word unknown+number right", relation_to_phrase, right_context_word);
                    if (seen_phrase) break;
                }
            }
        }
    }

    return true;

}

bool address_parser_predict(address_parser_t *self, address_parser_context_t *context, cstring_array *token_labels, tagger_feature_function feature_function, tokenized_string_t *tokenized_str) {
    if (self->model_type == ADDRESS_PARSER_TYPE_GREEDY_AVERAGED_PERCEPTRON) {
        return averaged_perceptron_tagger_predict(self->model.ap, self, context, context->features, context->prev_tag_features, context->prev2_tag_features, token_labels, feature_function, tokenized_str, self->options.print_features);
    } else if (self->model_type == ADDRESS_PARSER_TYPE_CRF) {
        return crf_tagger_predict(self->model.crf, self, context, context->features, context->prev_tag_features, token_labels, feature_function, tokenized_str, self->options.print_features);
    } else {
        log_error("Parser has unknown model type\n");
    }
    return false;
}

#ifdef EXP_RESPONSE_BLOCK
// Experiment 017 step 2 (audit R1). The response's string arrays and all their bytes live in ONE allocation: [char *components[nc]][char *labels[nl]]
// [component bytes][label bytes]; response->components points at the block start and libpostal_address_parser_response_destroy frees only that.
// Same strings, same order; replaces 2 + nc + nl mallocs/strdups and frees per address.
static bool ap_response_block(libpostal_address_parser_response_t *r, cstring_array *components, cstring_array *labels) {
    size_t nc = cstring_array_num_strings(components), nl = cstring_array_num_strings(labels);
    size_t bc = components->str->n, bl = labels->str->n;
    char *block = malloc((nc + nl) * sizeof(char *) + bc + bl);
    if (block == NULL) return false;
    char **cp = (char **)block, **lp = cp + nc; char *sp = (char *)(lp + nl);
    memcpy(sp, components->str->a, bc); for (size_t i = 0; i < nc; i++) cp[i] = sp + components->indices->a[i];
    memcpy(sp + bc, labels->str->a, bl); for (size_t i = 0; i < nl; i++) lp[i] = sp + bc + labels->indices->a[i];
    r->num_components = nc; r->components = cp; r->labels = lp;
    return true;
}
#endif
libpostal_address_parser_response_t *address_parser_response_new(void) {
    libpostal_address_parser_response_t *response = malloc(sizeof(libpostal_address_parser_response_t));
    return response;
}

#ifdef EXP_STAGE_STATS
// Experiment 016 T1/T2 (measurement build only, never a candidate): per-stage ticks and per-address counts, read by tools/stage_bench.c.
#include <mach/mach_time.h>
uint64_t ss_t[16];   // 0 normalize, 1 tokenize, 2 context_fill, 3 predict (incl. 5-7), 4 response build, 5 features, 6 lookups, 7 accumulate
uint64_t ss_n[8];    // 0 addresses predicted, 1 tokens, 2 state features, 3 transition features, 4 bypassed (single phrase), 5 input bytes
#define SS_NOW() mach_absolute_time()
#endif
#ifdef EXP_REUSE
// Experiment 019: per-address objects that address_parser_parse used to create and free every call (token array, tokenized string with its two
// arrays and string copy, token labels, response label/component arrays) are kept and cleared instead. Same contents, no allocation per address.
// Requires EXP_RESPONSE_BLOCK (the response copies the label/component arrays into its own block).
#include "scanner.h"
static token_array *rs_tokens = NULL;
static tokenized_string_t *rs_ts = NULL;
static char_array *rs_str = NULL;
static cstring_array *rs_token_labels = NULL, *rs_labels = NULL, *rs_components = NULL;
static bool rs_ready(void) {
#ifdef EXP_AUDIT_FIXES
    if (!rs_tokens) rs_tokens = token_array_new();
    if (!rs_ts) rs_ts = tokenized_string_new();
    if (!rs_str) rs_str = char_array_new();
    if (!rs_token_labels) rs_token_labels = cstring_array_new();
    if (!rs_labels) rs_labels = cstring_array_new();
    if (!rs_components) rs_components = cstring_array_new();
#else
    if (rs_tokens) return true;
    rs_tokens = token_array_new(); rs_ts = tokenized_string_new(); rs_str = char_array_new();
    rs_token_labels = cstring_array_new(); rs_labels = cstring_array_new(); rs_components = cstring_array_new();
#endif
    return rs_tokens && rs_ts && rs_str && rs_token_labels && rs_labels && rs_components;
}
#endif
libpostal_address_parser_response_t *address_parser_parse(char *address, char *language, char *country) {
    if (address == NULL) return NULL;

    address_parser_t *parser = get_address_parser();
    if (parser == NULL || parser->context == NULL) {
        log_error("parser is not setup, call libpostal_setup_address_parser()\n");
        return NULL;
    }

    address_parser_context_t *context = parser->context;

#ifdef EXP_STAGE_STATS
    uint64_t ss0 = SS_NOW(); ss_n[5] += strlen(address);
#endif
    char *normalized = address_parser_normalize_string(address);
#ifdef EXP_STAGE_STATS
    uint64_t ss1 = SS_NOW(); ss_t[0] += ss1 - ss0;
#endif
    bool is_normalized = normalized != NULL;
    if (!is_normalized) {
        normalized = address;
    }

#ifdef EXP_REUSE
    if (!rs_ready()) { if (is_normalized) free(normalized); return NULL; }
    token_array *tokens = rs_tokens; token_array_clear(tokens);
    tokenize_add_tokens(tokens, normalized, strlen(normalized), false);
#else
    token_array *tokens = tokenize(normalized);
#endif

#ifdef EXP_REUSE
    tokenized_string_t *tokenized_str = rs_ts;
    cstring_array_clear(tokenized_str->strings); token_array_clear(tokenized_str->tokens);
    char_array_clear(rs_str); char_array_add_len(rs_str, normalized, strlen(normalized)); tokenized_str->str = char_array_get_string(rs_str);
#else
    tokenized_string_t *tokenized_str = tokenized_string_new_from_str_size(normalized, strlen(normalized), tokens->n);
#endif

    uint32_array_clear(context->separators);

    for (size_t i = 0; i < tokens->n; i++) {
        token_t token = tokens->a[i];
        if (ADDRESS_PARSER_IS_SEPARATOR(token.type)) {
            uint32_array_pop(context->separators);
            uint32_array_push(context->separators, ADDRESS_SEPARATOR_FIELD_INTERNAL);
            continue;
        } else if (ADDRESS_PARSER_IS_IGNORABLE(token.type)) {
            continue;
        }

        tokenized_string_add_token(tokenized_str, (const char *)normalized, token.len, token.type, token.offset);
        uint32_array_push(context->separators, ADDRESS_SEPARATOR_NONE);
    }

    // This parser was trained without knowing language/country.
    // If at some point we build country-specific/language-specific
    // parsers, these parameters could be used to select a model.
    // The language parameter does technically control which dictionaries
    // are searched at the street level. It's possible with e.g. a phrase
    // like "de", which can be either the German country code or a stopword
    // in Spanish, that even in the case where it's being used as a country code,
    // it's possible that both the street-level and admin-level phrase features
    // may be working together as a kind of intercept. Depriving the model
    // of the street-level phrase features by passing in a known language
    // may change the decision threshold so explicitly ignore these
    // options until there's a use for them (country-specific or language-specific
    // parser models).

    language = NULL;
    country = NULL;
#ifdef EXP_STAGE_STATS
    uint64_t ss2 = SS_NOW(); ss_t[1] += ss2 - ss1;
#endif
    address_parser_context_fill(context, parser, tokenized_str, language, country);
#ifdef EXP_STAGE_STATS
    uint64_t ss3 = SS_NOW(); ss_t[2] += ss3 - ss2;
#endif

    libpostal_address_parser_response_t *response = NULL;

    // If the whole input string is a single known phrase at the SUBURB level or higher, bypass sequence prediction altogether
    phrase_t only_phrase = NULL_PHRASE;
    token_t token, prev_token;
    bool is_postal = false;
    if (context->component_phrases->n == 1) {
        only_phrase = context->component_phrases->a[0];
    } else if (context->postal_code_phrases->n == 1) {
        only_phrase = context->postal_code_phrases->a[0];
        is_postal = true;
    }

    if (only_phrase.start == 0 && only_phrase.len == tokenized_str->tokens->n && only_phrase.len > 0) {
        uint32_t most_common = 0;

        char *label = NULL;

        if (!is_postal) {
            uint32_t component_phrase_index = only_phrase.data;
            address_parser_types_t types = parser->phrase_types->a[component_phrase_index];
            most_common = types.most_common;

            if (most_common == ADDRESS_PARSER_BOUNDARY_CITY) {
                label = strdup(ADDRESS_PARSER_LABEL_CITY);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_STATE) {
                label = strdup(ADDRESS_PARSER_LABEL_STATE);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_COUNTRY) {
                label = strdup(ADDRESS_PARSER_LABEL_COUNTRY);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_STATE_DISTRICT) {
                label = strdup(ADDRESS_PARSER_LABEL_STATE_DISTRICT);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_COUNTRY_REGION) {
                label = strdup(ADDRESS_PARSER_LABEL_COUNTRY_REGION);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_SUBURB) {
                label = strdup(ADDRESS_PARSER_LABEL_SUBURB);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_CITY_DISTRICT) {
                label = strdup(ADDRESS_PARSER_LABEL_CITY_DISTRICT);
            } else if (most_common == ADDRESS_PARSER_BOUNDARY_WORLD_REGION) {
                label = strdup(ADDRESS_PARSER_LABEL_WORLD_REGION);
            }
        } else {
            label = strdup(ADDRESS_PARSER_LABEL_POSTAL_CODE);
        }

        // Implicit: if most_common is not one of the above, ignore and parse regularly
        if (label != NULL) {
#ifdef EXP_RESPONSE_BLOCK
            response = address_parser_response_new();
            cstring_array *one_c = cstring_array_new(), *one_l = cstring_array_new();
            cstring_array_add_string(one_c, normalized); cstring_array_add_string(one_l, label);
            bool ok_block = ap_response_block(response, one_c, one_l);
            cstring_array_destroy(one_c); cstring_array_destroy(one_l); free(label);
            if (!ok_block) { free(response); response = NULL; }
#else
            char **single_label = malloc(sizeof(char *));
            single_label[0] = label;
            char **single_component = malloc(sizeof(char *));
            single_component[0] = strdup(normalized);

            response = address_parser_response_new();

            response->num_components = 1;
            response->labels = single_label;
            response->components = single_component;
#endif

#ifndef EXP_REUSE
            token_array_destroy(tokens);
            tokenized_string_destroy(tokenized_str);
#endif

            if (is_normalized) {
                free(normalized);
            }
#ifdef EXP_STAGE_STATS
            ss_n[4]++;
#endif
            return response;
        }
    }

#ifdef EXP_REUSE
    cstring_array *token_labels = rs_token_labels; cstring_array_clear(token_labels);
#else
    cstring_array *token_labels = cstring_array_new_size(tokens->n);
#endif

    char *prev_label = NULL;

#ifdef EXP_STAGE_STATS
    uint64_t ss4 = SS_NOW();
#endif
    bool prediction_success = address_parser_predict(parser, context, token_labels, &address_parser_features, tokenized_str);
#ifdef EXP_STAGE_STATS
    uint64_t ss5 = SS_NOW(); ss_t[3] += ss5 - ss4; ss_n[0]++; ss_n[1] += tokenized_str->tokens->n;
#endif

    if (prediction_success) {
        response = address_parser_response_new();

        size_t num_strings = cstring_array_num_strings(tokenized_str->strings);

#ifdef EXP_REUSE
        cstring_array *labels = rs_labels, *components = rs_components; cstring_array_clear(labels); cstring_array_clear(components);
#else
        cstring_array *labels = cstring_array_new_size(num_strings);
        cstring_array *components = cstring_array_new_size(strlen(address) + num_strings);
#endif

        token_t *tokens = tokenized_str->tokens->a;

        for (size_t i = 0; i < num_strings; i++) {
            char *str = tokenized_string_get_token(tokenized_str, i);

            char *label = cstring_array_get_string(token_labels, i);

            if (prev_label == NULL || strcmp(label, prev_label) != 0) {
                cstring_array_add_string(labels, label);
                cstring_array_start_token(components);

            }

            if (prev_label != NULL && strcmp(label, prev_label) == 0) {
                token = tokens[i];
                prev_token = tokens[i - 1];
                if (token.offset > prev_token.offset + prev_token.len) {
                    cstring_array_cat_string(components, " ");
                }
                cstring_array_cat_string(components, str);
            } else {
                cstring_array_append_string(components, str);
                cstring_array_terminate(components);
            }

            prev_label = label;
        }
#ifdef EXP_RESPONSE_BLOCK
        bool ok_block = ap_response_block(response, components, labels);
#ifndef EXP_REUSE
        cstring_array_destroy(components); cstring_array_destroy(labels);
#endif
        if (!ok_block) { free(response); response = NULL; }
#else
        response->num_components = cstring_array_num_strings(components);
        response->components = cstring_array_to_strings(components);
        response->labels = cstring_array_to_strings(labels);
#endif

    } else {
        log_error("Error in prediction\n");
    }

#ifndef EXP_REUSE
    token_array_destroy(tokens);
    tokenized_string_destroy(tokenized_str);
    cstring_array_destroy(token_labels);
#endif

    if (is_normalized) {
        free(normalized);
    }

#ifdef EXP_STAGE_STATS
    ss_t[4] += SS_NOW() - ss5;
#endif
    return response;
}



bool address_parser_module_setup(char *dir) {
    if (parser == NULL) {
        return address_parser_load(dir);
    }
    return true;
}

void address_parser_module_teardown(void) {
    if (parser != NULL) {
        address_parser_destroy(parser);
    }
    parser = NULL;
#if defined(EXP_REUSE) && defined(EXP_AUDIT_FIXES)
    token_array_destroy(rs_tokens); rs_tokens = NULL;
    if (rs_ts) rs_ts->str = NULL; // rs_str owns the borrowed character buffer.
    tokenized_string_destroy(rs_ts); rs_ts = NULL;
    char_array_destroy(rs_str); rs_str = NULL;
    cstring_array_destroy(rs_token_labels); rs_token_labels = NULL;
    cstring_array_destroy(rs_labels); rs_labels = NULL;
    cstring_array_destroy(rs_components); rs_components = NULL;
#endif
}
