import re
import statistics as st
from pathlib import Path

root = Path(__file__).resolve().parent / "runtime"


def read(tag):
    d = root / tag
    settings = (d / "settings.txt").read_text()
    assert "exit=0" in settings and "max_threads=1" in settings, tag
    output = (d / "bench.stdout").read_text()
    peak = int(re.search(r"peak_rss_bytes=(\d+)", output)[1])
    setup = float(re.search(r"setup_seconds=([\d.]+)", output)[1])
    samples = (d / "samples.txt").read_text()
    rss = [int(x) * 1024 for x in re.findall(r"^VmRSS:\s*(\d+) kB", samples, re.M)]
    swap = [int(x) * 1024 for x in re.findall(r"^VmSwap:\s*(\d+) kB", samples, re.M)]
    assert rss and swap and max(swap) == 0, tag
    return peak, int(st.median(rss[-8:])), setup, len(rss)


for corpus in ("v1", "osm"):
    baseline = read(f"{corpus}_upstream")
    dense = read(f"{corpus}_dense")
    print(corpus, "upstream peak/late/setup/samples=", baseline)
    print(corpus, "dense peak/late/setup/samples=", dense)
    print(corpus, "peak_diff_bytes=", dense[0] - baseline[0],
          "late_rss_diff_bytes=", dense[1] - baseline[1])
