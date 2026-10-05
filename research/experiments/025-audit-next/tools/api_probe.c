#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpostal.h"

static void print_response(libpostal_address_parser_response_t *r) {
    printf("components=%zu\n", r->num_components);
    for (size_t i = 0; i < r->num_components; i++) printf("%s\t%s\n", r->labels[i], r->components[i]);
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    if (!libpostal_setup() || !libpostal_setup_parser()) return 3;
    if (strcmp(argv[1], "features") == 0 && !libpostal_parser_print_features(true)) return 4;
    libpostal_address_parser_options_t options = libpostal_get_address_parser_default_options();
    const char *input = strncmp(argv[1], "lifecycle", 9) == 0
        ? "123 Main Street Apartment Five Near Central Park Springfield Sangamon County Illinois 62704 USA"
        : "123 Main Street Springfield Illinois 62704 USA";
    libpostal_address_parser_response_t *r = libpostal_parse_address((char *)input, options);
    if (!r) return 5;
    print_response(r);
    libpostal_address_parser_response_destroy(r);
    if (strncmp(argv[1], "lifecycle", 9) == 0) {
        libpostal_teardown_parser();
        if (strcmp(argv[1], "lifecycle_disable") == 0) setenv("CRF_INT_KEYS_DISABLE", "1", 1);
        if (!libpostal_setup_parser()) return 6;
        r = libpostal_parse_address((char *)input, options);
        if (!r) return 7;
        print_response(r);
        libpostal_address_parser_response_destroy(r);
    }
    libpostal_teardown_parser();
    libpostal_teardown();
    return 0;
}
