// Experiment 045 (measurement only): per-address record of model memory touched, by structure.
#ifndef TRACE_TOUCH_H
#define TRACE_TOUCH_H
#ifdef EXP_TRACE
#include <stddef.h>
enum { TS_FSLOT, TS_FARENA, TS_ISLOT, TS_INDPTR, TS_SROW, TS_TROW, TS_ROLE, TS_VSLOT, TS_VARENA, TS_TRIE_COMP, TS_TRIE_POST, TS_TRIE_OTHER, TS_PCCTX, TS_EXPV, TS_EXPA, TS_PTYPES, TS_N };
void trace_touch(int s, const void *p, size_t len);
int trace_trie_id(const void *trie);
extern const void *trace_vocab;
#define TT(s, p, len) trace_touch((s), (const void *)(p), (size_t)(len))
#else
#define TT(s, p, len) ((void)0)
#endif
#endif
