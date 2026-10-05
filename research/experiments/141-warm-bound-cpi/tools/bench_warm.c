// 041 diagnostic harness (no library change): for every address, parse once untimed, then parse the same address again and time only that second
// call, so the timed call finds this address's model data already cached. Reports warm µs/address per pass. One thread.
// usage: bench_warm target.tsv passes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <pthread/qos.h>
#include "libpostal.h"
static inline uint64_t nowns(void) { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW); }
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
    unsigned long long comps = 0;
    for (int p = 1; p <= passes; p++) {
        uint64_t tot = 0, cold = 0;
        for (size_t i = 0; i < n; i++) {
            uint64_t a = nowns(); libpostal_address_parser_response_t *r0 = libpostal_parse_address(addr[i], o); uint64_t b = nowns();
            libpostal_address_parser_response_destroy(r0);
            uint64_t c = nowns(); libpostal_address_parser_response_t *r = libpostal_parse_address(addr[i], o); uint64_t d = nowns();
            comps += r->num_components; libpostal_address_parser_response_destroy(r);
            tot += d - c; cold += b - a;
        }
        printf("pass=%d warm_us_per_addr=%.3f first_call_us_per_addr=%.3f\n", p, tot / 1e3 / n, cold / 1e3 / n);
    }
    printf("components=%llu\n", comps);
    return 0;
}
