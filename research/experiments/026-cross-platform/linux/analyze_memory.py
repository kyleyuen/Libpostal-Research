import re
import statistics as st
from pathlib import Path

root = Path(__file__).resolve().parent / "runtime"
groups = {
    "upstream": ["upstream_novel", "t2_upstream", "t3_upstream"],
    "dense": ["candidate_dense_novel", "t2_dense", "t3_dense"],
    "l4_glibc": ["candidate_novel"],
    "l4_mi2": ["candidate_mi_novel"],
    "l4_mi3": ["candidate_mi3_novel"],
}
peaks = {}
for variant, tags in groups.items():
    values = []
    for tag in tags:
        d = root / tag
        setting = (d / "settings.txt").read_text()
        assert "exit=0" in setting and "max_threads=1" in setting, tag
        output = (d / "bench.stdout").read_text()
        peak = int(re.search(r"peak_rss_bytes=(\d+)", output)[1])
        sample = (d / "samples.txt").read_text()
        rss = [int(x) * 1024 for x in re.findall(r"^VmRSS:\s*(\d+) kB", sample, re.M)]
        swap = [int(x) * 1024 for x in re.findall(r"^VmSwap:\s*(\d+) kB", sample, re.M)]
        assert rss and swap, tag
        steady = int(st.median(rss[-8:]))
        values.append(peak)
        print(tag, "peak=", peak, "late_rss_median=", steady,
              "max_vm_swap=", max(swap), "samples=", len(rss))
    peaks[variant] = values
    print(variant, "peak_max=", max(values), "peak_all=", values)
base = peaks["upstream"]
for variant in ("dense", "l4_glibc", "l4_mi2", "l4_mi3"):
    cand = peaks[variant]
    print(variant, "max_peak_diff_bytes=", max(cand) - max(base))
    if len(cand) == len(base):
        print(variant, "paired_peak_diff_bytes=", [c - b for c, b in zip(cand, base)])
