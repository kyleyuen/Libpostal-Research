// 140: 003's dump_outputs through libpostal_parse_address_batch (all lines, one batch). Correctness only, never timed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"
bool libpostal_parse_address_batch(char **addrs, size_t n, libpostal_address_parser_options_t options, libpostal_address_parser_response_t **out);   // experiment 140
static void esc(const char *s) { putchar('"'); for (; *s; s++) { unsigned char c = *s; if (c == '"' || c == '\\') { putchar('\\'); putchar(c); } else if (c < 0x20) printf("\\u%04x", c); else putchar(c); } putchar('"'); }
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "r"); char *line = NULL; size_t cap = 0; ssize_t len;
    if (!f || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    char **L = NULL; size_t n = 0, m = 0;
    while ((len = getline(&line, &cap, f)) >= 0) {
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        if (!len) continue;
        if (n == m) { m = m ? m * 2 : 1024; L = realloc(L, m * sizeof *L); } L[n++] = strdup(line);
    }
    libpostal_address_parser_response_t **out = malloc((n ? n : 1) * sizeof *out);
    libpostal_parse_address_batch(L, n, o, out);
    for (size_t k = 0; k < n; k++) {
        libpostal_address_parser_response_t *r = out[k];
        if (!r) return 3;
        printf("{\"address\":"); esc(L[k]); printf(",\"components\":[");
        for (size_t i = 0; i < r->num_components; i++) { if (i) putchar(','); putchar('['); esc(r->labels[i]); putchar(','); esc(r->components[i]); putchar(']'); }
        printf("]}\n");
        libpostal_address_parser_response_destroy(r);
    }
    libpostal_teardown_parser(); libpostal_teardown();
    return 0;
}
