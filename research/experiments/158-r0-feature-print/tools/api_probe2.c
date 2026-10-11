// 158: 025's api_probe.c (modes default, features, lifecycle, lifecycle_disable kept verbatim in behavior) plus datadir and lifecycle modes.
// usage: api_probe2 MODE [DATADIR]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"

static const char *SHORT = "123 Main Street Springfield Illinois 62704 USA";
static const char *MEDIUM = "123 Main Street Apartment Five Near Central Park Springfield Sangamon County Illinois 62704 USA";
// 41 tokens: longer than the default CRF context (10 items) and than the first lean capacity (32).
static const char *LONG = "Attn Receiving Dock Building Seven Suite 4400 Floor 12 Room 9 The Very Long Business Park Center Complex 1600 North Old Mill Creek "
                          "Industrial Parkway Extension East Unit B Lower Level Springfield Sangamon County Illinois 62704 9921 United States Of America";

static int parse_print(const char *input) {
    libpostal_address_parser_options_t options = libpostal_get_address_parser_default_options();
    libpostal_address_parser_response_t *r = libpostal_parse_address((char *)input, options);
    if (!r) return 0;
    printf("components=%zu\n", r->num_components);
    for (size_t i = 0; i < r->num_components; i++) printf("%s\t%s\n", r->labels[i], r->components[i]);
    libpostal_address_parser_response_destroy(r);
    fflush(stdout);
    return 1;
}
#define STEP(label, expr) do { int ok_ = (expr) ? 1 : 0; printf("%s=%d\n", label, ok_); fflush(stdout); if (!ok_) return 10; } while (0)

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    const char *m = argv[1];
    char *dir = argc > 2 ? argv[2] : NULL;

    if (strncmp(m, "datadir", 7) == 0) {
        if (dir == NULL) return 2;
        STEP("setup_datadir", libpostal_setup_datadir(dir));
        if (strcmp(m, "datadir_bad") == 0) {
            // A missing parser directory must fail cleanly; then the real one must still load and parse.
            printf("setup_parser_datadir(missing)=%d\n", libpostal_setup_parser_datadir("/nonexistent/libpostal-158") ? 1 : 0); fflush(stdout);
            STEP("setup_parser_datadir", libpostal_setup_parser_datadir(dir));
            STEP("parse", parse_print(SHORT));
        } else {
            STEP("setup_parser_datadir", libpostal_setup_parser_datadir(dir));
            if (strcmp(m, "datadir_features") == 0) STEP("print_features(true)", libpostal_parser_print_features(true));
            STEP("parse", parse_print(SHORT));
            STEP("parse", parse_print(MEDIUM));
            if (strcmp(m, "datadir_lifecycle") == 0) {
                // teardown / re-setup three times: plain, with feature printing, with printing switched off again
                libpostal_teardown_parser();
                STEP("setup_parser_datadir#2", libpostal_setup_parser_datadir(dir));
                STEP("parse", parse_print(MEDIUM));
                libpostal_teardown_parser();
                STEP("setup_parser_datadir#3", libpostal_setup_parser_datadir(dir));
                STEP("print_features(true)", libpostal_parser_print_features(true));
                STEP("parse", parse_print(SHORT));
                STEP("parse", parse_print(MEDIUM));
                libpostal_teardown_parser();
                STEP("setup_parser_datadir#4", libpostal_setup_parser_datadir(dir));
                STEP("print_features(false)", libpostal_parser_print_features(false));
                STEP("parse", parse_print(MEDIUM));
            }
        }
        libpostal_teardown_parser();
        libpostal_teardown();
        return 0;
    }

    if (strcmp(m, "lifecycle_long") == 0) {
        // Long address in the first parser lifetime, then again after teardown + re-setup (separate mode: probes CRF state kept across setups).
        if (!libpostal_setup() || !libpostal_setup_parser()) return 3;
        STEP("parse", parse_print(LONG));
        libpostal_teardown_parser();
        STEP("setup_parser#2", libpostal_setup_parser());
        STEP("parse", parse_print(LONG));
        libpostal_teardown_parser();
        libpostal_teardown();
        return 0;
    }

    // ---- 025 modes (unchanged behavior) ----
    if (!libpostal_setup() || !libpostal_setup_parser()) return 3;
    if (strcmp(m, "features") == 0 && !libpostal_parser_print_features(true)) return 4;
    const char *input = strncmp(m, "lifecycle", 9) == 0 ? MEDIUM : SHORT;
    if (!parse_print(input)) return 5;
    if (strncmp(m, "lifecycle", 9) == 0) {
        libpostal_teardown_parser();
        if (strcmp(m, "lifecycle_disable") == 0) setenv("CRF_INT_KEYS_DISABLE", "1", 1);
        if (!libpostal_setup_parser()) return 6;
        if (!parse_print(input)) return 7;
    }
    libpostal_teardown_parser();
    libpostal_teardown();
    return 0;
}
