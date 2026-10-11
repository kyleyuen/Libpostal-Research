// 155 step 3 (PLAN A4, pre-registered in README): replay the exact per-address RECORD-KEY stream (O3k/O3i, emission order, misses
// included) against the real A2 arena built from the whole model, with K3's exact f64 accumulate + Viterbi. Measures the record design's
// irreducible cost after context fill (block B_new): hash + index probe + full string compare per distinct record key per address
// (within-address reuse only), slot scan per emitted feature, emission-order accumulate, vectorized Viterbi. Labels must equal K0.
// The feature-function control flow and the unit strings are given (precomputed outside timing) -- an optimistic bound.
// usage: rec_replay MODEL.dat KEYDIR ORACLE_DIR PASSES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <float.h>
#include <time.h>
#include "crf.h"
#define LL 20
#define TMAX 4096
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
typedef struct { char *rk; uint32_t rkl; uint8_t fam, kind; uint32_t id; } ent_t;   // kind 'S', 'P' or 'V' (vocab: id = frequency)
static char *fams[256]; static int nfams;
static uint8_t famcode(const char *f, size_t n) {
    for (int i = 0; i < nfams; i++) if (strlen(fams[i]) == n && !memcmp(fams[i], f, n)) return (uint8_t)i;
    if (nfams == 255) { fprintf(stderr, "too many families\n"); exit(3); }
    fams[nfams] = strndup(f, n); return (uint8_t)nfams++;
}
static size_t unesc(char *s) {   // keydump escaping: \\ \t \n \r \0
    char *r = s, *w = s;
    while (*r) { if (*r == '\\' && r[1]) { r++; *w++ = *r == 't' ? '\t' : *r == 'n' ? '\n' : *r == 'r' ? '\r' : *r == '0' ? 0 : *r; r++; } else *w++ = *r++; }
    return (size_t)(w - s);
}
static int is_unit_family(const char *f, size_t n) {
    static const char *u[] = { "word", "prev word", "next word", "first word", "phrase", "sub_word", "prefix", "suffix",
                               "postcode have context", "postcode no context", NULL };
    for (int i = 0; u[i]; i++) if (strlen(u[i]) == n && !memcmp(u[i], f, n)) return 1;
    return n > 9 && !memcmp(f, "commonly ", 9);
}
static ent_t *E; static size_t nE, capE;
static void add(char kind, char *key, size_t kl, uint32_t id) {
    if (nE == capE) { capE = capE ? capE * 2 : 1 << 20; E = realloc(E, capE * sizeof(ent_t)); }
    ent_t *e = &E[nE++]; e->kind = (uint8_t)kind; e->id = id;
    char *bar = memchr(key, '|', kl); char pfx; const char *rk, *fam; size_t rkl, fl; static char fb[512];
    if (kind == 'V') { pfx = 'U'; rk = key; rkl = kl; fam = "freq"; fl = 4; }
    else if (!bar) { pfx = 'K'; rk = key; rkl = kl; fam = "own"; fl = 3; }
    else {
        fam = key; fl = (size_t)(bar - key); const char *arg = bar + 1; size_t al = kl - fl - 1;
        if ((fl == 18 && !memcmp(key, "phrase type+phrase", 18)) || (fl == 30 && !memcmp(key, "unambiguous phrase type+phrase", 30))) {
            const char *b2 = memchr(arg, '|', al);
            if (!b2) { pfx = 'K'; rk = key; rkl = kl; fam = "own"; fl = 3; }
            else { size_t tl = (size_t)(b2 - arg); int m = snprintf(fb, sizeof fb, "%.*s|%.*s", (int)fl, key, (int)tl, arg);
                   fam = fb; fl = (size_t)m; pfx = 'U'; rk = b2 + 1; rkl = al - tl - 1; }
        } else if (fl == 14 && (!memcmp(key, "prev word+word", 14) || !memcmp(key, "word+next word", 14))) { pfx = 'B'; rk = arg; rkl = al; }
        else if (is_unit_family(key, fl)) { pfx = 'U'; rk = arg; rkl = al; }
        else { pfx = 'K'; rk = key; rkl = kl; fam = "own"; fl = 3; }
    }
    e->fam = famcode(fam, fl);
    e->rk = malloc(rkl + 1); e->rk[0] = pfx; memcpy(e->rk + 1, rk, rkl); e->rkl = (uint32_t)rkl + 1;
}

static int cmprk(const ent_t *x, const ent_t *y) {
    uint32_t n = x->rkl < y->rkl ? x->rkl : y->rkl; int c = memcmp(x->rk, y->rk, n);
    if (c) return c; return x->rkl == y->rkl ? 0 : (x->rkl < y->rkl ? -1 : 1);
}
static int cmpe(const void *a, const void *b) {
    const ent_t *x = a, *y = b; int c = cmprk(x, y); if (c) return c;
    if (x->kind != y->kind) return x->kind - y->kind; return x->fam - y->fam;
}
static inline uint64_t hkey(const char *s, size_t n) {          // 8-byte-chunk multiplicative hash (stands for any fast string hash)
    uint64_t h = 0x9E3779B97F4A7C15ull ^ n;
    while (n >= 8) { uint64_t w; memcpy(&w, s, 8); h = (h ^ w) * 0xff51afd7ed558ccdull; h ^= h >> 32; s += 8; n -= 8; }
    uint64_t w = 0; memcpy(&w, s, n); h = (h ^ w) * 0xc4ceb9fe1a85ec53ull; h ^= h >> 29; return h;
}
static void loadk(const char *dir, const char *name, char kind) {
    char p[4096]; snprintf(p, sizeof p, "%s/%s", dir, name); FILE *f = fopen(p, "r"); if (!f) { perror(p); exit(2); }
    char *line = NULL; size_t cap = 0; ssize_t n;
    while ((n = getline(&line, &cap, f)) > 0) { if (line[n - 1] == '\n') line[--n] = 0; char *tab = strrchr(line, '\t'); if (!tab) continue; *tab = 0;
        uint32_t id = (uint32_t)strtoul(tab + 1, NULL, 10); size_t kl = unesc(line); add(kind, line, kl, id); }
    fclose(f);
}
// arena: record = u32 freq | u16 nslots | u16 keylen | key | pad8 | slots { u8 fam, u8 kind, u16 nnz, u32 pmask, labels, pad8, f64 values }
static uint8_t *ar; static uint64_t *idx, IS;
static crf_t *crf; static uint32_t *ipS, *ixS, *ipP, *ixP; static double *dS, *dP; static double trD[LL * LL];
static void build_arena(void) {
    qsort(E, nE, sizeof(ent_t), cmpe);
    size_t nR = 0; for (size_t i = 0; i < nE; i++) if (!i || cmprk(&E[i - 1], &E[i])) nR++;
    uint64_t A = 0; uint64_t *roff = malloc(8 * nR); size_t *rbeg = malloc(sizeof(size_t) * (nR + 1)); size_t r = 0;
    for (size_t i = 0; i < nE;) { size_t j = i; while (j < nE && !cmprk(&E[j], &E[i])) j++;
        uint64_t s = (8 + E[i].rkl + 7) & ~7ull;
        for (size_t q = i; q < j; q++) { if (E[q].kind == 'V') continue; int S = E[q].kind == 'S';
            uint32_t n = S ? ipS[E[q].id + 1] - ipS[E[q].id] : ipP[E[q].id + 1] - ipP[E[q].id]; s += ((8 + (S ? n : 2 * n) + 7) & ~7ull) + 8ull * n; }
        roff[r] = A; rbeg[r] = i; A += s; r++; i = j; }
    rbeg[nR] = nE; ar = aligned_alloc(128, (A + 127) & ~127ull); memset(ar, 0, A);
    for (size_t k = 0; k < nR; k++) {
        uint8_t *p = ar + roff[k]; size_t i = rbeg[k], j = rbeg[k + 1]; uint32_t freq = 0; uint16_t ns = 0;
        for (size_t q = i; q < j; q++) { if (E[q].kind == 'V') freq = E[q].id; else ns++; }
        uint16_t kl = (uint16_t)E[i].rkl; memcpy(p, &freq, 4); memcpy(p + 4, &ns, 2); memcpy(p + 6, &kl, 2); memcpy(p + 8, E[i].rk, E[i].rkl);
        uint8_t *s = p + ((8 + E[i].rkl + 7) & ~7ull);
        for (size_t q = i; q < j; q++) { if (E[q].kind == 'V') continue;
            int S = E[q].kind == 'S'; uint32_t f = E[q].id, b = S ? ipS[f] : ipP[f], n = (S ? ipS[f + 1] : ipP[f + 1]) - b; uint16_t nn = (uint16_t)n; uint32_t m = 0;
            s[0] = E[q].fam; s[1] = E[q].kind; memcpy(s + 2, &nn, 2);
            for (uint32_t c = 0; c < n; c++) { if (S) s[8 + c] = (uint8_t)ixS[b + c]; else { uint16_t v = (uint16_t)ixP[b + c]; memcpy(s + 8 + 2 * c, &v, 2); m |= 1u << (v / LL); } }
            memcpy(s + 4, &m, 4);
            double *v = (double *)(s + ((8 + (S ? n : 2 * n) + 7) & ~7ull)); for (uint32_t c = 0; c < n; c++) v[c] = S ? dS[b + c] : dP[b + c];
            s = (uint8_t *)(v + n); }
    }
    IS = 1; while (IS < 2 * (uint64_t)nR) IS <<= 1; idx = calloc(IS, 8);
    for (size_t k = 0; k < nR; k++) { size_t i = rbeg[k]; uint64_t h = hkey(E[i].rk, E[i].rkl), pos = h & (IS - 1);
        while (idx[pos]) pos = (pos + 1) & (IS - 1); idx[pos] = ((h >> 32) << 32) | (roff[k] / 8 + 1); }
    printf("arena %.1f MB, records %zu, index %.1f MB\n", A / 1e6, nR, IS * 8 / 1e6);
}
// ---------- per-address replay stream ----------
typedef struct { uint32_t rec; uint8_t fam, kind; } emit_t;           // rec = per-address distinct record index
static int nA, *Tof; static uint32_t *aRec0, *aTok0;                   // per address: first distinct-record index, first token index
static uint32_t *recStr0, *recLen; static char *strbuf; static size_t nstr, capstr, nrec, caprec;
static uint32_t *tokE0; static emit_t *em; static size_t nem, capem;   // per token: emits [tokE0[t], tokE0[t+1]), S emits first then P
static uint32_t *k0S, *k0P, *k0tS, *k0tP; static size_t nk0S, nk0P;    // K0 reference ids (hits only)
static FILE *gz(const char *dir, const char *name) { char cmd[4096]; snprintf(cmd, sizeof cmd, "gzip -dc '%s/%s.gz'", dir, name); return popen(cmd, "r"); }
static void load_stream(const char *dir) {
    char *line = NULL, *l2 = NULL; size_t lc = 0, lc2 = 0; int cap = 1 << 16; Tof = calloc(cap, sizeof(int)); nA = 0;
    FILE *f = gz(dir, "O4.txt");
    while (getline(&line, &lc, f) > 0) { char *p = line, *e; long a = strtol(p, &e, 10); p = e + 1; int t = (int)strtol(p, &e, 10); p = e + 1; if (*p != 'S') continue;
        while (a >= cap) { Tof = realloc(Tof, sizeof(int) * cap * 2); memset(Tof + cap, 0, sizeof(int) * cap); cap *= 2; }
        if (t + 1 > Tof[a]) Tof[a] = t + 1; if (a + 1 > nA) nA = (int)a + 1; }
    pclose(f);
    aTok0 = malloc(4 * (nA + 1)); uint32_t nt = 0; for (int a = 0; a < nA; a++) { aTok0[a] = nt; nt += Tof[a]; } aTok0[nA] = nt;
    tokE0 = calloc(nt + 1, 4); k0tS = calloc(nt + 1, 4); k0tP = calloc(nt + 1, 4); aRec0 = calloc(nA + 1, 4);
    FILE *fk = gz(dir, "O3k.txt"), *fi = gz(dir, "O3i.txt");
    long cur = -1, curA = -1; static uint64_t lh[8192]; static uint32_t lrec[8192]; uint32_t nloc = 0;
    while (getline(&line, &lc, fk) > 0 && getline(&l2, &lc2, fi) > 0) {
        char *p = line, *e; long a = strtol(p, &e, 10); p = e + 1; int t = (int)strtol(p, &e, 10); p = e + 1; char k = *p; p += 2; p = strchr(p, '\t') + 1;
        size_t n = strlen(p); if (n && p[n - 1] == '\n') p[--n] = 0;
        char *q = strrchr(l2, '\t') + 1; int hit = *q != '-'; uint32_t id = hit ? (uint32_t)strtoul(q, NULL, 10) : 0;
        if (a >= nA || t >= Tof[a]) { fprintf(stderr, "O3 outside O4 tokens\n"); exit(4); }
        while (curA < a) { curA++; aRec0[curA] = (uint32_t)nrec; nloc = 0; }
        long tk = aTok0[a] + t; while (cur < tk) { cur++; tokE0[cur] = (uint32_t)nem; k0tS[cur] = (uint32_t)nk0S; k0tP[cur] = (uint32_t)nk0P; }
        size_t kl = unesc(p); size_t before = nE; add(k == 'S' ? 'S' : 'P', p, kl, 0); ent_t *x = &E[before]; nE = before;   // reuse grouping, then drop
        uint64_t h = hkey(x->rk, x->rkl); uint32_t li = UINT32_MAX;
        for (uint32_t z = 0; z < nloc; z++) if (lh[z] == h && recLen[lrec[z]] == x->rkl && !memcmp(strbuf + recStr0[lrec[z]], x->rk, x->rkl)) { li = lrec[z]; break; }
        if (li == UINT32_MAX) {
            if (nrec == caprec) { caprec = caprec ? caprec * 2 : 1 << 16; recStr0 = realloc(recStr0, 4 * caprec); recLen = realloc(recLen, 4 * caprec); }
            if (nstr + x->rkl > capstr) { capstr = (capstr + x->rkl) * 2; strbuf = realloc(strbuf, capstr); }
            memcpy(strbuf + nstr, x->rk, x->rkl); recStr0[nrec] = (uint32_t)nstr; recLen[nrec] = x->rkl; nstr += x->rkl; li = (uint32_t)nrec++;
            if (nloc < 8192) { lh[nloc] = h; lrec[nloc++] = li; }
        }
        free(x->rk);
        if (nem == capem) { capem = capem ? capem * 2 : 1 << 20; em = realloc(em, capem * sizeof(emit_t)); }
        em[nem++] = (emit_t){ li - aRec0[a], x->fam, (uint8_t)(k == 'S' ? 'S' : 'P') };
        if (hit) { if (k == 'S') { k0S = realloc(k0S, 4 * (nk0S + 1)); k0S[nk0S++] = id; } else { k0P = realloc(k0P, 4 * (nk0P + 1)); k0P[nk0P++] = id; } }
    }
    pclose(fk); pclose(fi);
    while (curA < nA) { curA++; aRec0[curA] = (uint32_t)nrec; }
    while (cur < (long)nt) { cur++; tokE0[cur] = (uint32_t)nem; k0tS[cur] = (uint32_t)nk0S; k0tP[cur] = (uint32_t)nk0P; }
}
// ---------- K0 reference (original CSR + library Viterbi) ----------
static void k0(int a, uint32_t *out) {
    int T = Tof[a]; crf_context_t *ctx = crf->context;
    crf_context_set_num_items(ctx, T); crf_context_reset(ctx, CRF_CONTEXT_RESET_ALL); double_matrix_copy(crf->trans_weights, ctx->trans);
    for (int t = 0; t < T; t++) { uint32_t tk = aTok0[a] + t; double *s = state_score(ctx, t), *st = state_trans_score_all(ctx, t);
        for (uint32_t q = k0tS[tk]; q < k0tS[tk + 1]; q++) { uint32_t f = k0S[q]; for (uint32_t c = ipS[f]; c < ipS[f + 1]; c++) s[ixS[c]] += dS[c]; }
        for (uint32_t q = k0tP[tk]; q < k0tP[tk + 1]; q++) { uint32_t f = k0P[q]; for (uint32_t c = ipP[f]; c < ipP[f + 1]; c++) st[ixP[c]] += dP[c]; } }
    crf_context_viterbi(ctx, out);
}
// ---------- record path ----------
static const uint8_t *rp[TMAX * 4]; static const uint8_t *slotp[TMAX * 64];
static double s3[TMAX][LL], st3[TMAX][LL * LL], al3[TMAX][LL]; static const double *rr[TMAX][LL]; static uint8_t bk[TMAX][LL]; static const double zrow[LL];
static inline const uint8_t *probe(const char *k, uint32_t kl) {
    uint64_t h = hkey(k, kl), pos = h & (IS - 1);
    while (idx[pos]) { if ((idx[pos] >> 32) == (h >> 32)) { const uint8_t *c = ar + ((idx[pos] & 0xffffffffu) - 1) * 8; uint16_t l; memcpy(&l, c + 6, 2);
            if (l == kl && !memcmp(c + 8, k, kl)) return c; } pos = (pos + 1) & (IS - 1); }
    return NULL;
}
static inline const uint8_t *find_slot(const uint8_t *r, uint8_t fam, uint8_t kind) {
    if (!r) return NULL; uint16_t ns, kl; memcpy(&ns, r + 4, 2); memcpy(&kl, r + 6, 2); const uint8_t *s = r + ((8 + kl + 7) & ~7u);
    for (int q = 0; q < ns; q++) { uint16_t nn; memcpy(&nn, s + 2, 2); uint32_t lb = s[1] == 'S' ? nn : 2u * nn;
        if (s[0] == fam && s[1] == kind) return s; s += ((8 + lb + 7) & ~7u) + 8u * nn; }
    return NULL;
}
static void krec(int a, uint32_t *out) {
    int T = Tof[a]; uint32_t r0 = aRec0[a], nr = aRec0[a + 1] - r0;
    for (uint32_t i = 0; i < nr; i++) rp[i] = probe(strbuf + recStr0[r0 + i], recLen[r0 + i]);
    for (int t = 0; t < T; t++) {
        uint32_t tk = aTok0[a] + t, e0 = tokE0[tk], e1 = tokE0[tk + 1]; double *s = s3[t], *st = st3[t]; uint32_t m = 0;
        memset(s, 0, 8 * LL);
        for (uint32_t e = e0; e < e1; e++) { const uint8_t *sl = find_slot(rp[em[e].rec], em[e].fam, em[e].kind); slotp[e - e0] = sl;
            if (sl && sl[1] == 'P') { uint32_t mm; memcpy(&mm, sl + 4, 4); m |= mm; } }
        for (int i = 0; i < LL; i++) if (m >> i & 1) { memset(st + i * LL, 0, 8 * LL); rr[t][i] = st + i * LL; } else rr[t][i] = zrow;
        for (uint32_t e = e0; e < e1; e++) { const uint8_t *sl = slotp[e - e0]; if (!sl) continue; uint16_t nn; memcpy(&nn, sl + 2, 2);
            if (sl[1] == 'S') { const double *v = (const double *)(sl + ((8 + nn + 7) & ~7u)); for (uint32_t c = 0; c < nn; c++) s[sl[8 + c]] += v[c]; }
            else { const uint16_t *ix = (const uint16_t *)(sl + 8); const double *v = (const double *)(sl + ((8 + 2u * nn + 7) & ~7u)); for (uint32_t c = 0; c < nn; c++) st[ix[c]] += v[c]; } }
    }
    for (int j = 0; j < LL; j++) al3[0][j] = s3[0][j];
    for (int t = 1; t < T; t++) { double best[LL]; uint8_t arg[LL];
        for (int j = 0; j < LL; j++) { best[j] = -DBL_MAX; arg[j] = 0; }
        for (int i = 0; i < LL; i++) { double p = al3[t - 1][i]; const double *row = rr[t][i], *trr = &trD[i * LL];
            for (int j = 0; j < LL; j++) { double sc = (p + row[j]) + trr[j]; int mm = best[j] < sc; best[j] = mm ? sc : best[j]; arg[j] = mm ? (uint8_t)i : arg[j]; } }
        for (int j = 0; j < LL; j++) { al3[t][j] = best[j] + s3[t][j]; bk[t][j] = arg[j]; } }
    double b = -DBL_MAX; int arg = 0; for (int j = 0; j < LL; j++) if (b < al3[T - 1][j]) { b = al3[T - 1][j]; arg = j; }
    for (int t = T - 1; t >= 0; t--) { out[t] = arg; if (t) arg = bk[t][arg]; }
}
typedef void (*kfn)(int, uint32_t *); static uint32_t ob[TMAX]; static volatile uint32_t sink;
static double pass(kfn k, int twice) { double t0 = now(); for (int a = 0; a < nA; a++) { if (!Tof[a]) continue; k(a, ob); if (twice) k(a, ob); sink += ob[0]; } return now() - t0; }
int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s MODEL KEYDIR ORACLE_DIR PASSES\n", argv[0]); return 2; }
    int P = atoi(argv[4]);
    crf = crf_load(argv[1]); if (!crf || crf->num_classes != LL) return 2;
    ipS = crf->weights->indptr->a; ixS = crf->weights->indices->a; dS = crf->weights->data->a;
    ipP = crf->state_trans_weights->indptr->a; ixP = crf->state_trans_weights->indices->a; dP = crf->state_trans_weights->data->a;
    for (int c = 0; c < LL * LL; c++) trD[c] = crf->trans_weights->values[c];
    loadk(argv[2], "state_features.tsv", 'S'); loadk(argv[2], "trans_features.tsv", 'P'); loadk(argv[2], "vocab.tsv", 'V');
    build_arena();
    load_stream(argv[3]);
    long n = 0, bad = 0, nT = 0; static uint32_t ref[TMAX], got[TMAX];
    for (int a = 0; a < nA; a++) { int T = Tof[a]; if (!T) continue; if (T > TMAX) return 5; n++; nT += T; k0(a, ref); krec(a, got); for (int t = 0; t < T; t++) bad += ref[t] != got[t]; }
    printf("addresses %ld tokens/addr %.3f distinct records/addr %.2f emits/addr %.2f  label mismatches record path vs K0: %ld\n",
           n, (double)nT / n, (double)nrec / n, (double)nem / n, bad); fflush(stdout);
    double *c = malloc(sizeof(double) * P), w[5];
    for (int p = 0; p < P; p++) c[p] = pass(krec, 0) / n * 1e6;
    for (int r = 0; r < 5; r++) { double t1 = pass(krec, 0), t2 = pass(krec, 1); w[r] = (t2 - t1) / n * 1e6; }
    double first = c[0]; qsort(c + 1, P - 1, sizeof(double), cmpd); qsort(w, 5, sizeof(double), cmpd);
    printf("B_new record path: pass1 %.3f  cold median p2..%d %.3f [%.3f-%.3f]  warm %.3f [%.3f-%.3f] us/addr\n", first, P, c[1 + (P - 1) / 2], c[1], c[P - 1], w[2], w[0], w[4]);
    return bad ? 1 : 0;
}
