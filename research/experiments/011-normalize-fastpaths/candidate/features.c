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
