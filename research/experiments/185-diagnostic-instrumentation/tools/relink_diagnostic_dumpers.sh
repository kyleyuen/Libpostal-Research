#!/bin/bash
# Copy an audited six-build set and relink only its three diagnostic dumpers.
set -eo pipefail

ROOT="/Users/kyle/Desktop/HW/CSCI 201 Reserach"
EXP="$ROOT/research/experiments/185-diagnostic-instrumentation"
OLD_ID="${1:?source build id required}"
NEW_ID="${2:?new build id required}"
OLD_ART="$EXP/artifacts/build-$OLD_ID"
OLD_RAW="$EXP/raw/build-$OLD_ID"
NEW_ART="$EXP/artifacts/build-$NEW_ID"
NEW_RAW="$EXP/raw/build-$NEW_ID"
CLANG="$(command -v clang)"
VARIANTS=(original cpi prototype181)

[[ -d "$OLD_ART" && -d "$OLD_RAW" ]] || { echo 'Source build set missing' >&2; exit 2; }
[[ ! -e "$NEW_ART" && ! -e "$NEW_RAW" ]] || { echo 'New build ID already exists' >&2; exit 2; }
mkdir -p "$NEW_ART" "$NEW_RAW/commands" "$NEW_RAW/provenance"
cp -R "$OLD_ART/." "$NEW_ART/"
printf 'build_id=%s\nparent_build_id=%s\nstart=%s\ncommand=%q\nclang=%s\n' \
  "$NEW_ID" "$OLD_ID" "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$0 $*" "$CLANG" > "$NEW_RAW/session.txt"
finish() {
  local status=$?
  trap - EXIT
  printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$NEW_RAW/session.txt"
  exit "$status"
}
trap finish EXIT

(cd "$EXP" && find source -type f -print0 | sort -z | xargs -0 shasum -a 256; \
  shasum -a 256 source/diag.c diag-schema.json tools/dump_outputs.c \
  tools/build_variants.sh tools/relink_diagnostic_dumpers.sh) > "$NEW_RAW/source.sha256"
cp "$OLD_RAW/provenance/"*.txt "$NEW_RAW/"

index=0
for variant in "${VARIANTS[@]}"; do
  index=$((index + 1))
  log="$NEW_RAW/commands/$variant"
  mkdir -p "$log"
  stem="$log/$(printf '%04d' "$index")-link-diagnostic-dumper"
  source_root="$EXP/source/$variant"
  command=("$CLANG" -O2 -g -DEXP185_DIAG -I"$source_root" -I"$source_root/.."
    "$EXP/tools/dump_outputs.c" "$NEW_ART/$variant/diagnostic/libpostal.a" -lm
    -o "$NEW_ART/$variant/diagnostic/dump_outputs")
  {
    printf 'start=%s\ncwd=%q\nargv=' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$ROOT"
    printf '%q ' "${command[@]}"
    printf '\n'
  } > "$stem.command"
  status=0
  (cd "$ROOT" && "${command[@]}" > "$stem.stdout" 2> "$stem.stderr") || status=$?
  printf 'end=%s\nexit=%s\n' "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$status" >> "$stem.command"
  printf '%s %s\n' "$stem" "$status" >> "$log/exit_codes.txt"
  [[ "$status" -eq 0 ]] || exit "$status"
  shasum -a 256 "$NEW_ART/$variant/diagnostic/libpostal.a" \
    "$NEW_ART/$variant/diagnostic/dump_outputs" > "$stem.artifacts.sha256"
done

for variant in "${VARIANTS[@]}"; do
  {
    case "$variant" in original) link_number=0001 ;; cpi) link_number=0002 ;; prototype181) link_number=0003 ;; esac
    printf 'build_id=%s\nparent_build_id=%s\nvariant=%s\n' "$NEW_ID" "$OLD_ID" "$variant"
    printf 'parent_provenance_sha256='; shasum -a 256 "$OLD_RAW/provenance/$variant.txt"
    printf '\nsource_and_linker_sha256:\n'; cat "$NEW_RAW/source.sha256"
    printf '\nrelease_artifacts_sha256:\n'
    shasum -a 256 "$NEW_ART/$variant/release/libpostal.a" "$NEW_ART/$variant/release/dump_outputs"
    printf '\ndiagnostic_artifacts_sha256:\n'
    shasum -a 256 "$NEW_ART/$variant/diagnostic/libpostal.a" "$NEW_ART/$variant/diagnostic/dump_outputs"
    printf '\ndiagnostic_link_command:\n'
    cat "$NEW_RAW/commands/$variant/$link_number-link-diagnostic-dumper.command"
  } > "$NEW_RAW/provenance/$variant.txt"
done

printf 'build_id\tvariant\tmode\tlibrary\tdump\tprovenance\n' > "$NEW_RAW/outputs.tsv"
for variant in "${VARIANTS[@]}"; do
  for mode in release diagnostic; do
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$NEW_ID" "$variant" "$mode" \
      "$NEW_ART/$variant/$mode/libpostal.a" "$NEW_ART/$variant/$mode/dump_outputs" \
      "$NEW_RAW/provenance/$variant.txt" >> "$NEW_RAW/outputs.tsv"
  done
done
printf 'artifacts=%s\noutputs=%s\n' "$NEW_ART" "$NEW_RAW/outputs.tsv" >> "$NEW_RAW/session.txt"
printf 'Build artifacts: %s\nBuild records: %s\n' "$NEW_ART" "$NEW_RAW"
