// 038 copy (adds feature-section times fs_t) of the 033 copy of the 016 T1/T2 driver, plus context-fill sub-stages ss_t[8..12] (EXP_CF_STATS).
// 016 T1/T2 driver (measurement only): per-stage µs and per-address counts for a stats build (EXP_STAGE_STATS).
// usage: stage_bench corpus.tsv passes   (tsv: country<TAB>address). One untimed warm-up pass over the corpus, then counters reset and `passes` timed passes.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mach/mach_time.h>
#include "libpostal.h"
extern uint64_t ss_t[16], ss_n[8];
extern uint64_t fs_t[8];
int main(int argc, char **argv) {
    if (argc < 3) return 2;
    FILE *f = fopen(argv[1], "r"); if (!f) return 2;
    char **addr = NULL; size_t n = 0, cap = 0; char *line = NULL; size_t lc = 0; ssize_t len;
    while ((len = getline(&line, &lc, f)) >= 0) {
        if (len && line[len - 1] == '\n') line[--len] = 0;
        char *tab = strchr(line, '\t'); if (!tab) return 2;
        if (n == cap) { cap = cap ? cap * 2 : 1024; addr = realloc(addr, cap * sizeof *addr); }
        addr[n++] = strdup(tab + 1);
    }
    int passes = atoi(argv[2]);
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    for (size_t i = 0; i < n; i++) libpostal_address_parser_response_destroy(libpostal_parse_address(addr[i], o));
    memset(ss_t, 0, sizeof(uint64_t) * 16); memset(fs_t, 0, sizeof(uint64_t) * 8); memset(ss_n, 0, sizeof(uint64_t) * 8);
    uint64_t t_total = 0, t_destroy = 0;
    for (int p = 0; p < passes; p++) for (size_t i = 0; i < n; i++) {
        uint64_t a = mach_absolute_time();
        libpostal_address_parser_response_t *r = libpostal_parse_address(addr[i], o);
        uint64_t b = mach_absolute_time();
        libpostal_address_parser_response_destroy(r);
        uint64_t c = mach_absolute_time();
        t_total += c - a; t_destroy += c - b;
    }
    mach_timebase_info_data_t tb; mach_timebase_info(&tb);
    double k = (double)tb.numer / tb.denom / 1000.0 / ((double)n * passes);   // ticks -> µs per address
    double pred = ss_t[3] * k, feat = ss_t[5] * k, look = ss_t[6] * k, acc = ss_t[7] * k;
    double staged = (ss_t[0] + ss_t[1] + ss_t[2] + ss_t[3] + ss_t[4]) * k + t_destroy * k;
    printf("file=%s addresses=%zu passes=%d\n", argv[1], n, passes);
    printf("total_us=%.3f (parse+destroy per address, timers included)\n", t_total * k);
    printf("normalize_us=%.3f\ntokenize_us=%.3f\ncontext_fill_us=%.3f\nfeatures_us=%.3f\nlookups_us=%.3f\naccumulate_us=%.3f\nviterbi_and_rest_of_predict_us=%.3f\nresponse_build_us=%.3f\nresponse_destroy_us=%.3f\n",
        ss_t[0] * k, ss_t[1] * k, ss_t[2] * k, feat, look, acc, pred - feat - look - acc, ss_t[4] * k, t_destroy * k);
    printf("cf_normalize_tokens_us=%.3f\ncf_address_dictionary_us=%.3f\ncf_prefix_suffix_us=%.3f\ncf_component_phrases_us=%.3f\ncf_postal_codes_us=%.3f\n", ss_t[8] * k, ss_t[9] * k, ss_t[10] * k, ss_t[11] * k, ss_t[12] * k);
    printf("fs_setup_us=%.3f\nfs_address_dict_block_us=%.3f\nfs_component_block_us=%.3f\nfs_postcode_block_us=%.3f\nfs_word_block_us=%.3f\nfs_trans_word_us=%.3f\nfs_prev_word_block_us=%.3f\nfs_next_word_block_us=%.3f\n", fs_t[0]*k, fs_t[1]*k, fs_t[2]*k, fs_t[3]*k, fs_t[4]*k, fs_t[5]*k, fs_t[6]*k, fs_t[7]*k);
    printf("unattributed_us=%.3f (setup of parse call, bypass paths, timer overhead)\n", t_total * k - staged);
    double na = (double)n * passes;
    printf("per_address: bytes=%.2f tokens=%.3f state_features=%.2f transition_features=%.2f bypassed=%.4f\n",
        ss_n[5] / na, ss_n[1] / na, ss_n[2] / na, ss_n[3] / na, ss_n[4] / na);
    return 0;
}
