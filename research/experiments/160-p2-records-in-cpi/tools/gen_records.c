// 160 (P2): offline generator of the compiled-record file (format: sources/rec_format.h), derived from the model only.
// Inputs: the model directory (crf_load of the preserved ORIGINAL libpostal.a gives the rows) and 143's keydump TSVs (keys, feature ids,
// vocabulary counts; hashes equal to 143's). No corpus input.
// Output: OUT (record file) and IDS (u32 original feature id per slot, in arena order; read only by the oracle-dump build).
// Verification (exit 1 on any failure) reads the WRITTEN file back through mmap: every model key -> index probe -> record -> slot ->
// labels and value bits equal the CSR row; every vocabulary count and ordinal read back equal; no slot is duplicated.
// usage: gen_records MODEL_DIR KEYDIR OUT IDS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "crf.h"
#include "rec_format.h"

typedef struct { char *rk; uint32_t rkl; uint8_t fam, kind; uint32_t id; } ent_t;   // rk includes the prefix byte; kind 'S', 'P', 'V' (id = ordinal)
static char *fams[256]; static int nfams;
static uint8_t famcode(const char *f, size_t n) {
    for (int i = 0; i < nfams; i++) if (strlen(fams[i]) == n && !memcmp(fams[i], f, n)) return (uint8_t)i;
    if (nfams == 254) { fprintf(stderr, "too many families\n"); exit(3); }
    fams[nfams] = strndup(f, n); return (uint8_t)nfams++;
}
static size_t unesc(char *s) {   // keydump escaping: \\ \t \n \r \0
    char *r = s, *w = s;
    while (*r) { if (*r == '\\' && r[1]) { r++; *w++ = *r == 't' ? '\t' : *r == 'n' ? '\n' : *r == 'r' ? '\r' : *r == '0' ? 0 : *r; r++; } else *w++ = *r++; }
    return (size_t)(w - s);
}
static ent_t *E; static size_t nE, capE;
static void add(char kind, const char *key, size_t kl, uint32_t id) {
    if (nE == capE) { capE = capE ? capE * 2 : 1 << 20; E = realloc(E, capE * sizeof(ent_t)); if (!E) exit(4); }
    ent_t *e = &E[nE++]; e->kind = (uint8_t)kind; e->id = id;
    char pfx; const char *rk, *fam; size_t rkl, fl;
    if (kind == 'V') { pfx = 'U'; rk = key; rkl = kl; e->fam = 255; }
    else { rec_group(key, kl, &pfx, &rk, &rkl, &fam, &fl); e->fam = famcode(fam, fl); }
    if (rkl + 1 > 65535) { fprintf(stderr, "key too long\n"); exit(3); }
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
static long load(const char *dir, const char *name, char kind) {
    char p[4096]; snprintf(p, sizeof p, "%s/%s", dir, name); FILE *f = fopen(p, "r"); if (!f) { perror(p); exit(2); }
    char *line = NULL; size_t cap = 0; ssize_t n; long cnt = 0;
    while ((n = getline(&line, &cap, f)) > 0) {
        if (line[n - 1] == '\n') line[--n] = 0;
        char *tab = strrchr(line, '\t'); if (!tab) { fprintf(stderr, "bad line in %s\n", name); exit(2); } *tab = 0;
        uint32_t id = (uint32_t)strtoul(tab + 1, NULL, 10); size_t kl = unesc(line);
        add(kind, line, kl, id); cnt++;
    }
    free(line); fclose(f); printf("%s: %ld keys\n", name, cnt); return cnt;
}
static uint64_t fsize(const char *dir, const char *name) { char p[4096]; struct stat st; snprintf(p, sizeof p, "%s/%s", dir, name); return stat(p, &st) == 0 ? (uint64_t)st.st_size : 0; }
static const uint8_t *probe(const uint8_t *ar, const uint64_t *idx, uint64_t IS, const char *k, uint32_t kl) {   // k includes the prefix byte
    uint64_t h = rec_hash(k[0], k + 1, kl - 1), pos = h & (IS - 1);
    while (idx[pos]) { if ((idx[pos] >> 32) == (h >> 32)) { const uint8_t *c = ar + ((idx[pos] & 0xffffffffu) - 1) * 8;
            if (rec_keylen(c) == kl && !memcmp(c + REC_HDR, k, kl)) return c; } pos = (pos + 1) & (IS - 1); }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s MODEL_DIR KEYDIR OUT IDS\n", argv[0]); return 2; }
    char mp[4096]; snprintf(mp, sizeof mp, "%s/address_parser_crf.dat", argv[1]);
    crf_t *crf = crf_load(mp); if (!crf) { fprintf(stderr, "crf_load failed\n"); return 2; }
    uint32_t *ipS = crf->weights->indptr->a, *ixS = crf->weights->indices->a; double *dS = crf->weights->data->a;
    uint32_t *ipP = crf->state_trans_weights->indptr->a, *ixP = crf->state_trans_weights->indices->a; double *dP = crf->state_trans_weights->data->a;
    famcode("own", 3);   // code 0
    load(argv[2], "state_features.tsv", 'S'); load(argv[2], "trans_features.tsv", 'P');
    size_t v0 = nE; uint32_t nV = (uint32_t)load(argv[2], "vocab.tsv", 'V');
    uint32_t *vcount = malloc(4ull * nV); for (uint32_t i = 0; i < nV; i++) { vcount[i] = E[v0 + i].id; E[v0 + i].id = i; }   // V: id = ordinal (dump order)
    for (size_t i = 0; i < nE; i++) {
        if (E[i].kind == 'S' && E[i].id >= crf->weights->m) { fprintf(stderr, "state id out of range\n"); return 1; }
        if (E[i].kind == 'P' && E[i].id >= crf->state_trans_weights->m) { fprintf(stderr, "trans id out of range\n"); return 1; }
    }
    qsort(E, nE, sizeof(ent_t), cmpe);
    size_t nR = 0; for (size_t i = 0; i < nE; i++) if (!i || cmprk(&E[i - 1], &E[i])) nR++;
    uint64_t A = 0, nslot = 0, dup = 0; uint64_t *roff = malloc(8 * nR); size_t *rbeg = malloc(sizeof(size_t) * (nR + 1)); size_t r = 0;
    for (size_t i = 0; i < nE;) {
        size_t j = i; while (j < nE && !cmprk(&E[j], &E[i])) j++;
        uint64_t s = (REC_HDR + E[i].rkl + 7) & ~7ull; size_t nv = 0;
        for (size_t q = i; q < j; q++) {
            if (q > i && E[q].kind == E[q - 1].kind && E[q].fam == E[q - 1].fam) dup++;
            if (E[q].kind == 'V') { nv++; continue; }
            int S = E[q].kind == 'S'; uint32_t n = S ? ipS[E[q].id + 1] - ipS[E[q].id] : ipP[E[q].id + 1] - ipP[E[q].id];
            if (n > 65535) { fprintf(stderr, "row too long\n"); return 1; }
            s += ((4 + (S ? n : 2ull * n) + 7) & ~7ull) + 8ull * n; nslot++;
        }
        if (nv > 1 || j - i - nv > 65535) { fprintf(stderr, "bad record group\n"); return 1; }
        roff[r] = A; rbeg[r] = i; A += s; r++; i = j;
    }
    rbeg[nR] = nE;
    if (dup) { fprintf(stderr, "duplicate (record, kind, family) slots: %llu\n", (unsigned long long)dup); return 1; }
    if (A / 8 + 1 >= 0xffffffffull) { fprintf(stderr, "arena too large for 32-bit offsets\n"); return 1; }
    uint8_t *ar = calloc(1, A ? A : 1); uint32_t *ids = malloc(4 * (nslot ? nslot : 1)); uint32_t *vlist = malloc(4ull * nV); uint64_t si = 0;
    for (uint32_t i = 0; i < nV; i++) vlist[i] = 0xffffffffu;
    for (size_t k = 0; k < nR; k++) {
        uint8_t *p = ar + roff[k]; size_t i = rbeg[k], j = rbeg[k + 1]; uint32_t freq = 0, vord = REC_NOVOCAB; uint16_t ns = 0;
        for (size_t q = i; q < j; q++) { if (E[q].kind == 'V') { vord = E[q].id; freq = vcount[vord]; vlist[vord] = (uint32_t)(roff[k] / 8); } else ns++; }
        uint16_t kl = (uint16_t)E[i].rkl; memcpy(p, &freq, 4); memcpy(p + 4, &vord, 4); memcpy(p + 8, &ns, 2); memcpy(p + 10, &kl, 2); memcpy(p + REC_HDR, E[i].rk, E[i].rkl);
        uint8_t *s = p + ((REC_HDR + E[i].rkl + 7) & ~7ull);
        for (size_t q = i; q < j; q++) {
            if (E[q].kind == 'V') continue;
            int S = E[q].kind == 'S'; uint32_t f = E[q].id, b = S ? ipS[f] : ipP[f], n = (S ? ipS[f + 1] : ipP[f + 1]) - b; uint16_t nn = (uint16_t)n;
            s[0] = E[q].fam; s[1] = E[q].kind; memcpy(s + 2, &nn, 2);
            for (uint32_t c = 0; c < n; c++) {
                uint32_t lab = S ? ixS[b + c] : ixP[b + c];
                if (S) { if (lab > 255) { fprintf(stderr, "state label > 255\n"); return 1; } s[4 + c] = (uint8_t)lab; }
                else { if (lab > 65535) { fprintf(stderr, "trans label > 65535\n"); return 1; } uint16_t v = (uint16_t)lab; memcpy(s + 4 + 2 * c, &v, 2); }
            }
            double *v = (double *)(s + ((4 + (S ? n : 2 * n) + 7) & ~7ull));
            for (uint32_t c = 0; c < n; c++) v[c] = S ? dS[b + c] : dP[b + c];
            ids[si++] = f; s = (uint8_t *)(v + n);
        }
    }
    for (uint32_t i = 0; i < nV; i++) if (vlist[i] == 0xffffffffu) { fprintf(stderr, "vocab ordinal %u has no record\n", i); return 1; }
    uint64_t IS = 1; while (IS < 2 * (uint64_t)nR) IS <<= 1;
    uint64_t *idx = calloc(IS, 8);
    for (size_t k = 0; k < nR; k++) {
        size_t i = rbeg[k]; uint64_t h = rec_hash(E[i].rk[0], E[i].rk + 1, E[i].rkl - 1), pos = h & (IS - 1);
        while (idx[pos]) pos = (pos + 1) & (IS - 1);
        idx[pos] = ((h >> 32) << 32) | (roff[k] / 8 + 1);
    }
    uint64_t fb = 0; for (int i = 0; i < nfams; i++) fb += 1 + strlen(fams[i]);
    rec_header_t H; memset(&H, 0, sizeof H); memcpy(H.magic, REC_MAGIC, 8); H.version = REC_VERSION; H.nfam = (uint32_t)nfams;
    H.nrec = nR; H.nslot = nslot; H.nvocab = nV; H.fam_off = sizeof H; H.fam_bytes = fb;
    H.arena_off = (H.fam_off + fb + 16383) & ~16383ull; H.arena_bytes = A;
    H.index_off = (H.arena_off + A + 16383) & ~16383ull; H.index_slots = IS;
    H.vlist_off = H.index_off + IS * 8; H.file_size = H.vlist_off + 4ull * nV;
    H.model_size = fsize(argv[1], "address_parser_crf.dat"); H.vocab_size = fsize(argv[1], "address_parser_vocab.trie");
    FILE *o = fopen(argv[3], "wb"); if (!o) { perror(argv[3]); return 2; }
    static const uint8_t zero[16384];
    fwrite(&H, sizeof H, 1, o);
    for (int i = 0; i < nfams; i++) { uint8_t l = (uint8_t)strlen(fams[i]); fwrite(&l, 1, 1, o); fwrite(fams[i], 1, l, o); }
    fwrite(zero, 1, H.arena_off - (H.fam_off + fb), o); fwrite(ar, 1, A, o);
    fwrite(zero, 1, H.index_off - (H.arena_off + A), o); fwrite(idx, 8, IS, o); fwrite(vlist, 4, nV, o);
    if (fclose(o) != 0) { perror("write"); return 2; }
    FILE *oi = fopen(argv[4], "wb"); if (!oi || fwrite(ids, 4, nslot, oi) != nslot || fclose(oi) != 0) { perror(argv[4]); return 2; }
    printf("records %zu slots %llu vocab %u families %d | arena %.1f MB index %.1f MB vlist %.1f MB file %.1f MB\n", nR, (unsigned long long)nslot, nV, nfams,
           A / 1e6, IS * 8 / 1e6, 4.0 * nV / 1e6, H.file_size / 1e6);
    for (int i = 0; i < nfams; i++) printf("family %d: %s\n", i, fams[i]);
    free(ar); free(idx); free(vlist);

    // ---- verification from the written file ----
    int fd = open(argv[3], O_RDONLY); struct stat st; if (fd < 0 || fstat(fd, &st) != 0 || (uint64_t)st.st_size != H.file_size) { fprintf(stderr, "reopen/size failed\n"); return 1; }
    const uint8_t *m = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0); if (m == MAP_FAILED) { perror("mmap"); return 1; }
    rec_header_t R; memcpy(&R, m, sizeof R);
    if (memcmp(R.magic, REC_MAGIC, 8) || R.version != REC_VERSION || R.nrec != nR) { fprintf(stderr, "header mismatch\n"); return 1; }
    const uint8_t *far = m + R.arena_off; const uint64_t *fidx = (const uint64_t *)(m + R.index_off); const uint32_t *fvl = (const uint32_t *)(m + R.vlist_off);
    long ok = 0, bad = 0, unmapped = 0, vok = 0, vbad = 0;
    for (size_t e = 0; e < nE; e++) {
        ent_t *x = &E[e]; const uint8_t *p = probe(far, fidx, R.index_slots, x->rk, x->rkl);
        if (!p) { unmapped++; continue; }
        if (x->kind == 'V') { if (rec_vord(p) == x->id && rec_freq(p) == vcount[x->id] && far + 8ull * fvl[x->id] == p) vok++; else vbad++; continue; }
        const uint8_t *s = rec_slot(p, x->fam, x->kind);
        if (!s) { bad++; continue; }
        int S = x->kind == 'S'; uint32_t f = x->id, b = S ? ipS[f] : ipP[f], n = (S ? ipS[f + 1] : ipP[f + 1]) - b; int same = n == slot_nnz(s);
        const double *v = slot_values(s);
        for (uint32_t c = 0; same && c < n; c++) {
            uint32_t lab = S ? s[4 + c] : (uint32_t)(s[4 + 2 * c] | (s[5 + 2 * c] << 8)); double w = S ? dS[b + c] : dP[b + c];
            if (lab != (S ? ixS[b + c] : ixP[b + c]) || memcmp(&v[c], &w, 8)) same = 0;
        }
        if (same) ok++; else bad++;
    }
    uint64_t flagged = 0; for (size_t k = 0; k < R.index_slots; k++) if (fidx[k]) { const uint8_t *c = far + ((fidx[k] & 0xffffffffu) - 1) * 8; if (rec_vord(c) != REC_NOVOCAB) flagged++; }
    long nS = crf->weights->m, nP = crf->state_trans_weights->m;
    printf("verify (written file): rows bit-identical %ld mismatched %ld unmapped %ld | vocab ok %ld bad %ld | records flagged vocab %llu of vocab keys %u | model keys state %ld trans %ld\n",
           ok, bad, unmapped, vok, vbad, (unsigned long long)flagged, nV, nS, nP);
    int fail = bad || unmapped || vbad || flagged != nV || ok != nS + nP;
    printf("VERIFY %s\n", fail ? "FAIL" : "PASS");
    return fail ? 1 : 0;
}
