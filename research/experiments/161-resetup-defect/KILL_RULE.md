# Kill rule — pre-registered 2026-10-05T19:15:24-0700 by the lead, before any build (not in REDESIGN_PLAN; added because 158 reproduced the defect)

**Goal.** cpi (119 sources + 158's print fix) survives any number of teardown → re-setup cycles in one process, with behaviour equal to the original's.

**Pass requires all of these:**
1. 158's api_probe2 modes lifecycle, lifecycle_long and datadir_lifecycle exit 0, with stdout equal to the original's:
   - under the system-malloc link with MallocGuardEdges=1 MallocScribble=1;
   - and under an ASan build, with 0 ASan reports;
   - and under the default mimalloc link.
2. Every global or static state reached at setup is listed, by source reading, and is reset or freed at teardown. That includes cp_pool_n/m, cp_ready, crf_lean_cap_items, crf_ik_active and crf_role_ids. List with file:line.
3. Parse output is byte-identical on the full brief suite, and one thread.
4. Timing is unchanged within noise against 158fp: same session, decided by the lead.

**Kill.** Any output difference, any failure in item 1 that cannot be fixed without touching the hot parse path, or a measured slowdown beyond noise.
