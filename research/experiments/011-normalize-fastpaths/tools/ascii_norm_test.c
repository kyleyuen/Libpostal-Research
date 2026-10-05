// Experiment 011 measurement: is address_parser_normalize_string(s) == ASCII-lowercase(s) for every ASCII string? (Tests the hypothesis behind a possible ASCII fast path.)
// Exhaustive over all strings of length 1..3 over bytes 1..127, then random strings of length 4..96 (printable-heavy and full-range mixes), then a list of corpus lines.
// usage: ascii_norm_test random_count corpus.txt
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libpostal.h"
#include "address_parser.h"
#include "normalize.h"
static uint64_t rs = 88172645463325252ULL;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static uint64_t checked, bad, shown;
static void check(const char *s) {
    size_t n = strlen(s); char exp[512]; for (size_t i = 0; i <= n; i++) exp[i] = (s[i] >= 'A' && s[i] <= 'Z') ? s[i] + 32 : s[i];
    char *r = address_parser_normalize_string((char *)s);
    checked++;
    if (r == NULL || strcmp(r, exp) != 0) { bad++; if (shown++ < 15) { printf("MISMATCH input=["); for (size_t i = 0; i < n; i++) printf(s[i] >= 32 && s[i] < 127 ? "%c" : "\\x%02x", (unsigned char)s[i]); printf("] got=["); if (r) for (size_t i = 0; r[i]; i++) printf((unsigned char)r[i] >= 32 && (unsigned char)r[i] < 127 ? "%c" : "\\x%02x", (unsigned char)r[i]); printf("]\n"); } }
    free(r);
}
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    char b[8];
    for (int a = 1; a < 128; a++) { b[0] = a; b[1] = 0; check(b);
        for (int c = 1; c < 128; c++) { b[1] = c; b[2] = 0; check(b);
            for (int d = 1; d < 128; d++) { b[2] = d; b[3] = 0; check(b); } } }
    printf("exhaustive length 1-3 over bytes 1..127: checked=%llu mismatches=%llu\n", (unsigned long long)checked, (unsigned long long)bad);
    uint64_t n = strtoull(argv[1], 0, 10), c0 = checked, b0 = bad; char s[128];
    for (uint64_t i = 0; i < n; i++) {
        size_t L = 4 + rnd() % 93; int mode = rnd() % 3;
        for (size_t k = 0; k < L; k++) { uint64_t r = rnd();
            s[k] = mode == 0 ? (char)(32 + r % 95) : mode == 1 ? (char)(1 + r % 127) : ((r % 4 == 0) ? (char)(33 + r % 30) : (char)(97 + r % 26));   // printable / full-range / letters+punctuation
            if (s[k] == 0) s[k] = 'a'; }
        s[L] = 0; check(s);
    }
    printf("random length 4-96 (printable, full ASCII, letters+punct): checked=%llu mismatches=%llu\n", (unsigned long long)(checked - c0), (unsigned long long)(bad - b0));
    FILE *f = fopen(argv[2], "r"); char *line = NULL; size_t lc = 0; ssize_t len; uint64_t c1 = checked, b1 = bad, skipped = 0;
    while ((len = getline(&line, &lc, f)) >= 0) { while (len && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0; int ascii = 1; for (ssize_t i = 0; i < len; i++) if ((unsigned char)line[i] >= 128) ascii = 0;
        if (!ascii || len > 400) { skipped++; continue; } check(line); }
    printf("corpus ASCII lines: checked=%llu mismatches=%llu (non-ASCII lines skipped: %llu)\n", (unsigned long long)(checked - c1), (unsigned long long)(bad - b1), (unsigned long long)skipped);
    return bad != 0;
}
