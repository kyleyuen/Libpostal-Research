# Build attempts (all under research/tools/run_locked.py)

1. `build_release.sh`; exit 1: candidate API undeclared in address_parser.c because explicit trie.h include was missing. Raw stdout/stderr and timestamps in `build_release.*`.
2. Added include after address_parser.h; exit 1: address_parser.h had already pulled in libpostal/src/trie.h under the include guard, so declaration remained hidden. Raw `build_release_attempt2.*`.
3. Included candidate trie.h before address_parser.h; exit 1: same include-order issue in crf.c. Raw `build_release_attempt3.*`.
4. Included candidate trie.h before crf.h; library/tools built, then final hash list failed because the selfcheck binary was not yet linked. Raw `build_release_attempt4.*`.
5. Added selfcheck compile/run, but omitted HAVE_CONFIG_H; compile exited before helper creation. Raw `build_release_attempt5.*`.
6. Added HAVE_CONFIG_H, but helper compile lacked `-Ilibpostal` for config.h; exited before helper creation. Raw `build_release_attempt6.*`.
7. Added the missing config include path. Release build, synthetic length-lookup selfcheck, and artifact hashing all exited 0. Raw `build_release_attempt7.*`.
