#!/bin/bash
# 161: link api_probe2 against an ASan libpostal.a (Homebrew clang 23; Apple clang 17's ASan runtime hangs before main on macOS 26.5.2)
# and run the given modes, one model process at a time through model_slot.sh. usage: asan_run.sh VARIANT OUTNAME MODE...  (env CP_IDS_DISABLE passes through)
W="/Users/kyle/Desktop/HW/CSCI 201 Reserach"; E="$W/research/experiments/161-resetup-defect"; v=$1; o="$E/raw/$2"; shift 2; mkdir -p "$o"
CC=/opt/homebrew/opt/llvm/bin/clang; D=/Users/kyle/.cache/libpostal-profile/libpostal; b="$E/artifacts/$v/api_probe2"
[ -x "$b" ] || ( cd "$W" && $CC -O1 -g -fsanitize=address -Ilibpostal/src "$E/tools/api_probe2.c" "$E/artifacts/$v/libpostal.a" -lm -o "$b" ) >> "$o/link.txt" 2>&1
export ASAN_OPTIONS=detect_leaks=0:quarantine_size_mb=32
for m in "$@"; do
  echo "# $(date "+%Y-%m-%dT%H:%M:%S%z") ASAN_OPTIONS=$ASAN_OPTIONS CP_IDS_DISABLE=${CP_IDS_DISABLE-} $v/api_probe2 $m $D" >> "$o/commands.txt"
  "$W/research/tools/model_slot.sh" "$b" "$m" "$D" > "$o/$m.stdout" 2> "$o/$m.stderr"; rc=$?
  r=$(grep -c "ERROR: AddressSanitizer" "$o/$m.stderr"); s=$(cmp -s "$o/$m.stdout" "$W/research/experiments/158-r0-feature-print/raw/api/original-$m.stdout" && echo equal || echo DIFFERENT)
  echo "#   exit=$rc asan_reports=$r stdout_vs_original=$s $(date "+%Y-%m-%dT%H:%M:%S%z")" >> "$o/commands.txt"
done
echo END >> "$o/commands.txt"
