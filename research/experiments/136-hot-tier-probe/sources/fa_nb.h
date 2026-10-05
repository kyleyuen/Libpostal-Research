// Experiment 059: per-call-site cache of a literal feature name's id in the state (0) and transition (1) dictionaries.
#ifndef FA_NB_H
#define FA_NB_H
#include <stdint.h>
#include <stddef.h>
typedef struct { const void *d[2]; int16_t nb[2]; } fa_nb_cache_t;
extern const void *fa_nb_dict[2];   // dictionaries of the current scoring pass (set by crf.c)
extern const void *fa_nb_arr[2];    // the key arrays they serve
#endif
