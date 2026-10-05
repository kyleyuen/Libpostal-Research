// Prints ordered (label, component) pairs for every address, one JSON line per input. Correctness only, never timed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"
static char *read_line(FILE *f) {
    size_t n = 0, cap = 256; int c; char *s = malloc(cap);
    if (!s) return NULL;
    while ((c = fgetc(f)) != EOF && c != '\n') {
        if (n + 1 == cap) { char *p = realloc(s, cap *= 2); if (!p) { free(s); return NULL; } s = p; }
        s[n++] = (char)c;
    }
    if (c == EOF && n == 0) { free(s); return NULL; }
    if (n && s[n - 1] == '\r') n--;
    s[n] = 0; return s;
}
static void esc(const char *s) { putchar('"'); for (; *s; s++) { unsigned char c = *s; if (c == '"' || c == '\\') { putchar('\\'); putchar(c); } else if (c < 0x20) printf("\\u%04x", c); else putchar(c); } putchar('"'); }
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "rb"); char *line;
    if (!f || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    libpostal_address_parser_options_t o = libpostal_get_address_parser_default_options();
    while ((line = read_line(f))) {
        if (!*line) { free(line); continue; }
        libpostal_address_parser_response_t *r = libpostal_parse_address(line, o);
        if (!r) return 3;
        printf("{\"address\":"); esc(line); printf(",\"components\":[");
        for (size_t i = 0; i < r->num_components; i++) { if (i) putchar(','); putchar('['); esc(r->labels[i]); putchar(','); esc(r->components[i]); putchar(']'); }
        printf("]}\n");
        libpostal_address_parser_response_destroy(r);
        free(line);
    }
    libpostal_teardown_parser(); libpostal_teardown();
    return 0;
}
