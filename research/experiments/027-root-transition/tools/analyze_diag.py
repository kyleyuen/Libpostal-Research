#!/usr/bin/env python3
"""Summarize paired development-only benchmark passes; retain raw runs."""
from pathlib import Path
from statistics import median

root = Path(__file__).resolve().parents[1] / "raw" / "commands"
values = {"control_r2": [], "cache_r2": []}
paired = []

for trial in range(1, 6):
    pair = {}
    for variant in values:
        path = root / f"diag_t{trial}_{variant}.stdout"
        lines = path.read_text().splitlines()
        setup = float(lines[0].split()[0].split("=")[1])
        passes = []
        for line in lines:
            if line.startswith("pass="):
                item = dict(field.split("=", 1) for field in line.split())
                passes.append({key: float(value) for key, value in item.items()})
        if len(passes) != 120:
            raise SystemExit(f"{path}: expected 120 passes, got {len(passes)}")
        warm = passes[1:]
        clean = [p for p in warm if p["nivcsw"] == 0 and p["us_per_addr"] <= p["cpu_us_per_addr"] * 1.02]
        speed = median(p["addresses_per_second"] for p in warm)
        values[variant].extend(warm)
        pair[variant] = speed
        print(f"trial={trial} variant={variant} setup_s={setup:.3f} first_us={passes[0]['us_per_addr']:.3f} "
              f"warm_median_addr_s={speed:.1f} warm_cpu_us={median(p['cpu_us_per_addr'] for p in warm):.3f} "
              f"clean={len(clean)}/{len(warm)}")
    ratio = pair["cache_r2"] / pair["control_r2"]
    paired.append(ratio)
    print(f"trial={trial} candidate_over_control={ratio:.5f}")

for variant, passes in values.items():
    rates = sorted(p["addresses_per_second"] for p in passes)
    clean = [p for p in passes if p["nivcsw"] == 0 and p["us_per_addr"] <= p["cpu_us_per_addr"] * 1.02]
    p10 = rates[int(0.1 * (len(rates) - 1))]
    print(f"pooled variant={variant} passes={len(passes)} median_addr_s={median(rates):.1f} "
          f"p10_addr_s={p10:.1f} median_cpu_us={median(p['cpu_us_per_addr'] for p in passes):.3f} "
          f"clean={len(clean)}/{len(passes)}")
print(f"paired_ratio median={median(paired):.5f} min={min(paired):.5f} max={max(paired):.5f}")
