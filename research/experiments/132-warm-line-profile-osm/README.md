# Experiment 132 — warm line profile of cpi on OSM addresses (diagnostic)

Date/time/timezone: created 2026-10-04T03:04:32-0700 (`raw/created.txt`), America/Los_Angeles. Status: complete

xctrace CPU Profiler for 10 s on cpi cycling the first 300 OSM addresses. `tools/pc_profile2.py` resolves binary references from the whole export; the first loader attributed every sample outside the binary and was replaced. Output: `raw/line_profile.stdout` (warm 2.63 µs/address under the profiler).
- **Accumulation:** crf.c:707 (transition rows) 0.217 and :693 (state rows) 0.210 µs, about 1,100 scattered adds per address.
- **Viterbi:** crf_context.c:655/658 0.17 µs.
- **Library calls:** memset 0.173, memmove 0.157, strlen 0.084 µs.
- **Trie walks:** about 0.13 µs.
Remaining compute is spread over items of about 1% each, apart from accumulation, which is near the hardware limit.
