// 160 (p2b): structural scan of the record file (no model load). Checks the property rec_vocab_freq relies on: a record that is not a
// vocabulary key (vord == REC_NOVOCAB) has freq 0, so "freq of any U record" == "vocabulary count or 0" as trie_get_data returns.
// Also: vlist[v] points at a record whose vord == v; every vocabulary record is a 'U' record; slot walk reaches nslot.
// usage: rec_scan FILE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rec_format.h"
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    int fd = open(argv[1], O_RDONLY); struct stat st; if (fd < 0 || fstat(fd, &st)) return 2;
    const uint8_t *m = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0); if (m == MAP_FAILED) return 2;
    rec_header_t H; memcpy(&H, m, sizeof H);
    const uint8_t *ar = m + H.arena_off, *r = ar; const uint32_t *vl = (const uint32_t *)(m + H.vlist_off);
    unsigned long long nonvocab_freq_nonzero = 0, nonvocab = 0, vocab = 0, vocab_not_U = 0, slots = 0, vlist_bad = 0, U = 0;
    for (uint64_t q = 0; q < H.nrec; q++) {
        uint32_t v = rec_vord(r);
        if (r[REC_HDR] == 'U') U++;
        if (v == REC_NOVOCAB) { nonvocab++; if (rec_freq(r)) nonvocab_freq_nonzero++; }
        else { vocab++; if (r[REC_HDR] != 'U') vocab_not_U++; if (v >= H.nvocab || ar + 8ull * vl[v] != r) vlist_bad++; }
        const uint8_t *s = rec_first_slot(r);
        for (uint32_t j = 0, ns = rec_nslots(r); j < ns; j++) { s = slot_next(s); slots++; }
        r = s;   // next record starts where the last slot (or the padded key) ends; same walk as crf.c's oracle init
    }
    printf("records %llu (U %llu) slots %llu (header %llu) vocab records %llu (header nvocab %llu) non-vocab %llu\n", (unsigned long long)H.nrec, U, slots,
           (unsigned long long)H.nslot, vocab, (unsigned long long)H.nvocab, nonvocab);
    printf("non-vocab records with freq != 0: %llu | vocab records not 'U': %llu | vlist mismatches: %llu | arena walk end - arena_bytes: %lld\n",
           nonvocab_freq_nonzero, vocab_not_U, vlist_bad, (long long)((r - ar) - (long long)H.arena_bytes));
    int ok = nonvocab_freq_nonzero == 0 && vocab_not_U == 0 && vlist_bad == 0 && slots == H.nslot && vocab == H.nvocab;
    printf("%s\n", ok ? "SCAN PASS" : "SCAN FAIL");
    return ok ? 0 : 1;
}
