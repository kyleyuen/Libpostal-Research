// 153 (PLAN A2): build "compiled lexicon records" from the WHOLE model (no data input) and prove the format is exact and fits memory.
// Inputs: the model (crf_load, ORIGINAL library) for rows; 143's keydump TSVs (re-generated, hashes equal to 143's) for keys and ids.
// Grouping rule (reproducible at runtime from strings the parser already has, with no key building):
//   unit record U:<unit>  word, prev word, next word, first word, phrase, sub_word, prefix, suffix, commonly *, postcode have/no
//                         context|w, the prev-tag families word|w and prev word|w; for "phrase type+phrase|T|P" and
//                         "unambiguous phrase type+phrase|T|P" the unit is P and T goes into the slot's family code
//   pair record B:<a|b>   prev word+word|a|b and word+next word|a|b
//   own record  K:<key>   everything else (bias, postcode have context, unambiguous phrase type|T, first word unknown+...)
//   Vocabulary frequency is stored in the unit record of the word (vocab-only words get a record of their own).
// Record (f64): u32 freq | u16 nslots | u16 keylen | key | pad8 | slots {u8 family, u8 kind S/P, u16 nnz, labels (1 B S / 2 B P), pad8,
//   f64 values}. The f32 variant is sized only. Index: open addressing, 8 B per slot (32-bit tag + offset/8), load <= 0.5.
// Verification: every model key -> record (index + full string compare) -> slot -> labels and value BITS equal the CSR row;
//   every vocabulary frequency read back equal. Exit 1 on any failure.
// usage: a2_records MODEL.dat KEYDIR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "crf.h"

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
static uint64_t fnv(const char *s, size_t n) { uint64_t h = 0xcbf29ce484222325ull; for (size_t i = 0; i < n; i++) { h ^= (uint8_t)s[i]; h *= 0x100000001b3ull; } return h; }
static void load(const char *dir, const char *name, char kind) {
    char p[4096]; snprintf(p, sizeof p, "%s/%s", dir, name); FILE *f = fopen(p, "r"); if (!f) { perror(p); exit(2); }
    char *line = NULL; size_t cap = 0; ssize_t n; long cnt = 0;
    while ((n = getline(&line, &cap, f)) > 0) {
        if (line[n - 1] == '\n') line[--n] = 0;
        char *tab = strrchr(line, '\t'); if (!tab) continue; *tab = 0;
        uint32_t id = (uint32_t)strtoul(tab + 1, NULL, 10); size_t kl = unesc(line); add(kind, line, kl, id); cnt++;
    }
    fclose(f); printf("%s: %ld keys\n", name, cnt);
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s MODEL KEYDIR\n", argv[0]); return 2; }
    crf_t *crf = crf_load(argv[1]); if (!crf) return 2;
    uint32_t *ipS = crf->weights->indptr->a, *ixS = crf->weights->indices->a; double *dS = crf->weights->data->a;
    uint32_t *ipP = crf->state_trans_weights->indptr->a, *ixP = crf->state_trans_weights->indices->a; double *dP = crf->state_trans_weights->data->a;
    load(argv[2], "state_features.tsv", 'S'); load(argv[2], "trans_features.tsv", 'P'); load(argv[2], "vocab.tsv", 'V');
    qsort(E, nE, sizeof(ent_t), cmpe);
    size_t nR = 0; for (size_t i = 0; i < nE; i++) if (!i || cmprk(&E[i - 1], &E[i])) nR++;
    uint64_t A64 = 0, A32 = 0, keybytes = 0, vocab_only = 0, dupslots = 0, nU = 0, nB = 0, nK = 0; size_t r = 0;
    uint64_t *roff = malloc(8 * nR); size_t *rbeg = malloc(sizeof(size_t) * (nR + 1));
    for (size_t i = 0; i < nE;) {
        size_t j = i; while (j < nE && !cmprk(&E[j], &E[i])) j++;
        uint64_t h = 8 + E[i].rkl, s64 = (h + 7) & ~7ull, s32 = (h + 3) & ~3ull; int slots = 0;
        for (size_t q = i; q < j; q++) {
            if (q > i && E[q].kind == E[q - 1].kind && E[q].fam == E[q - 1].fam) dupslots++;
            if (E[q].kind == 'V') continue; slots++;
            uint32_t n = E[q].kind == 'S' ? ipS[E[q].id + 1] - ipS[E[q].id] : ipP[E[q].id + 1] - ipP[E[q].id], lb = E[q].kind == 'S' ? n : 2 * n;
            s64 += (((4 + lb) + 7) & ~7ull) + 8ull * n; s32 += (((4 + lb) + 3) & ~3ull) + 4ull * n;
        }
        if (!slots) vocab_only++;
        nU += E[i].rk[0] == 'U'; nB += E[i].rk[0] == 'B'; nK += E[i].rk[0] == 'K';
        roff[r] = A64; rbeg[r] = i; A64 += s64; A32 += s32; keybytes += E[i].rkl; r++; i = j;
    }
    rbeg[nR] = nE;
    uint8_t *ar = calloc(1, A64);
    for (size_t k = 0; k < nR; k++) {
        uint8_t *p = ar + roff[k]; size_t i = rbeg[k], j = rbeg[k + 1]; uint32_t freq = 0; uint16_t ns = 0;
        for (size_t q = i; q < j; q++) { if (E[q].kind == 'V') freq = E[q].id; else ns++; }
        uint16_t kl = (uint16_t)E[i].rkl; memcpy(p, &freq, 4); memcpy(p + 4, &ns, 2); memcpy(p + 6, &kl, 2); memcpy(p + 8, E[i].rk, E[i].rkl);
        uint8_t *s = p + ((8 + E[i].rkl + 7) & ~7ull);
        for (size_t q = i; q < j; q++) {
            if (E[q].kind == 'V') continue;
            int S = E[q].kind == 'S'; uint32_t f = E[q].id, b = S ? ipS[f] : ipP[f], n = (S ? ipS[f + 1] : ipP[f + 1]) - b; uint16_t nn = (uint16_t)n;
            s[0] = E[q].fam; s[1] = E[q].kind; memcpy(s + 2, &nn, 2);
            for (uint32_t c = 0; c < n; c++) { if (S) s[4 + c] = (uint8_t)ixS[b + c]; else { uint16_t v = (uint16_t)ixP[b + c]; memcpy(s + 4 + 2 * c, &v, 2); } }
            double *v = (double *)(s + ((4 + (S ? n : 2 * n) + 7) & ~7ull));
            for (uint32_t c = 0; c < n; c++) v[c] = S ? dS[b + c] : dP[b + c];
            s = (uint8_t *)(v + n);
        }
    }
    uint64_t IS = 1; while (IS < 2 * (uint64_t)nR) IS <<= 1;
    uint64_t *idx = calloc(IS, 8);
    for (size_t k = 0; k < nR; k++) {
        size_t i = rbeg[k]; uint64_t h = fnv(E[i].rk, E[i].rkl), pos = h & (IS - 1);
        while (idx[pos]) pos = (pos + 1) & (IS - 1);
        idx[pos] = ((h >> 32) << 32) | (roff[k] / 8 + 1);
    }
    long ok = 0, bad = 0, unmapped = 0, vok = 0, vbad = 0; uint64_t probes = 0;
    for (size_t e = 0; e < nE; e++) {
        ent_t *x = &E[e]; uint64_t h = fnv(x->rk, x->rkl), pos = h & (IS - 1); uint8_t *p = NULL;
        while (idx[pos]) { probes++;
            if ((idx[pos] >> 32) == (h >> 32)) { uint8_t *c = ar + ((idx[pos] & 0xffffffffu) - 1) * 8; uint16_t kl; memcpy(&kl, c + 6, 2);
                if (kl == x->rkl && !memcmp(c + 8, x->rk, kl)) { p = c; break; } }
            pos = (pos + 1) & (IS - 1); }
        if (!p) { unmapped++; continue; }
        if (x->kind == 'V') { uint32_t fr; memcpy(&fr, p, 4); if (fr == x->id) vok++; else vbad++; continue; }
        uint16_t ns, kl; memcpy(&ns, p + 4, 2); memcpy(&kl, p + 6, 2); uint8_t *s = p + ((8 + kl + 7) & ~7ull); int found = 0;
        for (int q = 0; q < ns; q++) {
            uint16_t nn; memcpy(&nn, s + 2, 2); int S = s[1] == 'S'; double *v = (double *)(s + ((4 + (S ? nn : 2u * nn) + 7) & ~7ull));
            if (s[0] == x->fam && s[1] == x->kind) {
                found = 1; uint32_t f = x->id, b = S ? ipS[f] : ipP[f], n = (S ? ipS[f + 1] : ipP[f + 1]) - b; int same = n == nn;
                for (uint32_t c = 0; same && c < n; c++) {
                    uint32_t lab = S ? s[4 + c] : (uint32_t)(s[4 + 2 * c] | (s[5 + 2 * c] << 8)); double w = S ? dS[b + c] : dP[b + c];
                    if (lab != (S ? ixS[b + c] : ixP[b + c]) || memcmp(&v[c], &w, 8)) same = 0;
                }
                if (same) ok++; else bad++; break;
            }
            s = (uint8_t *)(v + nn);
        }
        if (!found) bad++;
    }
    long nS = crf->weights->m, nP = crf->state_trans_weights->m;
    printf("entries %zu = state %ld + trans %ld + vocab; records %zu (unit %llu, pair %llu, own %llu; vocab-only %llu); families %d; duplicate slots %llu\n",
           nE, nS, nP, nR, (unsigned long long)nU, (unsigned long long)nB, (unsigned long long)nK, (unsigned long long)vocab_only, nfams, (unsigned long long)dupslots);
    printf("verify: rows bit-identical %ld  mismatched %ld  unmapped %ld | vocab freq ok %ld bad %ld | mean index probes per lookup %.3f\n",
           ok, bad, unmapped, vok, vbad, (double)probes / nE);
    printf("sizes MB: arena f64 %.1f, f32 %.1f (record key bytes %.1f); index %.1f => total f64 %.1f, f32 %.1f\n",
           A64 / 1e6, A32 / 1e6, keybytes / 1e6, IS * 8 / 1e6, (A64 + IS * 8) / 1e6, (A32 + IS * 8) / 1e6);
    printf("original CSR arrays MB %.1f; cpi structures these replace (143 raw/cpi_dict_verbose: 398.2 + 56.4 + 26.0 + 48.9 + vocab 115.3) = 644.8\n",
           (4.0 * (nS + 1) + 12.0 * ipS[nS] + 4.0 * (nP + 1) + 12.0 * ipP[nP]) / 1e6);
    for (int i = 0; i < nfams; i++) printf("family %d: %s\n", i, fams[i]);
    return (bad || unmapped || vbad) ? 1 : 0;
}
