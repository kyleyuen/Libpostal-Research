# Command record

All model-loaded parser executions were invoked through `research/tools/model_slot.sh`; individual start/end timestamps and exits are in `commands.tsv`, `*.timing`, and `*.exit`.

```sh
cd research/experiments/175-direct-event-flow
EXTRA_SW="$(cat raw/switches.txt)" tools/build_cc.sh event clang none "-O2" "-O1" "-O2" /opt/homebrew/opt/mimalloc/lib/mimalloc.o
./tools/run_checks.sh
clang -O2 -I libpostal/src research/experiments/000-baseline/artifacts/parser_profile.c artifacts/event/libpostal.a -lm -o artifacts/event/parser_profile
```

The first build used the same build command and failed with exit 1 because the copied build script did not create `artifacts/scanner_cache`. Its stdout, stderr, timing and exit are retained as `build.*`. The experiment-local build script was amended to create that directory; the retry is `build_retry.*` and exited 0.

Memory samples each ran `model_slot.sh <parser_profile> profiling/golden-v1/development.txt <seconds>` with `/usr/bin/time -l`; commands were launched in the worker shell to capture a READY PID, followed by `ps -M -p PID` and, for the 5-second samples, `footprint -p PID`. Exact outputs, PIDs, timestamps and exits are retained in `memory_*` and `steady_*` files.
