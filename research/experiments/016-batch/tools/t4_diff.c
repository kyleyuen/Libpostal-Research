// 016 T4 differential test: transliterate("latin-ascii-simple", s) with the skip enabled vs disabled (same binary, same tables).
// usage: t4_diff seed n_random corpus1.txt [...]   Prints counts and exits 1 on the first mismatch (printing the input in hex).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libpostal.h"
#include "transliterate.h"
extern int trans_skip_test_disable;
static uint64_t rs; static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static unsigned long long n_same = 0, n_null = 0, n_changed = 0;
static int check(const char *s, size_t len) {
    trans_skip_test_disable = 0; char *a = transliterate(LATIN_ASCII_SIMPLE, (char *)s, len);
    trans_skip_test_disable = 1; char *b = transliterate(LATIN_ASCII_SIMPLE, (char *)s, len);
    trans_skip_test_disable = 0;
    int ok = (a == NULL && b == NULL) || (a && b && strcmp(a, b) == 0);
    if (!ok) { printf("MISMATCH input:"); for (size_t i = 0; i < len; i++) printf(" %02x", (uint8_t)s[i]); printf("\n  skip=%s\n  orig=%s\n", a ? a : "(null)", b ? b : "(null)"); return 0; }
    if (!a) n_null++; else if (strlen(a) == len && memcmp(a, s, len) == 0) n_same++; else n_changed++;
    free(a); free(b); return 1;
}
static size_t put_cp(char *d, uint32_t cp) {
    if (cp < 0x80) { d[0] = (char)cp; return 1; }
    if (cp < 0x800) { d[0] = (char)(0xC0 | cp >> 6); d[1] = (char)(0x80 | (cp & 63)); return 2; }
    if (cp < 0x10000) { d[0] = (char)(0xE0 | cp >> 12); d[1] = (char)(0x80 | (cp >> 6 & 63)); d[2] = (char)(0x80 | (cp & 63)); return 3; }
    d[0] = (char)(0xF0 | cp >> 18); d[1] = (char)(0x80 | (cp >> 12 & 63)); d[2] = (char)(0x80 | (cp >> 6 & 63)); d[3] = (char)(0x80 | (cp & 63)); return 4;
}
int main(int argc, char **argv) {
    if (argc < 3 || !libpostal_setup()) return 2;
    rs = strtoull(argv[1], 0, 10) * 0x9E3779B97F4A7C15ULL + 1; long nr = atol(argv[2]);
    unsigned long long lines = 0;
    for (int a = 3; a < argc; a++) {
        FILE *f = fopen(argv[a], "r"); if (!f) return 2; char *l = NULL; size_t c = 0; ssize_t n;
        while ((n = getline(&l, &c, f)) >= 0) { if (n && l[n - 1] == '\n') l[--n] = 0; char *t = strchr(l, '\t'); char *s = t ? t + 1 : l; if (!check(s, strlen(s))) return 1; lines++; }
        fclose(f); free(l);
    }
    // cps that begin step-1 rules (from the enumeration) plus neighbours, '&', Latin/Cyrillic/CJK, controls, and raw invalid bytes
    static const uint32_t special[] = {0xA0, 0xAD, 0xD7, 0xF7, 0x2C2, 0x2C3, 0x2C4, 0x2D6, 0x2D7, 0x2DC, 0x2002, 0x2003, 0x2010, 0x2011, 0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D,
        0x2032, 0x2039, 0x2044, 0x2212, 0x3000, 0x3001, 0x3002, 0xFE50, 0xFF01, 0xFF0C, 0xFF1A, 0xFE57, 0xA1, 0xE9, 0xDF, 0x2020, 0x3003, 0xFF00, 0xFFFD, 0x1F642, 0x0410, 0x4E2D};
    char buf[512];
    for (long i = 0; i < nr; i++) {
        size_t n = 0; int k = 1 + (int)(rnd() % 24);
        for (int j = 0; j < k && n < 480; j++) {
            unsigned r = (unsigned)(rnd() % 100);
            if (r < 35) buf[n++] = (char)('a' + rnd() % 26);
            else if (r < 45) buf[n++] = " ,.-/0123456789"[rnd() % 15];
            else if (r < 50) { const char *e[] = {"&", "&amp;", "&eacute;", "&#39;", "&nbsp;", "&x"}; const char *x = e[rnd() % 6]; memcpy(buf + n, x, strlen(x)); n += strlen(x); }
            else if (r < 75) n += put_cp(buf + n, special[rnd() % (sizeof special / sizeof *special)]);
            else if (r < 85) n += put_cp(buf + n, 0x80 + (uint32_t)(rnd() % 0x780));
            else if (r < 93) { uint32_t cp; do cp = 0x800 + (uint32_t)(rnd() % 0xF800); while (cp >= 0xD800 && cp < 0xE000); n += put_cp(buf + n, cp); }
            else if (r < 97) buf[n++] = (char)(1 + rnd() % 31);
            else n += put_cp(buf + n, 0x10000 + (uint32_t)(rnd() % 0x100000));    // supplementary planes (valid UTF-8 only: the original library crashes on invalid input)
        }
        buf[n] = 0; if (n == 0 || strlen(buf) != n) continue;
        if (!check(buf, n)) return 1;
    }
    printf("OK corpus_lines=%llu random=%ld unchanged=%llu changed=%llu null=%llu\n", lines, nr, n_same, n_changed, n_null);
    return 0;
}
