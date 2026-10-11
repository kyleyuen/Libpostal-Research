#!/usr/bin/env python3
"""Render the complete 185 operation-diagnostics result set as Markdown."""
import argparse
import json
from pathlib import Path


def md(value):
    return str(value).replace("|", "\\|").replace("\n", " ")


def show(value):
    return "N/A" if value is None else value


def metric(record, name, rate=None):
    item = record["metrics"][name]
    value = item.get("value")
    if value is None:
        return "N/A"
    if rate == "address":
        value /= record["addresses"]
    elif rate == "token":
        tokens = record["metrics"].get("_tokens")
        if not tokens:
            return "N/A"
        value /= tokens
    return f"{value:.2f}" if isinstance(value, float) else str(value)


def table(headers, rows):
    return ["| " + " | ".join(headers) + " |",
            "| " + " | ".join("---" for _ in headers) + " |",
            *["| " + " | ".join(md(cell) for cell in row) + " |" for row in rows], ""]


def render(results_path):
    results_path = Path(results_path).resolve()
    data = json.loads(results_path.read_text())
    config = json.loads((results_path.parent / "config.json").read_text())
    datasets = [Path(item["txt"]).stem if isinstance(item, dict) else str(item)
                for item in config.get("datasets", [])]
    variants = [item["name"] for item in config.get("variants", [])]
    expected = {(dataset, variant) for dataset in datasets for variant in variants}
    records = {(row["dataset"], row["variant"]): row for row in data.get("records", [])}
    if data.get("status") != "diagnostics collected; final acceptance not evaluated":
        raise ValueError("results are not complete: " + str(data.get("status")))
    if set(records) != expected:
        raise ValueError(f"expected {len(expected)} dataset/variant records, found {len(records)}")

    out = ["# Experiment 185 — operation diagnostics", "",
           f"Run: `{results_path.parent.name}`  ",
           f"Build set: `{config.get('build_id', 'unspecified')}`  ",
           f"Status: {data['status']}  ",
           f"Window: {data.get('start', 'unknown')} to {data.get('end', 'unknown')}", "",
           "This report describes operation counts, not speed. Each corpus is reported separately. Process durations are provenance only; diagnostic instrumentation adds overhead.", "",
           "## Per-corpus counters", "",
           "Counts are shown as total plus per-address and per-token rates where applicable. `N/A` means the runner marked a metric unavailable; it is never treated as zero.", ""]
    columns = ["Variant", "Addresses", "Input bytes", "Normalized bytes", "Tokens",
               "Lookup calls", "Calls/address", "Calls/token",
               "Hits", "Misses", "Repeats", "Answers reused", "Feature keys", "Key bytes",
               "Direct IDs offered", "Direct IDs consumed", "Direct ID fallbacks",
               "Rows", "Weights", "Alloc calls", "Alloc bytes", "Filter checks", "Searches avoided",
               "Output bytes", "Distinct repeat groups", "Repeat observations",
               "Retained examples", "Omitted groups", "Overflow addresses"]
    for dataset in datasets:
        out.extend([f"### {dataset}", ""])
        rows = []
        for variant in variants:
            r = records[(dataset, variant)]
            summary = r["metrics"].get("_repeated_patterns_summary") or {}
            token_count = r["metrics"].get("_tokens")
            rows.append([variant, r["addresses"], metric(r, "input_bytes"),
                         metric(r, "normalized_bytes"), token_count if token_count is not None else "N/A",
                         metric(r, "lookup_calls"), metric(r, "lookup_calls", "address"),
                         metric(r, "lookup_calls", "token"), metric(r, "lookup_hits"),
                         metric(r, "lookup_misses"), metric(r, "repeated_queries"),
                         metric(r, "answers_reused"), metric(r, "feature_keys_materialized"),
                         metric(r, "feature_key_bytes_materialized"),
                         metric(r, "direct_word_ids_offered"), metric(r, "direct_word_ids_consumed"),
                         metric(r, "direct_word_fallbacks"), metric(r, "model_rows"),
                         metric(r, "weights_accumulated"), metric(r, "allocation_calls"),
                         metric(r, "allocation_requested_bytes"), metric(r, "filter_checks"),
                         metric(r, "searches_avoided"), metric(r, "output_bytes"),
                         show(summary.get("distinct_patterns_total")),
                         show(summary.get("observations_total")),
                         show(summary.get("retained_patterns")), show(summary.get("omitted_patterns")),
                         show(summary.get("query_table_overflow_addresses"))])
        out.extend(table(columns, rows))

        for axis, dims in (("by_stage", config.get("breakdown_dimensions", {}).get("by_stage", [])),
                           ("by_family", config.get("breakdown_dimensions", {}).get("by_family", []))):
            out.extend([f"#### {axis.replace('_', ' ').title()} lookup totals", ""])
            rows = []
            for dim in dims:
                for variant in variants:
                    r = records[(dataset, variant)]
                    breakdown = r["metrics"]["_breakdowns"].get(axis, {})
                    vals = []
                    for name in ("lookup_calls", "lookup_hits", "lookup_misses", "repeated_queries"):
                        item = breakdown.get(name, {}).get("values", {}).get(dim)
                        vals.append("N/A" if item is None or item.get("value") is None else item["value"])
                    rows.append([dim, variant, *vals])
            out.extend(table(["Dimension", "Variant", "Calls", "Hits", "Misses", "Repeats"], rows))

    out.extend(["## Evidence and interpretation", "",
                "- Output checks are reported by the runner as byte-identical for each release and diagnostic output against the original output; thread evidence and input/output hashes remain in `results.json`.",
                f"- Reproduction identity: build records `{config.get('build_record_dir', 'unspecified')}`, artifacts `{config.get('build_artifacts_dir', 'unspecified')}`, source/config/input/model hashes in `manifest.json`, and per-dataset output hashes in `results.json`.",
                "- Repeated-pattern summaries are counts from all tracked queries. Any retained pattern rows are first-encountered examples, not top-frequency results; see raw result JSON for their bounded summaries.",
                "- Exact repeat identity is dictionary pointer + operation + key bytes within one run. Exported pattern rows merge by family + operation + key bytes and cannot establish cross-dictionary sharing.",
                "- Feature-key counts/bytes describe scorer-boundary strings, not all feature constructors or intermediate text. cpi `EXP_CP_IDS` and `EXP_ROLE_IDS` first-use precomputed model-ID request/consumption counts are not instrumented and remain unavailable; do not classify them as repeated-answer reuse.",
                "- Allocation metrics cover direct malloc/calloc/realloc sites in `address_parser.c` and `crf.c` during parsing only; they are not total heap growth or RSS. Hash probes, phrase-trie edges, complete key construction, and general text-copy bytes are unavailable.",
                "- `filter_checks` and `searches_avoided` apply only to cpi's enabled `EXP_PS_FLAGS` path; original and prototype181 have no matching filter path.",
                "- Necessary model row/weight contributions are separate from searches that might be shareable. This report does not infer savings or performance improvement.",
                "- Full source and metric boundary inventory: [`COVERAGE.md`](COVERAGE.md). Reproduction records, calibration and output evidence are retained in this experiment directory.", ""])
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("results", type=Path, help="completed run_measurements.py results.json")
    parser.add_argument("--output", type=Path, help="output Markdown path (default: sibling REPORT.md)")
    args = parser.parse_args()
    output = args.output or args.results.resolve().parent / "REPORT.md"
    output.write_text(render(args.results))
    print(output)


if __name__ == "__main__":
    main()
