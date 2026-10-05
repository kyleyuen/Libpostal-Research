#!/bin/bash
# Hard gate for benchmark sessions: exit 1 (with reason) unless on AC power, not in Low Power Mode, and no thermal/performance warning is recorded.
# Load/memory conditions are NOT gated (the laptop is shared); they are recorded per run and used by the analysis instead.
# Pause switch (added 2026-09-30): while research/PAUSE_TIMING exists the gate fails, so a running session finishes its current run and waits.
# Delete the file to resume. (run_unattended.sh gives up after 2 h of waiting; completed runs are kept.)
for pf in "$(dirname "$0")"/../PAUSE_TIMING*; do [ -e "$pf" ] && { echo "GATE FAIL: paused ($(basename "$pf") present)"; exit 1; }; done   # any PAUSE_TIMING* file pauses (one per job)
batt=$(pmset -g batt | head -1)
case "$batt" in *"AC Power"*) ;; *) echo "GATE FAIL: not on AC power ($batt)"; exit 1;; esac
if pmset -g | grep -Eq "^ *lowpowermode +1"; then echo "GATE FAIL: Low Power Mode is on"; exit 1; fi
if pmset -g therm 2>&1 | grep -Eq "(CPU_Speed_Limit|Performance warning level|Thermal warning level) *[=:] *[1-9]"; then echo "GATE FAIL: thermal/performance warning active"; exit 1; fi
# Added 2026-10-03 (after 077's contaminated screen): fail while this project's builds, checks or another timing run are active.
for pn in clang ld llvm-profdata dump_outputs fuzz_pipeline bench_ext; do pgrep -x "$pn" > /dev/null && { echo "GATE FAIL: concurrent project process running ($pn)"; exit 1; }; done
echo "GATE OK: $batt"
