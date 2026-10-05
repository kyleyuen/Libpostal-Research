#include "features.h"


#ifdef EXP_FAST_JOIN
// Same bytes as char_array_add_vjoined(features->str, "|", strip_separator=true, ...): every argument but the last loses ONE trailing '|' if it has one and is followed by
// a '|'; the last argument is appended whole; the string is NUL-terminated. Sizes are computed first, capacity is reserved once (same doubling policy as char_array_push)
// and short pieces are copied inline instead of through libc calls.
static inline void fj_copy(char *dst, const char *src, size_t n) {
    if (n >= 16) { memcpy(dst, src, n); }
    else if (n >= 8) { uint64_t a, b; memcpy(&a, src, 8); memcpy(&b, src + n - 8, 8); memcpy(dst, &a, 8); memcpy(dst + n - 8, &b, 8); }
    else if (n >= 4) { uint32_t a, b; memcpy(&a, src, 4); memcpy(&b, src + n - 4, 4); memcpy(dst, &a, 4); memcpy(dst + n - 4, &b, 4); }
    else if (n >= 2) { uint16_t a, b; memcpy(&a, src, 2); memcpy(&b, src + n - 2, 2); memcpy(dst, &a, 2); memcpy(dst + n - 2, &b, 2); }
    else if (n == 1) { *dst = *src; }
}

#ifdef EXP_NAME_LEN
// Experiment 019: identical to feature_array_add (EXP_FAST_JOIN path) except that the first part's length is supplied by the caller (the macro in
// address_parser.c passes sizeof(literal) - 1 for string literals and strlen() otherwise). Same bytes, same strip rule, same growth policy.
void feature_array_add_named(cstring_array *features, size_t count, const char *name, size_t name_len, ...) {
    if (count <= 0) return;
    va_list args;
    va_start(args, name_len);
    const char *part[8]; size_t len[8]; size_t total = 0;
    if (count > 8) {   // never used by the parser; keep the generic path
        va_end(args);
        fprintf(stderr, "feature_array_add_named: count > 8 unsupported\n"); abort();
    }
    for (size_t i = 0; i < count; i++) {
        part[i] = i == 0 ? name : va_arg(args, char *);
        size_t l = i == 0 ? name_len : strlen(part[i]);
        if (i + 1 < count) {
            if (l > 0 && part[i][l - 1] == FEATURE_SEPARATOR_CHAR[0]) l--;
            total += l + 1;
        } else {
            total += l;
        }
        len[i] = l;
    }
    va_end(args);
    cstring_array_start_token(features);
    char_array *s = features->str;
    size_t need = s->n + total + 1;
    if (need > s->m) {
        size_t size = s->m ? s->m : 2;
        while (size < need) size <<= 1;
        char *ptr = realloc(s->a, size);
        if (ptr == NULL) { fprintf(stderr, "realloc failed during char_array_push\n"); exit(EXIT_FAILURE); }
        s->a = ptr; s->m = size;
    }
    char *p = s->a + s->n;
    for (size_t i = 0; i < count; i++) {
        fj_copy(p, part[i], len[i]);
        p += len[i];
        if (i + 1 < count) *p++ = FEATURE_SEPARATOR_CHAR[0];
    }
    *p = '\0';
    s->n += total + 1;
}
#endif

#ifdef EXP_NAME_BYTE
// Experiment 059. When a feature is appended to the state or transition key array of the scoring pass and its name is a string literal known to
// that array's dictionary, the key is written as [0x01][name id | 0x80 if value parts follow][value parts joined exactly as before] instead of
// [name][|][value parts]. 0x01 cannot start a text key (names are text), and the dictionary's own split of the text form yields exactly this
// (name id, flag, rest), so the lookup result is identical. The name id per call site and dictionary is a model constant, cached once.
#include "feature_dict.h"
#include "fa_nb.h"
const void *fa_nb_dict[2] = {NULL, NULL}; const void *fa_nb_arr[2] = {NULL, NULL};
void feature_array_add_nb(cstring_array *features, size_t count, const char *name, size_t name_len, fa_nb_cache_t *cache, ...) {
    if (count <= 0) return;
    if (count > 8) { fprintf(stderr, "feature_array_add_nb: count > 8 unsupported\n"); abort(); }
    int w = (cache == NULL) ? -1 : (const void *)features == fa_nb_arr[0] ? 0 : (const void *)features == fa_nb_arr[1] ? 1 : -1;
    int nb = -1;
    if (w >= 0 && fa_nb_dict[w] != NULL && name_len > 0 && name[name_len - 1] != FEATURE_SEPARATOR_CHAR[0]) {
        if (cache->d[w] != fa_nb_dict[w]) { cache->d[w] = fa_nb_dict[w]; cache->nb[w] = (int16_t)fd_find_name((const feature_dict_t *)fa_nb_dict[w], name, name_len); }
        nb = cache->nb[w];
    }
    va_list args; va_start(args, cache);
    const char *part[8]; size_t len[8]; size_t total = 0;
    part[0] = name; len[0] = name_len;
    for (size_t i = 1; i < count; i++) { part[i] = va_arg(args, char *); len[i] = strlen(part[i]); }
    va_end(args);
    for (size_t i = 0; i < count; i++) {               // the original strip rule: a non-last part loses one trailing separator
        if (i + 1 < count && len[i] > 0 && part[i][len[i] - 1] == FEATURE_SEPARATOR_CHAR[0]) len[i]--;
    }
    size_t first = nb >= 0 ? 1 : 0;
    total = nb >= 0 ? 2 : 0;
    for (size_t i = first; i < count; i++) total += len[i] + (i + 1 < count ? 1 : 0);
    cstring_array_start_token(features);
    char_array *s = features->str;
    size_t need = s->n + total + 1;
    if (need > s->m) {
        size_t size = s->m ? s->m : 2;
        while (size < need) size <<= 1;
        char *ptr = realloc(s->a, size);
        if (ptr == NULL) { fprintf(stderr, "realloc failed during char_array_push\n"); exit(EXIT_FAILURE); }
        s->a = ptr; s->m = size;
    }
    char *p = s->a + s->n;
    if (nb >= 0) { *p++ = 0x01; *p++ = (char)(uint8_t)(nb | (count >= 2 ? 0x80 : 0)); }
    for (size_t i = first; i < count; i++) {
        fj_copy(p, part[i], len[i]);
        p += len[i];
        if (i + 1 < count) *p++ = FEATURE_SEPARATOR_CHAR[0];
    }
    *p = '\0';
    s->n += total + 1;
}
#endif
#ifdef EXP_FA_LEN
// Experiment 067: feature_array_add_nb with the value lengths supplied by the caller (same bytes; same strip rule for non-last parts).
void feature_array_add_nbl(cstring_array *features, size_t count, const char *name, size_t name_len, fa_nb_cache_t *cache, const char *v1, size_t l1, const char *v2, size_t l2) {
    if (count < 1 || count > 3) { fprintf(stderr, "feature_array_add_nbl: count %zu unsupported\n", count); abort(); }
    int w = (cache == NULL) ? -1 : (const void *)features == fa_nb_arr[0] ? 0 : (const void *)features == fa_nb_arr[1] ? 1 : -1;
    int nb = -1;
    if (w >= 0 && fa_nb_dict[w] != NULL && name_len > 0 && name[name_len - 1] != FEATURE_SEPARATOR_CHAR[0]) {
        if (cache->d[w] != fa_nb_dict[w]) { cache->d[w] = fa_nb_dict[w]; cache->nb[w] = (int16_t)fd_find_name((const feature_dict_t *)fa_nb_dict[w], name, name_len); }
        nb = cache->nb[w];
    }
    const char *part[3] = {name, v1, v2}; size_t len[3] = {name_len, l1, l2};
    for (size_t i = 0; i < count; i++) if (i + 1 < count && len[i] > 0 && part[i][len[i] - 1] == FEATURE_SEPARATOR_CHAR[0]) len[i]--;
    size_t first = nb >= 0 ? 1 : 0, total = nb >= 0 ? 2 : 0;
    for (size_t i = first; i < count; i++) total += len[i] + (i + 1 < count ? 1 : 0);
    cstring_array_start_token(features);
    char_array *s = features->str;
    size_t need = s->n + total + 1;
    if (need > s->m) {
        size_t size = s->m ? s->m : 2;
        while (size < need) size <<= 1;
        char *ptr = realloc(s->a, size);
        if (ptr == NULL) { fprintf(stderr, "realloc failed during char_array_push\n"); exit(EXIT_FAILURE); }
        s->a = ptr; s->m = size;
    }
    char *p = s->a + s->n;
    if (nb >= 0) { *p++ = 0x01; *p++ = (char)(uint8_t)(nb | (count >= 2 ? 0x80 : 0)); }
    for (size_t i = first; i < count; i++) { fj_copy(p, part[i], len[i]); p += len[i]; if (i + 1 < count) *p++ = FEATURE_SEPARATOR_CHAR[0]; }
    *p = '\0';
    s->n += total + 1;
}
#endif
void feature_array_add(cstring_array *features, size_t count, ...) {
    if (count <= 0) {
        return;
    }

    va_list args;
    va_start(args, count);

    if (count > 8) {
        cstring_array_start_token(features);
        char_array_add_vjoined(features->str, FEATURE_SEPARATOR_CHAR, true, count, args);
        va_end(args);
        return;
    }

    const char *part[8];
    size_t len[8];
    size_t total = 0;
    for (size_t i = 0; i < count; i++) {
        part[i] = va_arg(args, char *);
        size_t l = strlen(part[i]);
        if (i + 1 < count) {
            if (l > 0 && part[i][l - 1] == FEATURE_SEPARATOR_CHAR[0]) l--;
            total += l + 1;
        } else {
            total += l;
        }
        len[i] = l;
    }
    va_end(args);

    cstring_array_start_token(features);
    char_array *s = features->str;
    size_t need = s->n + total + 1;
    if (need > s->m) {
        size_t size = s->m ? s->m : 2;
        while (size < need) size <<= 1;
        char *ptr = realloc(s->a, size);
        if (ptr == NULL) {
            fprintf(stderr, "realloc failed during char_array_push\n");
            exit(EXIT_FAILURE);
        }
        s->a = ptr;
        s->m = size;
    }
    char *p = s->a + s->n;
    for (size_t i = 0; i < count; i++) {
        fj_copy(p, part[i], len[i]);
        p += len[i];
        if (i + 1 < count) *p++ = FEATURE_SEPARATOR_CHAR[0];
    }
    *p = '\0';
    s->n += total + 1;
}
#else
void feature_array_add(cstring_array *features, size_t count, ...) {
    if (count <= 0) {
        return;        
    }

    va_list args;
    va_start(args, count);

    cstring_array_start_token(features);

    bool strip_separator = true;
    char_array_add_vjoined(features->str, FEATURE_SEPARATOR_CHAR, strip_separator, count, args);
    va_end(args);
}


#endif


void feature_array_add_printf(cstring_array *features, char *format, ...) {
    va_list args;
    va_start(args, format);
    cstring_array_start_token(features);
    char_array_cat_vprintf(features->str, format, args);
    va_end(args);
}


bool feature_counts_update_or_add(khash_t(str_double) *features, char *feature, double count, bool copy, bool add) {
    khiter_t k;

    k = kh_get(str_double, features, feature);
    if (k == kh_end(features)) {
        int ret;
        char *str;
        if (copy) {
            str = strdup(feature);
        } else {
            str = feature;
        }

        k = kh_put(str_double, features, str, &ret);
        if (ret < 0) return false;

        kh_value(features, k) = count;
    } else if (add) {
        kh_value(features, k) += count;
    } else {
        kh_value(features, k) = count;
    }
    return true;
}


inline bool feature_counts_add(khash_t(str_double) *features, char *feature, double count) {
    return feature_counts_update_or_add(features, feature, count, true, true);
}

inline bool feature_counts_add_no_copy(khash_t(str_double) *features, char *feature, double count) {
    return feature_counts_update_or_add(features, feature, count, false, true);
}

inline bool feature_counts_update(khash_t(str_double) *features, char *feature, double count) {
    return feature_counts_update_or_add(features, feature, count, true, false);
}

inline bool feature_counts_update_no_copy(khash_t(str_double) *features, char *feature, double count) {
    return feature_counts_update_or_add(features, feature, count, false, false);
}
