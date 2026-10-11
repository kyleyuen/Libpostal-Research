# Short project summary draft — final metrics pending 202

- **Goal:** Make libpostal faster on one thread while preserving ordered labels and components and staying within the original peak and steady memory.
- **How:** CPI combines model-based lookups, per-address reuse, less string handling, and a decoder tuned for 20 labels. Vector instructions run within the same thread; the package uses Clang profile-guided optimization, link-time optimization, and mimalloc.
- **Result:** [Insert speed, spread, peak and steady footprint from the completed 202 report. Compare roles measured in that same run.]
- **Correctness:** In 202, complete output files matched byte-for-byte for all 13 existing datasets (128,540 addresses). Fuzz and API checks have separate coverage limits.
- **What did not transfer:** Experiment 201 measured fewer token payload copies with views, but its bounded speed comparison was inconclusive and it was not promoted over CPI.
- **Limits and details:** These checks cover saved datasets and API cases, not every possible address or public API state. The [detailed report outline](DETAIL_REPORT_OUTLINE.md) lists the evidence and limits to include.
