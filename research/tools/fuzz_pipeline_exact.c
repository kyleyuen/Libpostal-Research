// Exact-output counterpart to fuzz_pipeline.c. Same deterministic mutation stream,
// but records each generated input and ordered result as length-prefixed hex bytes.
// usage: fuzz_pipeline_exact seed count corpus1.txt [corpus2.txt ...]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libpostal.h"
static uint64_t rs;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static char **base; static size_t nbase;
static const char *pool[] = {"é","ß","ñ","ü","ø","å","ł","ž","č","ő","Ä","Ö","İ","ı","Ж","д","я","中","文","日","本","한","글","ع","ب","🙂","–","—","’","“","”","№","½","²","ﬁ","\xC2\xA0","\xE2\x80\x8B"};
static const char *ents[] = {"&amp;","&#39;","&quot;","&lt;","&gt;","&eacute;","&nbsp;","&","&#x27;","&AMP;","&copy;","&#0;"};
static const char *ctrl[] = {"\t","\r","\n","\x01","\x07","\x0b","\x0c","\x1b","\x7f"," ","  "};
static const char *misc[] = {"-",".","'","/",",",";",":","#","0","1","12","9","3rd","st.","U.S.A.","a.b.c","x-y","123-456","(",")","\"","@","*"};
static void hex(const unsigned char *p, size_t n) {
    static const char h[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) { putchar(h[p[i] >> 4]); putchar(h[p[i] & 15]); }
}
static void emit_input(long it, const char *status, const char *in, size_t n) {
    printf("%ld|%zu|", it, n); hex((const unsigned char *)in, n); printf("|%s", status);
}
static int utf8_ok(const unsigned char *p, size_t n) {   // strict UTF-8 validity (no overlongs, no surrogates, <= U+10FFFF)
    size_t i = 0;
    while (i < n) {
        unsigned char c = p[i];
        if (c < 0x80) { i++; continue; }
        size_t l; uint32_t cp;
        if (c >= 0xC2 && c <= 0xDF) { l = 2; cp = c & 0x1F; } else if (c >= 0xE0 && c <= 0xEF) { l = 3; cp = c & 0x0F; } else if (c >= 0xF0 && c <= 0xF4) { l = 4; cp = c & 0x07; } else return 0;
        if (i + l > n) return 0;
        for (size_t k = 1; k < l; k++) { if ((p[i + k] & 0xC0) != 0x80) return 0; cp = (cp << 6) | (p[i + k] & 0x3F); }
        if ((l == 3 && cp < 0x800) || (l == 4 && cp < 0x10000) || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return 0;
        i += l;
    }
    return 1;
}
static size_t put(char *dst, size_t cap, size_t n, const char *s) { size_t l = strlen(s); if (n + l >= cap) return n; memcpy(dst + n, s, l); return n + l; }
int main(int argc, char **argv) {
    if (argc < 4) return 2;
    rs = strtoull(argv[1], 0, 10) * 0x9E3779B97F4A7C15ULL + 88172645463325252ULL; if (!rs) rs = 1;
    long count = atol(argv[2]); const char *show = getenv("FUZZ_SHOW"); long show_i = show ? atol(show) : -1;
    size_t cap = 0; char *line = NULL; size_t lc = 0; ssize_t len;
    for (int a = 3; a < argc; a++) { FILE *f = fopen(argv[a], "r"); if (!f) return 2;
        while ((len = getline(&line, &lc, f)) >= 0) { while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0; if (!len) continue;
            if (nbase == cap) { cap = cap ? cap * 2 : 4096; base = realloc(base, cap * sizeof *base); } base[nbase++] = strdup(line); } fclose(f); }
    if (!show && (!libpostal_setup() || !libpostal_setup_parser())) return 2;
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    char in[1024], tmp[1024];
    for (long it = 0; it < count; it++) {
        const char *b = base[rnd() % nbase]; size_t n = 0; in[0] = 0;
        if (rnd() % 8 == 0) { n = put(in, sizeof in, 0, b); n = put(in, sizeof in, n, ", "); n = put(in, sizeof in, n, base[rnd() % nbase]); } else n = put(in, sizeof in, 0, b);
        in[n] = 0; int m = rnd() % 4;
        for (int k = 0; k < m && n > 0; k++) {
            int op = rnd() % 14; size_t p = rnd() % (n + 1); size_t t = 0;
            switch (op) {
            case 0: if (n) in[rnd() % n] = (char)(1 + rnd() % 255); break;
            case 1: if (n > 1) { memmove(in + p, in + p + (p < n), n - p); n -= (p < n); } break;
            case 2: t = put(tmp, sizeof tmp, 0, pool[rnd() % (sizeof pool / sizeof *pool)]); if (n + t < sizeof in) { memmove(in + p + t, in + p, n - p); memcpy(in + p, tmp, t); n += t; } break;
            case 3: t = put(tmp, sizeof tmp, 0, ents[rnd() % (sizeof ents / sizeof *ents)]); if (n + t < sizeof in) { memmove(in + p + t, in + p, n - p); memcpy(in + p, tmp, t); n += t; } break;
            case 4: t = put(tmp, sizeof tmp, 0, ctrl[rnd() % (sizeof ctrl / sizeof *ctrl)]); if (n + t < sizeof in) { memmove(in + p + t, in + p, n - p); memcpy(in + p, tmp, t); n += t; } break;
            case 5: t = put(tmp, sizeof tmp, 0, misc[rnd() % (sizeof misc / sizeof *misc)]); if (n + t < sizeof in) { memmove(in + p + t, in + p, n - p); memcpy(in + p, tmp, t); n += t; } break;
            case 6: for (size_t i = 0; i < n; i++) if (in[i] >= 'a' && in[i] <= 'z') in[i] -= 32; break;
            case 7: for (size_t i = 0; i < n; i++) if (in[i] >= 'A' && in[i] <= 'Z') in[i] += 32; break;
            case 8: if (n > 2) n = 1 + rnd() % (n - 1); break;
            case 9: { size_t c = n; while (c > 0 && n + c + 1 < 400) { memcpy(in + n, in, c); n += c; if (rnd() % 2) break; } } break;
            case 10: for (size_t i = 0; i < n; i++) if (in[i] == ' ' && rnd() % 3 == 0) in[i] = ','; break;
            case 11: for (size_t i = 0; i < n; i++) if (in[i] >= 'a' && in[i] <= 'z' && rnd() % 12 == 0) in[i] = (char)('0' + rnd() % 10); break;
            case 12: if (n > 3) { size_t q = rnd() % (n - 2); char c = in[q]; in[q] = in[q + 1]; in[q + 1] = c; } break;
            default: if (n + 2 < sizeof in) { memmove(in + 1, in, n); in[0] = ' '; n++; } break;
            }
            in[n] = 0;
        }
        for (size_t i = 0; i < n; i++) if (in[i] == 0) in[i] = 'x';
        if (n == 0) { if (!show) { emit_input(it, "EMPTY", in, n); putchar('\n'); } continue; }
        if (!utf8_ok((unsigned char *)in, n)) { if (!show) { emit_input(it, "INVALID_UTF8", in, n); putchar('\n'); } if (show && it == show_i) { printf("%s\n", in); return 0; } continue; }   // baseline crashes on invalid UTF-8; no defined behaviour to preserve
        if (show) { if (it == show_i) { printf("%s\n", in); return 0; } continue; }
        libpostal_address_parser_response_t *r = libpostal_parse_address(in, o);
        if (!r) { emit_input(it, "NULL", in, n); putchar('\n'); continue; }
        emit_input(it, "OK", in, n); printf("|%zu", r->num_components);
        for (size_t i = 0; i < r->num_components; i++) {
            size_t ll = strlen(r->labels[i]), cl = strlen(r->components[i]);
            printf("|%zu|", ll); hex((const unsigned char *)r->labels[i], ll);
            printf("|%zu|", cl); hex((const unsigned char *)r->components[i], cl);
        }
        putchar('\n');
        libpostal_address_parser_response_destroy(r);
    }
    return 0;
}
