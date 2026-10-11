#ifndef STREAM_POSTCODE_GRAPH_H
#define STREAM_POSTCODE_GRAPH_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "graph.h"
#include "file_utils.h"

typedef struct {
    uint8_t *bytes;
    uint32_t *offsets;
    size_t rows;
    uint32_t m;
    uint32_t n;
    uint64_t indices_count;
    uint64_t encoded_bytes;
    bool compressed;
} stream_postcode_result_t;

#define STREAM_POSTCODE_SCRATCH_VALUES 65536

static bool stream_postcode_read_indices(FILE *f, uint32_t *scratch, size_t count) {
    unsigned char *raw = (unsigned char *)scratch;
    if (fread(raw, sizeof(uint32_t), count, f) != count) return false;
    for (size_t i = 0, offset = 0; i < count; i++, offset += sizeof(uint32_t)) {
        scratch[i] = file_deserialize_uint32(raw + offset);
    }
    return true;
}

static bool stream_postcode_discard(FILE *f, uint64_t count, uint32_t *scratch) {
    while (count > 0) {
        size_t n = count > STREAM_POSTCODE_SCRATCH_VALUES
            ? STREAM_POSTCODE_SCRATCH_VALUES : (size_t)count;
        if (!stream_postcode_read_indices(f, scratch, n)) return false;
        count -= n;
    }
    return true;
}

static bool stream_postcode_decode_one(const uint8_t *bytes, uint64_t start,
                                      uint64_t end, uint32_t *value) {
    uint32_t gap = 0;
    unsigned shift = 0;
    while (start < end && shift < 35) {
        uint8_t byte = bytes[start++];
        uint32_t part = byte & 0x7f;
        if (shift == 28 && part > 0x0f) return false;
        gap |= part << shift;
        if ((byte & 0x80) == 0) {
            *value = gap;
            return true;
        }
        shift += 7;
    }
    return false;
}

static bool stream_postcode_process_indices(FILE *f, const uint32_t *indptr,
                                             size_t rows, uint64_t index_count,
                                             uint8_t *bytes, uint32_t *offsets,
                                             uint64_t *encoded_size) {
    uint32_t scratch[STREAM_POSTCODE_SCRATCH_VALUES];
    uint64_t written = 0;
    if (!stream_postcode_discard(f, indptr[0], scratch)) return false;

    for (size_t row = 0; row < rows; row++) {
        uint32_t start = indptr[row];
        uint32_t end = indptr[row + 1];
        uint64_t remaining = (uint64_t)end - start;
        uint32_t previous = 0;
        bool have_previous = false;

        if (offsets != NULL) {
            if (written >= UINT32_MAX) return false;
            offsets[row] = (uint32_t)written;
        }

        while (remaining > 0) {
            size_t n = remaining > STREAM_POSTCODE_SCRATCH_VALUES
                ? STREAM_POSTCODE_SCRATCH_VALUES : (size_t)remaining;
            if (!stream_postcode_read_indices(f, scratch, n)) return false;

            for (size_t i = 0; i < n; i++) {
                uint32_t value = scratch[i];
                if (have_previous && value < previous) return false;
                uint32_t gap = value - previous;
                uint64_t value_start = written;
                do {
                    uint8_t low = (uint8_t)(gap & 0x7f);
                    gap >>= 7;
                    if (written >= UINT32_MAX) return false;
                    if (bytes != NULL) {
                        bytes[written] = (uint8_t)(low | (gap != 0 ? 0x80 : 0));
                    }
                    written++;
                } while (gap != 0);

                if (bytes != NULL) {
                    uint32_t decoded;
                    if (!stream_postcode_decode_one(bytes, value_start, written, &decoded) ||
                        decoded != value - previous) return false;
                }
                previous = value;
                have_previous = true;
            }
            remaining -= n;
        }
    }

    if (offsets != NULL) {
        if (written >= UINT32_MAX) return false;
        offsets[rows] = (uint32_t)written;
    }

    if (!stream_postcode_discard(f, index_count - indptr[rows], scratch)) return false;
    *encoded_size = written;
    return true;
}

static graph_t *stream_postcode_fallback(FILE *f, const fpos_t *graph_start,
                                         graph_t *partial,
                                         stream_postcode_result_t *result) {
    if (partial != NULL) graph_destroy(partial);
    free(result->bytes);
    free(result->offsets);
    result->bytes = NULL;
    result->offsets = NULL;
    result->rows = 0;
    result->m = 0;
    result->n = 0;
    result->indices_count = 0;
    result->encoded_bytes = 0;
    result->compressed = false;
    clearerr(f);
    if (fsetpos(f, graph_start) != 0) return NULL;
    return graph_read(f);
}

/*
 * Reads the graph following the postcode trie. On success, the returned graph
 * retains only metadata (its arrays are NULL) and result owns compressed bytes
 * and row offsets. Unsupported/corrupt-to-compress input rewinds and delegates
 * to graph_read(), leaving result->compressed false for the caller's old path.
 */
static graph_t *stream_postcode_graph_read(FILE *f, stream_postcode_result_t *result) {
    fpos_t graph_start, indices_start;
    graph_t *g = NULL;
    uint64_t len_indptr, len_indices, total = 0, encoded_again = 0;
    uint8_t fixed_rows;

    if (f == NULL || result == NULL) return NULL;
    result->bytes = NULL;
    result->offsets = NULL;
    result->rows = 0;
    result->m = 0;
    result->n = 0;
    result->indices_count = 0;
    result->encoded_bytes = 0;
    result->compressed = false;

    if (fgetpos(f, &graph_start) != 0) return graph_read(f);

    g = (graph_t *)calloc(1, sizeof(*g));
    if (g == NULL) return stream_postcode_fallback(f, &graph_start, NULL, result);

    if (!file_read_uint32(f, &g->m) || !file_read_uint32(f, &g->n) ||
        !file_read_uint8(f, &fixed_rows) || !file_read_uint64(f, &len_indptr) ||
        len_indptr < 1 || len_indptr > UINT32_MAX ||
        len_indptr > SIZE_MAX / sizeof(uint32_t)) {
        return stream_postcode_fallback(f, &graph_start, g, result);
    }
    g->fixed_rows = fixed_rows != 0;
    g->indptr = uint32_array_new_size((size_t)len_indptr);
    if (g->indptr == NULL ||
        !file_read_uint32_array(f, g->indptr->a, (size_t)len_indptr)) {
        return stream_postcode_fallback(f, &graph_start, g, result);
    }
    g->indptr->n = (size_t)len_indptr;
    g->indices = NULL;

    if (!file_read_uint64(f, &len_indices) || len_indices > UINT32_MAX ||
        fgetpos(f, &indices_start) != 0) {
        return stream_postcode_fallback(f, &graph_start, g, result);
    }

    size_t rows = (size_t)len_indptr - 1;
    const uint32_t *ip = g->indptr->a;
    for (size_t row = 0; row < rows; row++) {
        if (ip[row + 1] < ip[row] || ip[row + 1] > len_indices) {
            return stream_postcode_fallback(f, &graph_start, g, result);
        }
    }
    if (ip[0] > len_indices || ip[rows] > len_indices ||
        (rows + 1) > SIZE_MAX / sizeof(uint32_t)) {
        return stream_postcode_fallback(f, &graph_start, g, result);
    }

    if (!stream_postcode_process_indices(f, ip, rows, len_indices, NULL, NULL, &total) ||
        total >= UINT32_MAX || fsetpos(f, &indices_start) != 0) {
        return stream_postcode_fallback(f, &graph_start, g, result);
    }

    result->bytes = (uint8_t *)malloc(total == 0 ? 1 : (size_t)total);
    result->offsets = (uint32_t *)malloc((rows + 1) * sizeof(*result->offsets));
    if (result->bytes == NULL || result->offsets == NULL ||
        !stream_postcode_process_indices(f, ip, rows, len_indices,
                                         result->bytes, result->offsets,
                                         &encoded_again) ||
        encoded_again != total) {
        return stream_postcode_fallback(f, &graph_start, g, result);
    }

    result->rows = rows;
    result->m = g->m;
    result->n = g->n;
    result->indices_count = len_indices;
    result->encoded_bytes = encoded_again;
    result->compressed = true;
    uint32_array_destroy(g->indptr);
    g->indptr = NULL;
    return g;
}

#endif
