// Prints how address_parser_normalize_string transforms each single ASCII byte (as a 1-char string, and inside "a<c>b"), and the corpus ASCII line that mismatched lowercase.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"
#include "address_parser.h"
int main(int argc, char **argv) {
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    char b[8]; int changed_alone = 0, changed_inside = 0;
    for (int c = 1; c < 128; c++) {
        b[0] = c; b[1] = 0; char *r = address_parser_normalize_string(b);
        char e = (c >= 'A' && c <= 'Z') ? c + 32 : c;
        int same = r && strlen(r) == 1 && r[0] == e;
        b[0] = 'a'; b[1] = c; b[2] = 'b'; b[3] = 0; char *r2 = address_parser_normalize_string(b);
        int same2 = r2 && strlen(r2) == 3 && r2[0] == 'a' && r2[1] == e && r2[2] == 'b';
        if (!same) changed_alone++; if (!same2) changed_inside++;
        if (!same || !same2) { printf("byte 0x%02x alone -> [", c); for (char *p = r; p && *p; p++) printf("\\x%02x", (unsigned char)*p); printf("]  inside a?b -> ["); for (char *p = r2; p && *p; p++) printf("\\x%02x", (unsigned char)*p); printf("]\n"); }
        free(r); free(r2);
    }
    printf("bytes whose single-char result differs from lowercase: %d ; inside a?b: %d\n", changed_alone, changed_inside);
    FILE *f = fopen(argv[1], "r"); char *line = NULL; size_t lc = 0; ssize_t len;
    while ((len = getline(&line, &lc, f)) >= 0) { while (len && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0; int ascii = 1; for (ssize_t i = 0; i < len; i++) if ((unsigned char)line[i] >= 128) ascii = 0; if (!ascii) continue;
        char *r = address_parser_normalize_string(line); char exp[512]; size_t n = strlen(line); if (n > 400) continue; for (size_t i = 0; i <= n; i++) exp[i] = (line[i] >= 'A' && line[i] <= 'Z') ? line[i] + 32 : line[i];
        if (!r || strcmp(r, exp)) printf("CORPUS MISMATCH input=[%s] got=[%s]\n", line, r ? r : "(null)"); free(r); }
    return 0;
}
