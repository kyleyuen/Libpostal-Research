#!/bin/bash
# Appends a compact snapshot of everything that can change benchmark speed on this laptop: power source, battery, low-power mode, thermal state,
# swap, memory pressure, load average, system CPU usage and the busiest processes. bash 3.2 safe. usage: env_snapshot.sh FILE [label]
f=$1; label=${2:-snap}
{
  echo "== $label $(date -Iseconds)"
  pmset -g batt | head -2 | tr '\n' ' '; echo
  pmset -g | grep -E "^ *(lowpowermode|powermode|sleep|displaysleep) " | tr -s ' ' | tr '\n' ';'; echo
  pmset -g therm 2>&1 | grep -v "^$" | head -4 | tr '\n' ';'; echo
  sysctl -n vm.swapusage
  memory_pressure 2>/dev/null | grep -E "free percentage" 
  echo "loadavg $(sysctl -n vm.loadavg)"
  top -l 2 -s 1 -n 0 2>/dev/null | grep "CPU usage" | tail -1
  ps -Ao pcpu,comm -r | sed -n 2,7p | cut -c1-70
} >> "$f" 2>&1
