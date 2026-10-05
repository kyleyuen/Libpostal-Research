#!/bin/bash
# Q2 timing session: env gate (AC power, no Low Power Mode, no thermal warning), caffeinate -i, telemetry before/after, top sampler during.
# 2.5 GiB chase (5 trials x 2 variants x k=1..16, 20M loads per measurement), then an 8 MiB L2-resident control (3 trials).
E="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/experiments/014-math-quantities"; T="/Users/kyle/Desktop/HW/CSCI 201 Reserach/research/tools"
# wait (up to 100 min, checking every 60 s) for the gate to pass; every refusal is logged
for i in $(seq 1 100); do "$T/env_gate.sh" >> "$E/raw/q2_gate.txt" 2>&1 && break; echo "$(date '+%Y-%m-%dT%H:%M:%S%z') waiting for gate" >> "$E/raw/q2_gate.txt"; sleep 60; done
"$T/env_gate.sh" >> "$E/raw/q2_gate.txt" 2>&1 || { echo "gate never passed: not timing"; exit 1; }
"$T/env_snapshot.sh" "$E/raw/q2_env.txt" before
top -l 0 -s 5 -n 5 -o cpu -stats pid,command,cpu > "$E/raw/q2_top.txt" 2>&1 & TOP=$!
caffeinate -i "$E/tools/run.sh" q2_chase_2p5G "$E/artifacts/chase" 2684354560 20000000 5; rc1=$?
caffeinate -i "$E/tools/run.sh" q2_chase_8M "$E/artifacts/chase" 8388608 20000000 3; rc2=$?
kill $TOP
"$T/env_snapshot.sh" "$E/raw/q2_env.txt" after
"$T/env_gate.sh" >> "$E/raw/q2_gate.txt" 2>&1
echo "rc 2.5G=$rc1 8M=$rc2"
