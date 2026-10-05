#!/bin/bash
# Records each calibration workload, exports the counter table, then deletes the raw trace (disk space); abort if < 1 GiB free.
cd "$(dirname "$0")/../raw" && mkdir -p calib
for m in chase alu branch; do
  free=$(df -k /System/Volumes/Data | tail -1 | awk '{print $4}'); [ "$free" -lt 1048576 ] && { echo "abort: low disk ($free KB) before $m" >> calib/ABORT; exit 1; }
  date -Iseconds > calib/$m.start
  xctrace record --quiet --template 'CPU Counters' --output calib/$m.trace --time-limit 24s --launch -- ../tools/counter_calib $m 40 > calib/$m.log 2>&1; echo "exit=$?" >> calib/$m.log
  xctrace export --input calib/$m.trace --xpath '/trace-toc/run[@number="1"]/data/table[@schema="CounterMetricAggregatedForProcess"]' > calib/$m.xml 2> calib/$m.err
  [ -s calib/$m.xml ] && rm -rf calib/$m.trace && echo "trace deleted after export (disk space)" >> calib/$m.log
  date -Iseconds > calib/$m.end
done
python3 ../tools/counter_table.py 12 22 calib/chase.xml calib/alu.xml calib/branch.xml > calib-summary.txt 2>&1
echo done > calib/DONE
