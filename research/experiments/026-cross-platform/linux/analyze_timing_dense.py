import re
import statistics as st
import sys
from pathlib import Path

directory = Path(sys.argv[1])
trials = int(sys.argv[2])
variants = ("upstream", "candidate", "dense")
corpora = tuple(sys.argv[3:]) if len(sys.argv) > 3 else ("v1-all", "novel")


def p10(values):
    values = sorted(values)
    return values[int((len(values) - 1) * 0.1)]


def read(corpus, trial, variant):
    tag = f"{corpus}-t{trial}-{variant}"
    meta = (directory / f"{tag}.meta").read_text()
    assert "exit=0" in meta, tag
    output = (directory / f"{tag}.stdout").read_text()
    error = (directory / f"{tag}.stderr").read_text()
    runs = []
    countries = {}
    for line in output.splitlines():
        if line.startswith("pass="):
            m = re.match(
                r"pass=(\d+) us_per_addr=([\d.]+) addresses_per_second=([\d.]+) "
                r"cpu_us_per_addr=([\d.]+) nivcsw=(\d+) nvcsw=(\d+)", line
            )
            assert m, line
            runs.append(tuple(map(float, m.groups())))
        elif line.startswith("country "):
            m = re.match(r"country pass=(\d+) cc=(\S+) n=(\d+) us_per_addr=([\d.]+)", line)
            assert m, line
            countries[(int(m[1]), m[2])] = float(m[4])
    assert runs, tag
    peak = int(re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", error)[1]) * 1024
    setup = float(re.search(r"setup_seconds=([\d.]+)", output)[1])
    return runs, countries, peak, setup


for corpus in corpora:
    data = {(t, v): read(corpus, t, v) for t in range(1, trials + 1) for v in variants}
    print(f"## {corpus}")
    for variant in variants:
        blocks = [data[(t, variant)] for t in range(1, trials + 1)]
        runs = [p for block in blocks for p in block[0]]
        warm = [p for p in runs if p[0] > 1]
        clean = [p for p in warm if p[4] == 0 and p[1] <= p[3] * 1.02]
        peaks = [b[2] for b in blocks]
        print(
            variant,
            "runs=", len(runs),
            "warm_median_us=", round(st.median(p[1] for p in warm), 3),
            "warm_p10_us=", round(p10([p[1] for p in warm]), 3),
            "warm_cpu_median_us=", round(st.median(p[3] for p in warm), 3),
            "prewarmed_first_pass_median_us=", round(st.median(b[0][0][1] for b in blocks), 3),
            "clean_n=", len(clean),
            "clean_median_us=", round(st.median(p[1] for p in clean), 3) if clean else "none",
            "setup_median_s=", round(st.median(b[3] for b in blocks), 3),
            "peak_max_bytes=", max(peaks),
            "peak_all_bytes=", peaks,
        )
    for variant in variants[1:]:
        paired = []
        for t in range(1, trials + 1):
            base = data[(t, "upstream")][0]
            cand = data[(t, variant)][0]
            paired.append(st.median(p[2] for p in cand[1:]) / st.median(p[2] for p in base[1:]))
        print(variant, "paired_speedups=", [round(x, 4) for x in paired],
              "median=", round(st.median(paired), 4))
        for pass_kind in (1, int(data[(1, variant)][0][-1][0])):
            ratios = []
            for t in range(1, trials + 1):
                baseline_cc = data[(t, "upstream")][1]
                variant_cc = data[(t, variant)][1]
                for (p, cc), base_us in baseline_cc.items():
                    if p == pass_kind and (p, cc) in variant_cc:
                        ratios.append(base_us / variant_cc[(p, cc)])
            print(variant, "country_pass=", pass_kind,
                  "country_speedup_min_med_max=",
                  tuple(round(f(ratios), 3) for f in (min, st.median, max)))
