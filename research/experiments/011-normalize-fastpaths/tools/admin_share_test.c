// Experiment 011: for ASCII tokens with no digit, normalize_token with the standard options and with the admin options (no digit replacement) must give identical bytes.
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
static void check(char *s, uint16_t type) {
    token_t t; t.offset = 0; t.len = strlen(s); t.type = type;
    cstring_array *a = cstring_array_new(), *b = cstring_array_new();
    address_parser_normalize_token(a, s, t); normalize_token(b, s, t, ADDRESS_PARSER_NORMALIZE_ADMIN_TOKEN_OPTIONS);
    char *x = cstring_array_get_string(a, 0), *y = cstring_array_get_string(b, 0);
    checked++; if (strcmp(x, y) != 0) { bad++; if (shown++ < 10) printf("MISMATCH type=%u input=[%s] std=[%s] admin=[%s]\n", type, s, x, y); }
    cstring_array_destroy(a); cstring_array_destroy(b);
}
int main(int argc, char **argv) {
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    uint64_t n = strtoull(argv[1], 0, 10); char b[8], s[128];
    for (int ty = 0; ty < 130; ty++) for (int a = 1; a < 128; a++) { if (a >= '0' && a <= '9') continue; b[0] = a; b[1] = 0; check(b, ty); }
    for (int a = 1; a < 128; a++) { if (a >= '0' && a <= '9') continue; for (int c = 1; c < 128; c++) { if (c >= '0' && c <= '9') continue; b[0] = a; b[1] = c; b[2] = 0; for (int ty = 0; ty < 130; ty += 7) check(b, ty); } }
    printf("exhaustive 1-char x 130 types and 2-char x 19 types (no digits): checked=%llu mismatches=%llu\n", (unsigned long long)checked, (unsigned long long)bad);
    uint64_t c0 = checked, b0 = bad;
    for (uint64_t i = 0; i < n; i++) { size_t L = 1 + rnd() % 30; int mode = rnd() % 3;
        for (size_t k = 0; k < L; k++) { uint64_t r = rnd(); char ch = mode == 0 ? (char)(1 + r % 127) : mode == 1 ? (char)(32 + r % 95) : ((r % 3 == 0) ? "-.'/&,;:"[r % 8] : (char)(97 + r % 26)); if (ch >= '0' && ch <= '9') ch = 'x'; s[k] = ch; }
        s[L] = 0; check(s, rnd() % 130); }
    printf("random: checked=%llu mismatches=%llu\n", (unsigned long long)(checked - c0), (unsigned long long)(bad - b0));
    return bad != 0;
}
