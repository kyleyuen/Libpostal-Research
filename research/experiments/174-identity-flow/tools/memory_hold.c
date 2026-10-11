#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libpostal.h"

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    unsigned seconds = (unsigned)strtoul(argv[2], NULL, 10);
    if (!seconds || !libpostal_setup() || !libpostal_setup_parser()) return 2;
    libpostal_address_parser_options_t options = libpostal_get_address_parser_default_options();
    libpostal_address_parser_response_t *result = libpostal_parse_address(argv[1], options);
    if (result == NULL) return 3;
    libpostal_address_parser_response_destroy(result);
    fprintf(stderr, "READY pid=%d hold_seconds=%u\n", getpid(), seconds);
    fflush(stderr);
    sleep(seconds);
    libpostal_teardown_parser();
    libpostal_teardown();
    return 0;
}
