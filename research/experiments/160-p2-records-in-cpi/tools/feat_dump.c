// 158: dump_outputs (003) with libpostal_parser_print_features(true). Per input: "#ADDR <address>", then the library's own printed
// feature lines (stdout), then the ordered (label, component) JSON line. Correctness only, never timed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"
static void esc(const char *s) { putchar('"'); for (; *s; s++) { unsigned char c = *s; if (c == '"' || c == '\\') { putchar('\\'); putchar(c); } else if (c < 0x20) printf("\\u%04x", c); else putchar(c); } putchar('"'); }
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "r"); char *line = NULL; size_t cap = 0; ssize_t len;
    if (!f || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    if (!libpostal_parser_print_features(true)) return 4;
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    while ((len = getline(&line, &cap, f)) >= 0) {
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        if (!len) continue;
        printf("#ADDR "); esc(line); putchar('\n');
        libpostal_address_parser_response_t *r = libpostal_parse_address(line, o);
        if (!r) return 3;
        printf("{\"components\":[");
        for (size_t i = 0; i < r->num_components; i++) { if (i) putchar(','); putchar('['); esc(r->labels[i]); putchar(','); esc(r->components[i]); putchar(']'); }
        printf("]}\n");
        libpostal_address_parser_response_destroy(r);
    }
    libpostal_teardown_parser(); libpostal_teardown();
    return 0;
}
