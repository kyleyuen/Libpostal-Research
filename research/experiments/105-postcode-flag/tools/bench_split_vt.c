// 043 diagnostic harness (no library change): links a stats build (EXP_STAGE_STATS [+ EXP_CF_STATS | EXP_FS_STATS]). Every address is parsed twice in a
// row; the stage timers are accumulated separately for the first ("cold") and the second ("warm") call. Prints µs/address per stage for both.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <pthread/qos.h>
#include "libpostal.h"
extern uint64_t ss_t[16], ss_n[8];
#ifdef HAVE_FS
extern uint64_t fs_t[8];
#endif
int main(int argc, char **argv) {
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    if (argc != 3) return 2;
    int passes = atoi(argv[2]); FILE *f = fopen(argv[1], "r"); if (!f) return 2;
    char **addr = NULL; size_t n = 0, cap = 0; char *line = NULL; size_t lc = 0; ssize_t len;
    while ((len = getline(&line, &lc, f)) >= 0) {
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        char *tab = strchr(line, '\t'); if (!tab) return 2;
        if (n == cap) { cap = cap ? cap * 2 : 1024; addr = realloc(addr, cap * sizeof *addr); }
        addr[n++] = strdup(tab + 1);
    }
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    for (size_t i = 0; i < n; i++) libpostal_address_parser_response_destroy(libpostal_parse_address(addr[i], o));   // warm-up pass, not counted
    uint64_t cold[24] = {0}, warm[24] = {0}, tcold = 0, twarm = 0;
    for (int p = 0; p < passes; p++) for (size_t i = 0; i < n; i++) {
        for (int c = 0; c < 2; c++) {
            uint64_t before[24]; memcpy(before, ss_t, 16 * sizeof(uint64_t));
#ifdef HAVE_FS
            memcpy(before + 16, fs_t, 8 * sizeof(uint64_t));
#endif
            uint64_t a = mach_absolute_time(); libpostal_address_parser_response_t *r = libpostal_parse_address(addr[i], o); uint64_t b = mach_absolute_time();
            libpostal_address_parser_response_destroy(r);
            uint64_t *acc = c == 0 ? cold : warm; if (c == 0) tcold += b - a; else twarm += b - a;
            for (int k = 0; k < 16; k++) acc[k] += ss_t[k] - before[k];
#ifdef HAVE_FS
            for (int k = 0; k < 8; k++) acc[16 + k] += fs_t[k] - before[16 + k];
#endif
        }
    }
    mach_timebase_info_data_t tb; mach_timebase_info(&tb); double k = (double)tb.numer / tb.denom / 1000.0 / ((double)n * passes);
    const char *nm[24] = {"normalize","tokenize","context_fill","predict_total","response","features","lookups","accumulate","cf_normalize_tokens","cf_address_dictionary","cf_prefix_suffix","cf_component_phrases","cf_postal_codes","vt_score_setup","vt_viterbi","s15",
        "fs_setup","fs_address_dict","fs_component","fs_postcode","fs_word","fs_trans_word","fs_prev_word","fs_next_word"};
    printf("total_parse cold_us=%.3f warm_us=%.3f\n", tcold * k, twarm * k);
    for (int q = 0; q < 24; q++) { if (cold[q] == 0 && warm[q] == 0) continue; printf("%s cold_us=%.3f warm_us=%.3f diff=%.3f\n", nm[q], cold[q] * k, warm[q] * k, (cold[q] - warm[q]) * k); }
    printf("viterbi_and_rest cold_us=%.3f warm_us=%.3f\n", (cold[3] - cold[5] - cold[6] - cold[7]) * k, (warm[3] - warm[5] - warm[6] - warm[7]) * k);
    return 0;
}
