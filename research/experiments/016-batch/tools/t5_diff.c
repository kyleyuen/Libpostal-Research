// 016 T5 differential test: norm_table_fast(s) (when it answers) vs normalize_string_latin(s, COMPOSE|LOWERCASE|SIMPLE_LATIN_ASCII) from the same library.
// usage: t5_diff seed n_pairs n_random corpus.txt ...   Exits 1 on the first mismatch.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libpostal.h"
#include "normalize.h"
#include "address_parser.h"
char *norm_table_fast(const char *str);
static uint64_t rs; static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static unsigned long long n_fast = 0, n_lib = 0;
static int check(const char *s) {
    char *f = norm_table_fast(s);
    if (f == NULL) { n_lib++; return 1; }
    char *r = normalize_string_latin((char *)s, strlen(s), ADDRESS_PARSER_NORMALIZE_STRING_OPTIONS);
    int ok = r != NULL && strcmp(f, r) == 0;
    if (!ok) { printf("MISMATCH input:"); for (const uint8_t *p = (const uint8_t *)s; *p; p++) printf(" %02x", *p); printf("\n  fast=%s\n  lib =%s\n", f, r ? r : "(null)"); return 0; }
    n_fast++; free(f); free(r); return 1;
}
static size_t enc(char *d, int32_t cp) {
    if (cp < 0x80) { d[0] = (char)cp; return 1; }
    if (cp < 0x800) { d[0] = (char)(0xC0 | cp >> 6); d[1] = (char)(0x80 | (cp & 63)); return 2; }
    d[0] = (char)(0xE0 | cp >> 12); d[1] = (char)(0x80 | (cp >> 6 & 63)); d[2] = (char)(0x80 | (cp & 63)); return 3;
}
int main(int argc, char **argv) {
    if (argc < 4 || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    rs = strtoull(argv[1], 0, 10) * 0x9E3779B97F4A7C15ULL + 7; long npairs = atol(argv[2]), nrand = atol(argv[3]);
    unsigned long long lines = 0;
    for (int a = 4; a < argc; a++) {
        FILE *f = fopen(argv[a], "r"); if (!f) return 2; char *l = NULL; size_t c = 0; ssize_t n;
        while ((n = getline(&l, &c, f)) >= 0) { if (n && l[n - 1] == '\n') l[--n] = 0; char *t = strchr(l, '\t'); if (!check(t ? t + 1 : l)) return 1; lines++; }
        fclose(f); free(l);
    }
    unsigned long long corpus_fast = n_fast;
    int32_t *W = malloc(65536 * sizeof *W); size_t nw = 0; char b[256];   // up to 30 codepoints x 3 bytes (a first version used 64 bytes and overflowed in the random-string stage)
    for (int32_t c = 0x80; c < 65536; c++) { if (c >= 0xD800 && c < 0xE000) continue; b[enc(b, c)] = 0; char *f = norm_table_fast(b); if (f) { W[nw++] = c; free(f); } }
    // every table codepoint next to every printable ASCII character except '&', both orders
    for (size_t i = 0; i < nw; i++) for (int32_t a = 0x20; a < 0x7F; a++) { if (a == '&') continue;
        size_t n = enc(b, W[i]); n += enc(b + n, a); b[n] = 0; if (!check(b)) return 1;
        n = enc(b, a); n += enc(b + n, W[i]); b[n] = 0; if (!check(b)) return 1; }
    for (long k = 0; k < npairs; k++) { size_t n = enc(b, W[rnd() % nw]); n += enc(b + n, W[rnd() % nw]); b[n] = 0; if (!check(b)) return 1; }
    for (long k = 0; k < nrand; k++) {
        int len = 3 + (int)(rnd() % 28); size_t n = 0;
        for (int j = 0; j < len; j++) { if (rnd() % 3) n += enc(b + n, W[rnd() % nw]); else { int32_t a; do a = 0x20 + (int32_t)(rnd() % 95); while (a == '&' || a == 0x7F); n += enc(b + n, a); } }
        b[n] = 0; if (!check(b)) return 1;
    }
    printf("OK corpus_lines=%llu corpus_fast=%llu table_codepoints=%zu checked_fast=%llu fell_back=%llu\n", lines, corpus_fast, nw, n_fast, n_lib);
    return 0;
}
