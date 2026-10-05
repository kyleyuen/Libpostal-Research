// Read only the loaded model dictionaries; never inspect benchmark addresses.
// Output exact hexadecimal name bytes and uniqueness of (first byte, length).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "address_parser.h"
#include "feature_dict.h"

static void inventory(const char *kind, const feature_dict_t *d) {
    if (d == NULL || !d->split) {
        fprintf(stderr, "missing split dictionary: %s\n", kind);
        exit(3);
    }
    unsigned unique = 0, collisions = 0, long_names = 0, fast64 = 0;
    for (uint32_t i = 0; i < d->num_names; i++) {
        const fd_name_t *name = &d->names[i];
        unsigned peers = 0;
        uint8_t first = name->len ? (uint8_t)name->str[0] : 0;
        for (uint32_t j = 0; j < d->num_names; j++) {
            const fd_name_t *other = &d->names[j];
            if (other->len == name->len &&
                (other->len ? (uint8_t)other->str[0] : 0) == first) peers++;
        }
        int direct = name->len > 0 && name->len < 64 && peers == 1;
        unique += peers == 1;
        collisions += peers > 1;
        long_names += name->len >= 64;
        fast64 += direct;
        printf("name\t%s\t%u\t%u\t%u\t%u\t", kind, i, name->len, first, peers, direct);
        for (uint32_t k = 0; k < name->len; k++) printf("%02x", (uint8_t)name->str[k]);
        putchar('\t');
        for (uint32_t k = 0; k < name->len; k++) {
            uint8_t b = (uint8_t)name->str[k];
            putchar(b >= 0x20 && b <= 0x7e && b != '\\' ? b : '?');
        }
        putchar('\n');
    }
    printf("summary\t%s\tnames=%u\tunique=%u\tcolliding=%u\tlen_ge_64=%u\tfast64=%u\tcoverage=%.6f\n",
           kind, d->num_names, unique, collisions, long_names, fast64,
           d->num_names ? (double)fast64 / d->num_names : 0.0);
}

int main(void) {
    if (!libpostal_setup() || !libpostal_setup_parser()) return 2;
    address_parser_t *parser = get_address_parser();
    if (parser == NULL || parser->model_type != ADDRESS_PARSER_TYPE_CRF || parser->model.crf == NULL) return 3;
    crf_t *crf = parser->model.crf;
    inventory("state", (const feature_dict_t *)crf->state_dict);
    inventory("transition", (const feature_dict_t *)crf->trans_dict);
    libpostal_teardown_parser();
    libpostal_teardown();
    return 0;
}
