#!/usr/bin/env bash
set -euo pipefail
W="$(cd "$(dirname "$0")" && pwd)"
for variant in original l4 l4_r2; do
  for target in libpostal-1.dll bench_ext_win_r1.exe dump_outputs_win.exe; do
    path="$W/artifacts/$variant/$target"
    file "$path"
    x86_64-w64-mingw32-objdump -f "$path" | sed -n '1,5p'
    x86_64-w64-mingw32-objdump -p "$path" | rg '^\s*DLL Name:' || true
  done
done
for target in libpostal-1.dll bench_ext_win_r3.exe dump_outputs_win_r3.exe; do
  path="$W/artifacts/l4_r3/$target"
  file "$path"
  x86_64-w64-mingw32-objdump -f "$path" | sed -n '1,5p'
  x86_64-w64-mingw32-objdump -p "$path" | rg '^\s*DLL Name:' || true
done
printf 'mimalloc override symbols:\n'
x86_64-w64-mingw32-nm -g "$W/mimalloc-build/mimalloc.obj" | rg ' (malloc|free|mi_malloc|mi_free)$' || true
