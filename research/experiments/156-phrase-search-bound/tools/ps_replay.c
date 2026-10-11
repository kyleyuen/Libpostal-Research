// 156 (pre-registered in README): context fill's phrase searches (address dictionary over normalized tokens, component phrases and
// postal codes over admin tokens) replayed with hash sets built from the MODEL's phrase keys (143 keydump TSVs), emulating
// trie_search_tokens_from_index's greedy longest match. Exactness: per-token memberships (start,len,data) vs the 142 O2 oracle,
// with the component validity filter applied. Timing: cold passes over the corpus + warm marginal repeat. Does not link libpostal.
// usage: ps_replay KEYDIR ORACLE_DIR PASSES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static inline uint64_t hkey(const char *s, size_t n) {
    uint64_t h = 0x9E3779B97F4A7C15ull ^ n;
    while (n >= 8) { uint64_t w; memcpy(&w, s, 8); h = (h ^ w) * 0xff51afd7ed558ccdull; h ^= h >> 32; s += 8; n -= 8; }
    uint64_t w = 0; memcpy(&w, s, n); h = (h ^ w) * 0xc4ceb9fe1a85ec53ull; h ^= h >> 29; return h;
}
// hash set: key -> (data+1 if it is a full key; cont = 1 if it is a token-boundary prefix of a longer key)
typedef struct { uint64_t h; uint32_t koff, data1; uint16_t klen; uint8_t cont, pad; } ent_t;
typedef struct { ent_t *e; uint64_t mask, n; char *keys; uint64_t nk, ck; } set_t;
static void set_init(set_t *s, uint64_t expect) { uint64_t c = 1; while (c < expect * 2) c <<= 1; s->e = calloc(c, sizeof(ent_t)); s->mask = c - 1; s->n = 0; }
static ent_t *set_put(set_t *s, const char *k, size_t kl) {
    uint64_t h = hkey(k, kl) | 1, p = h & s->mask;
    while (s->e[p].h) { if (s->e[p].h == h && s->e[p].klen == kl && !memcmp(s->keys + s->e[p].koff, k, kl)) return &s->e[p]; p = (p + 1) & s->mask; }
    if (s->nk + kl > s->ck) { s->ck = (s->ck + kl) * 2; s->keys = realloc(s->keys, s->ck); }
    memcpy(s->keys + s->nk, k, kl); s->e[p].h = h; s->e[p].koff = (uint32_t)s->nk; s->e[p].klen = (uint16_t)kl; s->nk += kl; s->n++; return &s->e[p];
}
static inline const ent_t *set_get(const set_t *s, const char *k, size_t kl) {
    uint64_t h = hkey(k, kl) | 1, p = h & s->mask;
    while (s->e[p].h) { if (s->e[p].h == h && s->e[p].klen == kl && !memcmp(s->keys + s->e[p].koff, k, kl)) return &s->e[p]; p = (p + 1) & s->mask; }
    return NULL;
}
static size_t unesc(char *s) {
    char *r = s, *w = s;
    while (*r) { if (*r == '\\' && r[1]) { r++; *w++ = *r == 't' ? '\t' : *r == 'n' ? '\n' : *r == 'r' ? '\r' : *r == '0' ? 0 : *r; r++; } else *w++ = *r++; }
    return (size_t)(w - s);
}
static void load_set(set_t *s, const char *dir, const char *name, const char *strip, uint64_t expect) {
    char p[4096]; snprintf(p, sizeof p, "%s/%s", dir, name); FILE *f = fopen(p, "r"); if (!f) { perror(p); exit(2); }
    char *line = NULL; size_t cap = 0; ssize_t n; long cnt = 0, lines = 0; size_t sl = strip ? strlen(strip) : 0;
    set_init(s, expect);
    while ((n = getline(&line, &cap, f)) > 0) {
        lines++; if (line[n - 1] == '\n') line[--n] = 0; char *tab = strrchr(line, '\t'); if (!tab) continue; *tab = 0;
        uint32_t data = (uint32_t)strtoul(tab + 1, NULL, 10); size_t kl = unesc(line); char *k = line;
        if (sl) { if (kl < sl || memcmp(k, strip, sl)) continue; k += sl; kl -= sl; }
        ent_t *e = set_put(s, k, kl); e->data1 = data + 1; cnt++;
        for (size_t i = 0; i < kl; i++) if (k[i] == ' ') set_put(s, k, i)->cont = 1;
    }
    fclose(f);
    printf("set %s: keys %ld (of %ld lines) entries %llu  table %.1f MB + keys %.1f MB = %.1f MB\n", name, cnt, lines, (unsigned long long)s->n,
           (s->mask + 1) * sizeof(ent_t) / 1e6, s->nk / 1e6, ((s->mask + 1) * sizeof(ent_t) + s->nk) / 1e6);
}
// oracle O2
typedef struct { uint32_t no, nl, ao, al; int64_t m[3][3]; } tok_t;
static tok_t *TK; static size_t nTK, capTK; static uint32_t *aT0; static int nA; static char *SB; static size_t nSB, capSB;
static uint32_t addstr(const char *s, size_t n) { if (nSB + n + 1 > capSB) { capSB = (capSB + n + 1) * 2; SB = realloc(SB, capSB); } memcpy(SB + nSB, s, n); SB[nSB + n] = 0; uint32_t o = (uint32_t)nSB; nSB += n + 1; return o; }
static void load_o2(const char *dir) {
    char cmd[4096]; snprintf(cmd, sizeof cmd, "gzip -dc '%s/O2.txt.gz'", dir); FILE *f = popen(cmd, "r");
    char *line = NULL; size_t cap = 0; ssize_t n; long capA = 1 << 16; aT0 = malloc(4 * capA); long cura = -1;
    while ((n = getline(&line, &cap, f)) > 0) {
        if (line[n - 1] == '\n') line[--n] = 0;
        char *fld[16]; int nf = 0; char *p = line; fld[nf++] = p;
        while ((p = strchr(p, '\t')) && nf < 16) { *p++ = 0; fld[nf++] = p; }
        long a = strtol(fld[0], NULL, 10);
        while (cura < a) { cura++; if (cura + 2 >= capA) { capA *= 2; aT0 = realloc(aT0, 4 * capA); } aT0[cura] = (uint32_t)nTK; }
        if (fld[1][0] == 'N') continue;
        if (nf < 7) { fprintf(stderr, "bad O2 line a=%ld\n", a); exit(4); }
        if (nTK == capTK) { capTK = capTK ? capTK * 2 : 1 << 18; TK = realloc(TK, capTK * sizeof(tok_t)); }
        tok_t *t = &TK[nTK++];
        size_t l1 = unesc(fld[2]); t->no = addstr(fld[2], l1); t->nl = (uint32_t)l1;
        size_t l2 = unesc(fld[3]); t->ao = addstr(fld[3], l2); t->al = (uint32_t)l2;
        for (int k = 0; k < 3; k++) { if (fld[4 + k][0] == '-') { t->m[k][0] = -1; continue; }
            long long x, y, z; sscanf(fld[4 + k], "%lld,%lld,%lld", &x, &y, &z); t->m[k][0] = x; t->m[k][1] = y; t->m[k][2] = z; }
    }
    pclose(f); nA = (int)cura + 1; aT0[nA] = (uint32_t)nTK;
}
// search with trie_search_tokens_from_index semantics (greedy longest match; restart at last+1 after a match, start+1 otherwise)
static char buf[1 << 16];
static void search(const set_t *s, const tok_t *t, int T, int admin, int64_t (*out)[3]) {
    for (int i = 0; i < T; i++) out[i][0] = -1;
    int i = 0;
    while (i < T) {
        int p = i, k = i, last = -1; uint32_t d = 0; size_t bl = 0;
        for (;;) {
            uint32_t o = admin ? t[k].ao : t[k].no, l = admin ? t[k].al : t[k].nl;
            if (l == 0 || bl + l + 1 > sizeof buf) break;
            if (k > p) buf[bl++] = ' ';
            memcpy(buf + bl, SB + o, l); bl += l;
            const ent_t *e = set_get(s, buf, bl);
            if (!e) break;
            if (e->data1) { last = k; d = e->data1 - 1; }
            if (!e->cont || k + 1 >= T) break;
            k++;
        }
        if (last >= 0) { for (int j = p; j <= last; j++) { out[j][0] = p; out[j][1] = last - p + 1; out[j][2] = d; } i = last + 1; }
        else i = p + 1;
    }
}
static set_t SD, SC, SP;
#define TMAXT (1 << 14)
static int64_t R[3][TMAXT][3];
static void run_addr(int a) {
    const tok_t *t = TK + aT0[a]; int T = (int)(aT0[a + 1] - aT0[a]);
    search(&SD, t, T, 0, R[0]); search(&SC, t, T, 1, R[1]); search(&SP, t, T, 1, R[2]);
    for (int i = 0; i < T;) {   // is_valid_component_phrase: drop phrases whose admin tokens are all digits
        if (R[1][i][0] < 0) { i++; continue; }
        int st = (int)R[1][i][0], ln = (int)R[1][i][1], valid = 0;
        for (int j = st; j < st + ln && !valid; j++) { const char *x = SB + t[j].ao; for (uint32_t c = 0; c < t[j].al; c++) if (x[c] < '0' || x[c] > '9') { valid = 1; break; } }
        if (!valid) for (int j = st; j < st + ln; j++) R[1][j][0] = -1;
        i = st + ln;
    }
}
static volatile int64_t sink;
static double pass(int twice) { double t0 = now(); for (int a = 0; a < nA; a++) { int T = (int)(aT0[a + 1] - aT0[a]); if (!T || T >= TMAXT) continue; run_addr(a); if (twice) run_addr(a); sink += R[1][0][0]; } return now() - t0; }
int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s KEYDIR ORACLE_DIR PASSES\n", argv[0]); return 2; }
    int P = atoi(argv[3]);
    load_set(&SD, argv[1], "address_dictionary.tsv", NULL, 300000);   /* lang NULL -> ROOT (address_dictionary.c:241): all keys */ load_set(&SC, argv[1], "component_phrases.tsv", NULL, 6000000);
    load_set(&SP, argv[1], "postal_codes.tsv", NULL, 7000000);
    load_o2(argv[2]);
    long n = 0, nt = 0, bad[3] = {0}, badA = 0, skipped = 0; const char *nm[3] = { "dictionary", "component", "postal" };
    for (int a = 0; a < nA; a++) {
        int T = (int)(aT0[a + 1] - aT0[a]); if (!T) continue; if (T >= TMAXT) { skipped++; continue; } n++; nt += T; run_addr(a);
        int any = 0;
        for (int k = 0; k < 3; k++) for (int i = 0; i < T; i++) {
            const int64_t *w = TK[aT0[a] + i].m[k], *g = R[k][i];
            int ok = (w[0] < 0 && g[0] < 0) || (w[0] >= 0 && g[0] == w[0] && g[1] == w[1] && g[2] == w[2]);
            if (!ok) { bad[k]++; any = 1; if (bad[k] <= 5) fprintf(stderr, "MISMATCH %s a=%d t=%d want %lld,%lld,%lld got %lld,%lld,%lld\n", nm[k], a, i,
                (long long)w[0], (long long)w[1], (long long)w[2], (long long)g[0], (long long)g[1], (long long)g[2]); }
        }
        badA += any;
    }
    printf("addresses %ld (skipped T>=%d: %ld) tokens %ld | token mismatches: dictionary %ld component %ld postal %ld | addresses with any mismatch %ld\n",
           n, TMAXT, skipped, nt, bad[0], bad[1], bad[2], badA);
    fflush(stdout);
    if (P > 0) {
        double *c = malloc(sizeof(double) * P), w[5];
        for (int p = 0; p < P; p++) c[p] = pass(0) / n * 1e6;
        for (int r = 0; r < 5; r++) { double t1 = pass(0), t2 = pass(1); w[r] = (t2 - t1) / n * 1e6; }
        double first = c[0]; qsort(c + 1, P - 1, sizeof(double), cmpd); qsort(w, 5, sizeof(double), cmpd);
        printf("PS_new phrase search: pass1 %.3f  cold median p2..%d %.3f [%.3f-%.3f]  warm %.3f [%.3f-%.3f] us/addr\n", first, P, c[1 + (P - 1) / 2], c[1], c[P - 1], w[2], w[0], w[4]);
    }
    return (bad[0] || bad[1] || bad[2]) ? 1 : 0;
}
