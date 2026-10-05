// Experiment 098: add_normalized_token_pair(std, admin) must give exactly the bytes of normalize_token with the address parser's standard options and with
// its admin options. Tokens sit inside a buffer with extra bytes after token.len (the iterate call may read past the token). usage: norm_pair_test N_RANDOM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libpostal.h"
#include "address_parser.h"
#include "normalize.h"
void add_normalized_token_pair(char_array *std, char_array *adm, char *str, token_t token);
static uint64_t rs = 88172645463325252ULL;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static uint64_t checked, bad, shown;
static cstring_array *a, *b, *c, *d;
static void check(char *buf, size_t off, size_t len, uint16_t type) {
    token_t t; t.offset = off; t.len = len; t.type = type;
    cstring_array_clear(a); cstring_array_clear(b); cstring_array_clear(c); cstring_array_clear(d);
    normalize_token(a, buf, t, ADDRESS_PARSER_NORMALIZE_TOKEN_OPTIONS); normalize_token(b, buf, t, ADDRESS_PARSER_NORMALIZE_ADMIN_TOKEN_OPTIONS);
    cstring_array_start_token(c); cstring_array_start_token(d); add_normalized_token_pair(c->str, d->str, buf, t);
    checked++;
    if (a->str->n != c->str->n || memcmp(a->str->a, c->str->a, a->str->n) || b->str->n != d->str->n || memcmp(b->str->a, d->str->a, b->str->n)) {
        bad++; if (shown++ < 20) printf("MISMATCH type=%u len=%zu input=[%.*s] std=[%s]/[%s] admin=[%s]/[%s]\n", type, len, (int)len, buf + off, a->str->a, c->str->a, b->str->a, d->str->a); }
}
// alphabet: ASCII 1..127 plus multibyte elements (numbers of several Unicode categories, letters, punctuation, malformed sequences)
static const char *mb[] = { "\xEF\xBC\x93" /*fullwidth 3*/, "\xD9\xA3" /*arabic-indic 3*/, "\xC2\xB2" /*superscript 2*/, "\xC2\xBD" /*1/2*/, "\xE2\x85\xAB" /*roman XII*/,
    "\xC3\xA9", "\xC3\x9F", "\xE4\xB8\xAD", "\xD0\x96", "\xEF\xBC\x8E" /*fullwidth stop*/, "\xE3\x80\x82" /*ideographic stop*/, "\xE2\x80\x90" /*hyphen*/, "\xE2\x80\x99",
    "\xC3" /*truncated*/, "\xE2\x85" /*truncated*/, "\x80" /*stray continuation*/, "\xF0\x9F\x98\x80" /*emoji*/, "\xE0\xA5\xA9" /*devanagari 3*/ };
#define NMB (sizeof(mb) / sizeof(mb[0]))
static size_t put(char *p, int e) { if (e < 127) { p[0] = (char)(e + 1); return 1; } const char *s = mb[e - 127]; size_t n = strlen(s); memcpy(p, s, n); return n; }
#define NE (127 + (int)NMB)
static const int small[] = { 'a' - 1, 'Z' - 1, '0' - 1, '9' - 1, '.' - 1, '-' - 1, '\'' - 1, ' ' - 1, '&' - 1, 127, 128, 129, 131, 135, 136, 140, 141, 143, 144 };
#define NS (int)(sizeof(small) / sizeof(small[0]))
int main(int argc, char **argv) {
    uint64_t n = strtoull(argv[1], 0, 10); char buf[256];
    a = cstring_array_new(); b = cstring_array_new(); c = cstring_array_new(); d = cstring_array_new();
    const char *tails[] = { "", ".", "5", "\xC2\xB2", "x" };
    for (int ti = 0; ti < 5; ti++) {
        for (int ty = 0; ty < 130; ty++) for (int e = 0; e < NE; e++) { size_t L = put(buf + 1, e); buf[0] = 'q'; strcpy(buf + 1 + L, tails[ti]); check(buf, 1, L, ty); }
        for (int e = 0; e < NE; e++) for (int f = 0; f < NE; f++) { size_t L = put(buf, e); L += put(buf + L, f); strcpy(buf + L, tails[ti]); for (int ty = 0; ty < 130; ty += ti == 0 ? 1 : 13) check(buf, 0, L, ty); }
    }
    printf("exhaustive 1- and 2-element strings over %d elements x token types x 5 trailing contexts: checked=%llu mismatches=%llu\n", NE, (unsigned long long)checked, (unsigned long long)bad);
    uint64_t c0 = checked, b0 = bad;
    for (int e = 0; e < NS; e++) for (int f = 0; f < NS; f++) for (int g = 0; g < NS; g++) { size_t L = put(buf, small[e]); L += put(buf + L, small[f]); L += put(buf + L, small[g]); buf[L] = '.'; buf[L + 1] = 0;
        for (int ty = 0; ty < 130; ty++) check(buf, 0, L, ty); }
    printf("exhaustive 3-element strings over %d elements x 130 types: checked=%llu mismatches=%llu\n", NS, (unsigned long long)(checked - c0), (unsigned long long)(bad - b0));
    c0 = checked; b0 = bad;
    for (uint64_t i = 0; i < n; i++) { size_t L = 0, k = 1 + rnd() % 24; int mode = rnd() % 3;
        for (size_t j = 0; j < k; j++) { uint64_t r = rnd(); int e = mode == 0 ? (int)(r % NE) : mode == 1 ? small[r % NS] : (r % 4 == 0 ? (int)(127 + (r >> 8) % NMB) : (int)(32 + (r >> 8) % 95)); L += put(buf + L, e); }
        size_t extra = rnd() % 4; for (size_t j = 0; j < extra; j++) buf[L + j] = "5.\xC2x"[rnd() % 4]; buf[L + extra] = 0;
        check(buf, 0, L, rnd() % 130); }
    printf("random: checked=%llu mismatches=%llu\n", (unsigned long long)(checked - c0), (unsigned long long)(bad - b0));
    return bad != 0;
}
