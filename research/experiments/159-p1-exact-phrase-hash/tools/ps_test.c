// 159 P1 harness (correctness only, never timed).
//   ps_test build STRUCTDIR OUTDIR            build dict.ps / comp.ps / postal.ps from trie_struct TSVs (model-derived keys only); print sizes
//   ps_test oracle TABLEDIR ORACLE_DIR [orig]  replay O1+O2: per address, the 3 searches by the hash path and by the reference port (and, with
//                                              "orig", by the ORIGINAL library on the real tries); phrase arrays must be identical, and the
//                                              per-token memberships (component validity filter applied) must equal O2
//   ps_test gen TABLEDIR STRUCTDIR SEED COUNT  model-derived adversarial token sequences (keys split anywhere, truncated, CJK per-char, empty,
//                                              whitespace and space-containing tokens), hash vs reference vs ORIGINAL
// env: PS_NEG="comp|postal|dict:escaped_key:xorflags" flips one entry's flags (negative control); PS_FORCE_FAST=1 bypasses the routing.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"
#include "address_parser.h"
#include "address_dictionary.h"
#include "string_utils.h"
#include "../src/ps_exact.h"

static ps_table_t *T[3]; static const char *TN[3] = { "dict", "comp", "postal" };
static trie_t *OT[3]; static int have_orig = 0;
static size_t unesc(char *s) {
    char *r = s, *w = s;
    while (*r) { if (*r == '\\' && r[1]) { r++; *w++ = *r == 't' ? '\t' : *r == 'n' ? '\n' : *r == 'r' ? '\r' : *r == '0' ? 0 : *r; r++; } else *w++ = *r++; }
    return (size_t)(w - s);
}
static void load_tables(const char *dir) {
    char p[4096];
    for (int k = 0; k < 3; k++) { snprintf(p, sizeof p, "%s/%s.ps", dir, TN[k]); T[k] = ps_table_load(p); if (!T[k]) { fprintf(stderr, "load %s failed\n", p); exit(2); } }
    const char *neg = getenv("PS_NEG");
    if (neg) {
        char b[4096]; snprintf(b, sizeof b, "%s", neg); char *c1 = strchr(b, ':'), *c2 = strrchr(b, ':'); *c1 = 0; *c2 = 0;
        int k = !strcmp(b, "dict") ? 0 : !strcmp(b, "comp") ? 1 : 2; size_t kl = unesc(c1 + 1);
        bool ok = ps_debug_flip(T[k], c1 + 1, kl, (uint8_t)atoi(c2 + 1));
        printf("NEGATIVE CONTROL: flip flags %d of %s entry '%.*s': %s\n", atoi(c2 + 1), TN[k], (int)kl, c1 + 1, ok ? "done" : "ENTRY NOT FOUND");
        if (!ok) exit(3);
    }
    if (getenv("PS_FORCE_FAST")) { ps_force_fast = 1; printf("PS_FORCE_FAST: routing to the reference disabled\n"); }
}
static void setup_orig(void) {
    if (!libpostal_setup() || !libpostal_setup_parser()) { fprintf(stderr, "libpostal setup failed\n"); exit(2); }
    address_parser_t *p = get_address_parser(); OT[1] = p->phrases; OT[2] = p->postal_codes; OT[0] = get_address_dictionary()->trie; have_orig = 1;
}
static bool orig_search(int k, char *str, token_array *tok, phrase_array **ph) {
    if (k == 0) return search_address_dictionaries_tokens_with_phrases(str, tok, NULL, ph);
    return trie_search_tokens_with_phrases(OT[k], str, tok, ph);
}
static int same(phrase_array *a, phrase_array *b) {
    if (a->n != b->n) return 0;
    for (size_t i = 0; i < a->n; i++) if (a->a[i].start != b->a[i].start || a->a[i].len != b->a[i].len || a->a[i].data != b->a[i].data) return 0;
    return 1;
}
static void dumpph(FILE *f, const char *nm, phrase_array *a) { fprintf(f, " %s:", nm); for (size_t i = 0; i < a->n; i++) fprintf(f, "(%u,%u,%u)", a->a[i].start, a->a[i].len, a->a[i].data); }
static void dumptok(FILE *f, char *str, token_array *t) { for (size_t i = 0; i < t->n; i++) fprintf(f, " [%u]'%.*s'", t->a[i].type, (int)t->a[i].len, str + t->a[i].offset); }

static phrase_array *PF, *PR, *PO;
static long nbad_ret[3], nbad_ref[3], nbad_orig[3], nrouted, ncalls;
static int run3(int k, char *str, token_array *tok, long id) {   // fast phrases left in PF
    phrase_array_clear(PF); phrase_array_clear(PR);
    bool rf = ps_search_tokens_with_phrases(T[k], str, tok, &PF);
    bool rr = ps_search_tokens_ref(T[k], str, tok, &PR);
    ncalls++; if (ps_needs_ref(str, tok)) nrouted++;
    int bad = 0;
    if (!same(PF, PR)) { nbad_ref[k]++; bad = 1; }
    if (rf != rr) { nbad_ret[k]++; bad = 1; }
    if (have_orig) {
        phrase_array_clear(PO); bool ro = orig_search(k, str, tok, &PO);
        if (!same(PF, PO) || rf != ro) { nbad_orig[k]++; bad = 1; }
        if (!same(PR, PO)) bad = 1;
    }
    if (bad && nbad_ref[k] + nbad_orig[k] + nbad_ret[k] <= 10) {
        fprintf(stderr, "MISMATCH %s id=%ld:", TN[k], id); dumptok(stderr, str, tok); dumpph(stderr, "fast", PF); dumpph(stderr, "ref", PR);
        if (have_orig) dumpph(stderr, "orig", PO); fprintf(stderr, "\n");
    }
    return bad;
}

// ---------------- oracle mode ----------------
typedef struct { char *s; size_t n, cap; } sbuf_t;
static void sb_add(sbuf_t *b, const char *s, size_t n) { if (b->n + n + 1 > b->cap) { b->cap = (b->n + n + 1) * 2; b->s = realloc(b->s, b->cap); } memcpy(b->s + b->n, s, n); b->s[b->n + n] = 0; b->n += n + 1; }
static int split(char *line, char **fld, int max) { int nf = 0; char *p = line; fld[nf++] = p; while ((p = strchr(p, '\t')) && nf < max) { *p++ = 0; fld[nf++] = p; } return nf; }
typedef struct { long long m[3][3]; } want_t;
static int oracle(const char *odir) {
    char cmd[4096]; snprintf(cmd, sizeof cmd, "gzip -dc '%s/O1.txt.gz'", odir); FILE *f1 = popen(cmd, "r");
    snprintf(cmd, sizeof cmd, "gzip -dc '%s/O2.txt.gz'", odir); FILE *f2 = popen(cmd, "r");
    char *l1 = NULL, *l2 = NULL; size_t c1 = 0, c2 = 0; ssize_t n1, n2;
    sbuf_t SN = {0}, SA = {0}; token_array *TKN = token_array_new(), *TKA = token_array_new();
    int64_array *mem = int64_array_new(); want_t *W = NULL; size_t *aoff = NULL; size_t wcap = 0;
    long addrs = 0, toks = 0, bad_mem[3] = {0}, bad_addr = 0;
    while ((n2 = getline(&l2, &c2, f2)) > 0) {
        if (l2[n2 - 1] == '\n') l2[--n2] = 0;
        char *fld[16]; int nf = split(l2, fld, 16);
        if (nf < 3 || fld[1][0] != 'N') { fprintf(stderr, "O2 desync\n"); return 4; }
        long a = atol(fld[0]); size_t nt = (size_t)atol(fld[2]);
        if ((n1 = getline(&l1, &c1, f1)) <= 0) { fprintf(stderr, "O1 short\n"); return 4; }
        { char *g[8]; split(l1, g, 8); if (atol(g[0]) != a || g[1][0] != 'N' || (size_t)atol(g[2]) != nt) { fprintf(stderr, "O1/O2 desync at a=%ld\n", a); return 4; } }
        SN.n = SA.n = 0; token_array_clear(TKN); token_array_clear(TKA);
        if (nt + 1 > wcap) { wcap = (nt + 1) * 2; W = realloc(W, wcap * sizeof *W); aoff = realloc(aoff, wcap * sizeof *aoff); }
        for (size_t t = 0; t < nt; t++) {
            if ((n1 = getline(&l1, &c1, f1)) <= 0 || (n2 = getline(&l2, &c2, f2)) <= 0) { fprintf(stderr, "short at a=%ld\n", a); return 4; }
            if (l1[n1 - 1] == '\n') l1[--n1] = 0;
            if (l2[n2 - 1] == '\n') l2[--n2] = 0;
            char *g[8]; split(l1, g, 8); uint16_t ty = (uint16_t)atoi(g[2]);
            nf = split(l2, fld, 16); if (nf < 7 || atol(fld[0]) != a || (size_t)atol(fld[1]) != t) { fprintf(stderr, "O2 bad line a=%ld t=%zu\n", a, t); return 4; }
            size_t ln = unesc(fld[2]), la = unesc(fld[3]);
            token_array_push(TKN, (token_t){SN.n, ln, ty}); sb_add(&SN, fld[2], ln);
            aoff[t] = SA.n; token_array_push(TKA, (token_t){SA.n, la, ty}); sb_add(&SA, fld[3], la);
            for (int k = 0; k < 3; k++) { if (fld[4 + k][0] == '-') W[t].m[k][0] = -1; else sscanf(fld[4 + k], "%lld,%lld,%lld", &W[t].m[k][0], &W[t].m[k][1], &W[t].m[k][2]); }
        }
        if (nt == 0) { addrs++; continue; }
        int abad = 0;
        for (int k = 0; k < 3; k++) {
            char *str = k == 0 ? SN.s : SA.s; token_array *tok = k == 0 ? TKN : TKA;
            abad |= run3(k, str, tok, a);
            int64_array_clear(mem); token_phrase_memberships(PF, mem, nt);
            if (k == 1) for (size_t i = 0; i < PF->n; i++) {   // is_valid_component_phrase (address_parser.c:675) on normalized_admin strings
                phrase_t ph = PF->a[i]; bool valid = false;
                for (uint32_t j = ph.start; j < ph.start + ph.len; j++) { char *s = SA.s + aoff[j]; if (!string_is_digit(s, strlen(s))) { valid = true; break; } }
                if (!valid) for (uint32_t j = ph.start; j < ph.start + ph.len; j++) mem->a[j] = NULL_PHRASE_MEMBERSHIP;
            }
            for (size_t t = 0; t < nt; t++) {
                int64_t m = mem->a[t]; long long *w = W[t].m[k]; int ok;
                if (m < 0) ok = w[0] < 0; else { phrase_t ph = PF->a[m]; ok = w[0] == ph.start && w[1] == ph.len && w[2] == ph.data; }
                if (!ok) { bad_mem[k]++; abad = 1; if (bad_mem[k] <= 5) fprintf(stderr, "O2 MISMATCH %s a=%ld t=%zu\n", TN[k], a, t); }
            }
        }
        bad_addr += abad; addrs++; toks += nt;
    }
    pclose(f1); pclose(f2);
    printf("addresses %ld tokens %ld | O2 token membership mismatches: dict %ld comp %ld postal %ld | phrase-array mismatches fast-vs-ref: %ld %ld %ld",
           addrs, toks, bad_mem[0], bad_mem[1], bad_mem[2], nbad_ref[0], nbad_ref[1], nbad_ref[2]);
    if (have_orig) printf(" | fast-vs-ORIGINAL: %ld %ld %ld", nbad_orig[0], nbad_orig[1], nbad_orig[2]);
    printf(" | return-value mismatches %ld | searches %ld, routed to reference %ld | addresses with any mismatch %ld\n",
           nbad_ret[0] + nbad_ret[1] + nbad_ret[2], ncalls, nrouted, bad_addr);
    return bad_addr ? 1 : 0;
}

// ---------------- generator mode (model-derived keys only) ----------------
static uint64_t rs = 88172645463325252ull;
static inline uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static inline int chance(int pct) { return (int)(rnd() % 100) < pct; }
typedef struct { char **k; uint32_t *l; size_t n; } klist_t;
static klist_t KL[3];
static void load_keys(int k, const char *path) {
    FILE *f = fopen(path, "r"); if (!f) { perror(path); exit(2); } char *line = NULL; size_t c = 0, cap = 1 << 16; ssize_t r;
    KL[k].k = malloc(cap * sizeof(char *)); KL[k].l = malloc(cap * 4);
    while ((r = getline(&line, &c, f)) > 0) { char *tab = strchr(line, '\t'); *tab = 0; size_t kl = unesc(line);
        if (KL[k].n == cap) { cap *= 2; KL[k].k = realloc(KL[k].k, cap * sizeof(char *)); KL[k].l = realloc(KL[k].l, cap * 4); }
        KL[k].k[KL[k].n] = malloc(kl + 1); memcpy(KL[k].k[KL[k].n], line, kl); KL[k].k[KL[k].n][kl] = 0; KL[k].l[KL[k].n] = (uint32_t)kl; KL[k].n++; }
    fclose(f); free(line);
}
static sbuf_t GS; static token_array *GT;
static void gtok(const char *s, size_t n, uint16_t type) { token_array_push(GT, (token_t){GS.n, n, type}); sb_add(&GS, s, n); }
static uint16_t pick_type(const char *s, size_t n, int abnormal_ok) {
    int hi = 0; for (size_t i = 0; i < n; i++) if ((unsigned char)s[i] >= 0x80) { hi = 1; break; }
    if (hi && chance(50)) return IDEOGRAPHIC_CHAR;
    if (abnormal_ok && chance(1)) return IDEOGRAPHIC_CHAR;   // ASCII "ideographic" token (routed)
    return chance(80) ? WORD : (uint16_t)(chance(50) ? NUMERIC : ABBREVIATION);
}
// emit bytes as tokens: split at spaces, and inside words at random byte positions (pct); utf8char: one IDEOGRAPHIC_CHAR token per UTF-8 char
static void emit_split(const char *s, size_t n, int pct, int utf8char, int abnormal_ok) {
    size_t i = 0;
    while (i < n) {
        if (s[i] == ' ') { if (abnormal_ok && chance(3)) gtok(" ", 1, WHITESPACE); i++; continue; }
        size_t j = i; while (j < n && s[j] != ' ') j++;
        size_t a = i;
        while (a < j) {
            size_t b;
            if (utf8char) { b = a + 1; while (b < j && ((unsigned char)s[b] & 0xC0) == 0x80) b++; }
            else { b = j; if (j - a > 1 && chance(pct)) b = a + 1 + rnd() % (j - a - 1); }
            uint16_t ty = utf8char ? IDEOGRAPHIC_CHAR : pick_type(s + a, b - a, abnormal_ok);
            gtok(s + a, b - a, ty); a = b;
        }
        i = j;
    }
}
static int gen(const char *sdir, long seed, long count) {
    const char *files[3] = { "address_dictionary", "component_phrases", "postal_codes" }; char p[4096];
    for (int k = 0; k < 3; k++) { snprintf(p, sizeof p, "%s/%s.struct.tsv", sdir, files[k]); load_keys(k, p); }
    rs ^= (uint64_t)seed * 0x9E3779B97F4A7C15ull; if (!rs) rs = 1;
    GT = token_array_new(); long bad = 0, cat[8] = {0};
    for (long c = 0; c < count; c++) {
        int k = (int)(rnd() % 3); GS.n = 0; token_array_clear(GT);
        int abnormal_ok = chance(10), segs = 1 + (int)(rnd() % 4);
        for (int s = 0; s < segs; s++) {
            int kind = (int)(rnd() % 8); cat[kind]++;
            size_t ki = rnd() % KL[k].n; const char *key = KL[k].k[ki]; size_t kl = KL[k].l[ki];
            switch (kind) {
            case 0: case 1: emit_split(key, kl, 0, 0, abnormal_ok); break;                         // whole key, split at spaces
            case 2: emit_split(key, kl, 40, 0, abnormal_ok); break;                                 // split inside words (strncmp quirk)
            case 3: emit_split(key, 1 + rnd() % kl, 30, 0, abnormal_ok); break;                     // truncated key (dead ends, "At last token")
            case 4: { size_t ki2 = (ki + 1 + rnd() % 3) % KL[k].n; const char *k2 = KL[k].k[ki2]; size_t l2 = KL[k].l[ki2];   // neighbour-key prefix, then a key
                      emit_split(k2, 1 + rnd() % l2, 20, 0, abnormal_ok); emit_split(key, kl, 10, 0, abnormal_ok); break; }
            case 5: emit_split(key, kl, 0, 1, abnormal_ok); break;                                  // one ideographic token per UTF-8 char
            case 6: { const char *junk[6] = { "zzq", "123", "st", "#", "-", "the" }; const char *j = junk[rnd() % 6]; gtok(j, strlen(j), WORD); break; }
            case 7: if (abnormal_ok) { int w = (int)(rnd() % 3); if (w == 0) gtok("", 0, WORD); else if (w == 1) gtok(" ", 1, WHITESPACE);
                                        else { size_t cut = kl < 6 ? kl : 6; gtok(key, cut, WORD); } }    // empty / whitespace / may contain ' '
                    else emit_split(key, kl, 0, 0, 0);
                    break;
            }
        }
        if (GT->n == 0) gtok("x", 1, WORD);
        bad += run3(k, GS.s, GT, c);
    }
    printf("gen seed %ld cases %ld | segment kinds", seed, count); for (int i = 0; i < 8; i++) printf(" %ld", cat[i]);
    printf(" | mismatches fast-vs-ref: %ld %ld %ld | fast-vs-ORIGINAL: %ld %ld %ld | return-value %ld | searches %ld routed %ld | cases with any mismatch %ld\n",
           nbad_ref[0], nbad_ref[1], nbad_ref[2], nbad_orig[0], nbad_orig[1], nbad_orig[2], nbad_ret[0] + nbad_ret[1] + nbad_ret[2], ncalls, nrouted, bad);
    return bad ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc >= 4 && !strcmp(argv[1], "build")) {
        const char *files[3] = { "address_dictionary", "component_phrases", "postal_codes" }; char p[4096], q[4096];
        for (int k = 0; k < 3; k++) {
            snprintf(p, sizeof p, "%s/%s.struct.tsv", argv[2], files[k]); snprintf(q, sizeof q, "%s/%s.ps", argv[3], TN[k]);
            ps_table_t *t = ps_table_build_tsv(p); if (!t) { fprintf(stderr, "build %s failed\n", p); return 2; }
            printf("%s: ", TN[k]); ps_table_stats(t, stdout);
            if (!ps_table_save(t, q)) { fprintf(stderr, "save %s failed\n", q); return 2; }
            ps_table_destroy(t);
        }
        return 0;
    }
    PF = phrase_array_new(); PR = phrase_array_new(); PO = phrase_array_new();
    if (argc >= 4 && !strcmp(argv[1], "oracle")) { load_tables(argv[2]); if (argc >= 5 && !strcmp(argv[4], "orig")) setup_orig(); return oracle(argv[3]); }
    if (argc >= 6 && !strcmp(argv[1], "gen")) { load_tables(argv[2]); setup_orig(); return gen(argv[3], atol(argv[4]), atol(argv[5])); }
    fprintf(stderr, "usage: see header\n"); return 2;
}
