// Public-API parity harness (experiment 204). Correctness only, never timed. One thread.
// usage: api_suite MODE addresses.txt     (one address per line; blank lines skipped)
// MODE: parse_options | expand | classify | dedupe | toponym | normalize | print_features | lifecycle
// Every result is printed deterministically (inputs echoed, doubles as %.17g) so the original and
// optimized builds can be compared byte-for-byte on stdout, stderr and exit status. Each mode is a
// separate process so an abort in one API family cannot hide results from the others.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"

static void esc(const char *s) {
    putchar('"');
    if (s) for (; *s; s++) { unsigned char c = *s; if (c == '"' || c == '\\') { putchar('\\'); putchar(c); } else if (c < 0x20) printf("\\u%04x", c); else putchar(c); }
    putchar('"');
}
static void strs(const char *tag, char **v, size_t n) {
    printf(" %s=[", tag);
    for (size_t i = 0; i < n; i++) { if (i) putchar(','); esc(v[i]); }
    putchar(']');
}

static char **lines; static size_t nlines;
static void load(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) exit(2);
    char *line = NULL; size_t cap = 0, lcap = 0; ssize_t len;
    while ((len = getline(&line, &cap, f)) >= 0) {
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        if (!len) continue;
        if (nlines == lcap) { lcap = lcap ? lcap * 2 : 1024; lines = realloc(lines, lcap * sizeof *lines); }
        lines[nlines++] = strdup(line);
    }
    free(line); fclose(f);
}

static void parse_print(char *a, libpostal_address_parser_options_t o) {
    libpostal_address_parser_response_t *r = libpostal_parse_address(a, o);
    if (!r) { printf(" null"); return; }
    printf(" [");
    for (size_t i = 0; i < r->num_components; i++) { if (i) putchar(','); putchar('['); esc(r->labels[i]); putchar(','); esc(r->components[i]); putchar(']'); }
    printf("]");
    libpostal_address_parser_response_destroy(r);
}

static void mode_parse_options(void) {
    char *opts[][2] = {{NULL, NULL}, {"en", "us"}, {"de", "de"}, {"fr", "fr"}, {NULL, "gb"}, {"es", NULL}};
    for (size_t i = 0; i < nlines; i++)
        for (size_t k = 0; k < sizeof opts / sizeof opts[0]; k++) {
            libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
            o.language = opts[k][0]; o.country = opts[k][1];
            printf("parse lang="); esc(o.language); printf(" country="); esc(o.country); printf(" in="); esc(lines[i]);
            parse_print(lines[i], o); putchar('\n');
        }
}

static void mode_expand(void) {
    libpostal_normalize_options_t o = libpostal_get_default_options();
    for (size_t i = 0; i < nlines; i++) {
        size_t n; char **e = libpostal_expand_address(lines[i], o, &n);
        printf("expand in="); esc(lines[i]); strs("out", e, e ? n : 0); putchar('\n');
        if (e) libpostal_expansion_array_destroy(e, n);
        e = libpostal_expand_address_root(lines[i], o, &n);
        printf("expand_root in="); esc(lines[i]); strs("out", e, e ? n : 0); putchar('\n');
        if (e) libpostal_expansion_array_destroy(e, n);
    }
}

static void mode_classify(void) {
    for (size_t i = 0; i < nlines; i++) {
        libpostal_language_classifier_response_t *r = libpostal_classify_language(lines[i]);
        printf("classify in="); esc(lines[i]);
        if (r) { for (size_t k = 0; k < r->num_languages; k++) printf(" %s:%.17g", r->languages[k], r->probs[k]); libpostal_language_classifier_response_destroy(r); }
        else printf(" null");
        putchar('\n');
    }
}

static const char *first_of(libpostal_address_parser_response_t *r, const char *label) {
    for (size_t i = 0; r && i < r->num_components; i++) if (!strcmp(r->labels[i], label)) return r->components[i];
    return NULL;
}
static size_t words(char *s, char ***out, double **scores) {   // whitespace tokens of a copy of s, score 1.0 each
    char *c = strdup(s); size_t n = 0, cap = 8; *out = malloc(cap * sizeof **out);
    for (char *t = strtok(c, " \t"); t; t = strtok(NULL, " \t")) { if (n == cap) { cap *= 2; *out = realloc(*out, cap * sizeof **out); } (*out)[n++] = strdup(t); }
    *scores = malloc((n ? n : 1) * sizeof **scores); for (size_t i = 0; i < n; i++) (*scores)[i] = 1.0;
    free(c); return n;
}
static void status(const char *tag, libpostal_duplicate_status_t s) { printf(" %s=%d", tag, (int)s); }
static void pair_status(const char *tag, libpostal_duplicate_status_t (*fn)(char *, char *, libpostal_duplicate_options_t),
                        const char *a, const char *b, libpostal_duplicate_options_t o) {
    if (a && b) status(tag, fn((char *)a, (char *)b, o)); else printf(" %s=absent", tag);
}

static void mode_dedupe(void) {
    libpostal_address_parser_options_t po = libpostal_get_address_parser_default_options();
    libpostal_near_dupe_hash_options_t ho = libpostal_get_near_dupe_hash_default_options();
    libpostal_normalize_options_t no = libpostal_get_default_options();
    libpostal_duplicate_options_t dopt = libpostal_get_default_duplicate_options();
    libpostal_fuzzy_duplicate_options_t fo = libpostal_get_default_fuzzy_duplicate_options();
    libpostal_address_parser_response_t *prev = NULL; char *prev_line = NULL;
    for (size_t i = 0; i < nlines; i++) {
        libpostal_address_parser_response_t *r = libpostal_parse_address(lines[i], po);
        printf("dedupe in="); esc(lines[i]);
        if (r && r->num_components) {
            size_t n; char **h = libpostal_near_dupe_hashes(r->num_components, r->labels, r->components, ho, &n);
            strs("near_dupe", h, h ? n : 0); if (h) libpostal_expansion_array_destroy(h, n);
            char **l = libpostal_place_languages(r->num_components, r->labels, r->components, &n);
            strs("place_languages", l, l ? n : 0);
            size_t nh; h = libpostal_near_dupe_hashes_languages(r->num_components, r->labels, r->components, ho, l ? n : 0, l, &nh);
            strs("near_dupe_languages", h, h ? nh : 0); if (h) libpostal_expansion_array_destroy(h, nh);
            free(l);   // upstream hands over the classifier's array; its strings are not owned (see language_classifier_response_destroy)
            h = libpostal_near_dupe_name_hashes(r->components[0], no, &n);
            strs("name_hashes", h, h ? n : 0); if (h) libpostal_expansion_array_destroy(h, n);
        }
        if (prev) {
            pair_status("name", libpostal_is_name_duplicate, lines[i], prev_line, dopt);
            pair_status("street", libpostal_is_street_duplicate, first_of(r, "road"), first_of(prev, "road"), dopt);
            pair_status("house_number", libpostal_is_house_number_duplicate, first_of(r, "house_number"), first_of(prev, "house_number"), dopt);
            pair_status("po_box", libpostal_is_po_box_duplicate, first_of(r, "po_box"), first_of(prev, "po_box"), dopt);
            pair_status("unit", libpostal_is_unit_duplicate, first_of(r, "unit"), first_of(prev, "unit"), dopt);
            pair_status("floor", libpostal_is_floor_duplicate, first_of(r, "level"), first_of(prev, "level"), dopt);
            pair_status("postal_code", libpostal_is_postal_code_duplicate, first_of(r, "postcode"), first_of(prev, "postcode"), dopt);
            pair_status("self_name", libpostal_is_name_duplicate, lines[i], lines[i], dopt);
            char **t1, **t2; double *s1, *s2; size_t n1 = words(lines[i], &t1, &s1), n2 = words(prev_line, &t2, &s2);
            libpostal_fuzzy_duplicate_status_t fs = libpostal_is_name_duplicate_fuzzy(n1, t1, s1, n2, t2, s2, fo);
            printf(" name_fuzzy=%d:%.17g", (int)fs.status, fs.similarity);
            fs = libpostal_is_street_duplicate_fuzzy(n1, t1, s1, n2, t2, s2, fo);
            printf(" street_fuzzy=%d:%.17g", (int)fs.status, fs.similarity);
            for (size_t k = 0; k < n1; k++) free(t1[k]);
            for (size_t k = 0; k < n2; k++) free(t2[k]);
            free(t1); free(t2); free(s1); free(s2);
        }
        putchar('\n');
        if (prev) libpostal_address_parser_response_destroy(prev);
        prev = r; prev_line = lines[i];
    }
    if (prev) libpostal_address_parser_response_destroy(prev);
}

static void mode_toponym(void) {   // separate mode: upstream is_toponym_duplicate crashes on some valid inputs (experiment 204)
    libpostal_address_parser_options_t po = libpostal_get_address_parser_default_options();
    libpostal_duplicate_options_t dopt = libpostal_get_default_duplicate_options();
    libpostal_address_parser_response_t *prev = NULL;
    for (size_t i = 0; i < nlines; i++) {
        libpostal_address_parser_response_t *r = libpostal_parse_address(lines[i], po);
        if (prev && r) { printf("toponym in="); esc(lines[i]); fflush(stdout);
            status("status", libpostal_is_toponym_duplicate(r->num_components, r->labels, r->components, prev->num_components, prev->labels, prev->components, dopt)); putchar('\n'); }
        if (prev) libpostal_address_parser_response_destroy(prev);
        prev = r;
    }
    if (prev) libpostal_address_parser_response_destroy(prev);
}

static void tokens_print(const char *tag, char *in, libpostal_normalized_token_t *nt, size_t n) {
    printf("%s in=", tag); esc(in); printf(" [");
    for (size_t k = 0; nt && k < n; k++) { if (k) putchar(','); esc(nt[k].str); printf(":%zu:%zu:%u", nt[k].token.offset, nt[k].token.len, (unsigned)nt[k].token.type); free(nt[k].str); }
    printf("]\n"); free(nt);
}

static void mode_normalize(void) {
    char *en[] = {"en"};
    for (size_t i = 0; i < nlines; i++) {
        size_t n; libpostal_token_t *t = libpostal_tokenize(lines[i], true, &n);
        printf("tokenize in="); esc(lines[i]); printf(" [");
        for (size_t k = 0; t && k < n; k++) printf("%s%zu:%zu:%u", k ? "," : "", t[k].offset, t[k].len, (unsigned)t[k].type);
        printf("]\n"); free(t);
        char *s = libpostal_normalize_string(lines[i], LIBPOSTAL_NORMALIZE_DEFAULT_STRING_OPTIONS);
        printf("normalize in="); esc(lines[i]); printf(" out="); esc(s); putchar('\n'); free(s);
        s = libpostal_normalize_string_languages(lines[i], LIBPOSTAL_NORMALIZE_DEFAULT_STRING_OPTIONS, 1, en);
        printf("normalize_en in="); esc(lines[i]); printf(" out="); esc(s); putchar('\n'); free(s);
        libpostal_normalized_token_t *nt = libpostal_normalized_tokens(lines[i], LIBPOSTAL_NORMALIZE_DEFAULT_STRING_OPTIONS, LIBPOSTAL_NORMALIZE_DEFAULT_TOKEN_OPTIONS, true, &n);
        tokens_print("normalized_tokens", lines[i], nt, n);
        nt = libpostal_normalized_tokens_languages(lines[i], LIBPOSTAL_NORMALIZE_DEFAULT_STRING_OPTIONS, LIBPOSTAL_NORMALIZE_DEFAULT_TOKEN_OPTIONS_NUMERIC, false, 1, en, &n);
        tokens_print("normalized_tokens_en", lines[i], nt, n);
    }
}

static void mode_print_features(void) {   // upstream prints parser features while parsing; 119 CPI aborted here
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    printf("print_features_on=%d\n", (int)libpostal_parser_print_features(true)); fflush(stdout);
    for (size_t i = 0; i < nlines && i < 20; i++) { printf("parse in="); esc(lines[i]); fflush(stdout); parse_print(lines[i], o); putchar('\n'); fflush(stdout); }
    printf("print_features_off=%d\n", (int)libpostal_parser_print_features(false));
    for (size_t i = 0; i < nlines && i < 20; i++) { printf("parse in="); esc(lines[i]); parse_print(lines[i], o); putchar('\n'); }
}

static void mode_lifecycle(void) {   // results must not depend on prior setup/teardown history
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    for (int cycle = 1; cycle <= 3; cycle++) {
        if (cycle > 1 && !(libpostal_setup() && libpostal_setup_parser())) { printf("cycle %d setup failed\n", cycle); exit(4); }
        for (size_t i = 0; i < nlines && i < 200; i++) { printf("cycle=%d in=", cycle); esc(lines[i]); parse_print(lines[i], o); putchar('\n'); }
        libpostal_teardown_parser(); libpostal_teardown();
    }
}

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "usage: api_suite MODE addresses.txt\n"); return 2; }
    setvbuf(stdout, NULL, _IOLBF, 0);   // line-buffered: a crash leaves every completed line in the output
    const char *m = argv[1]; load(argv[2]);
    int lc = !strcmp(m, "classify") || !strcmp(m, "dedupe") || !strcmp(m, "toponym") || !strcmp(m, "expand") || !strcmp(m, "normalize");
    if (!libpostal_setup() || !libpostal_setup_parser() || (lc && !libpostal_setup_language_classifier())) return 3;
    if (!strcmp(m, "parse_options")) mode_parse_options();
    else if (!strcmp(m, "expand")) mode_expand();
    else if (!strcmp(m, "classify")) mode_classify();
    else if (!strcmp(m, "dedupe")) mode_dedupe();
    else if (!strcmp(m, "toponym")) mode_toponym();
    else if (!strcmp(m, "normalize")) mode_normalize();
    else if (!strcmp(m, "print_features")) mode_print_features();
    else if (!strcmp(m, "lifecycle")) { mode_lifecycle(); return 0; }
    else { fprintf(stderr, "unknown mode %s\n", m); return 2; }
    fflush(stdout);
    if (lc) libpostal_teardown_language_classifier();
    libpostal_teardown_parser(); libpostal_teardown();
    return 0;
}
