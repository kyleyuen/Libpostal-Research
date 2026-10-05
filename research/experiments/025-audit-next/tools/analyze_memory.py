import glob
import re
import statistics as st
from pathlib import Path

directory = Path(__file__).resolve().parents[1] / 'raw' / 'final_timing'
for dataset in ('v1-all', 'novel'):
    for variant in ('baseline', 'fixes', 'borrow'):
        files = sorted(glob.glob(str(directory / f'{dataset}-t*-{variant}.stderr')))
        peaks = [int(re.search(r'(\d+)\s+peak memory footprint', Path(f).read_text()).group(1)) for f in files]
        assert len(peaks) == 5
        print(f'{dataset} {variant}: peak bytes min={min(peaks)} median={int(st.median(peaks))} max={max(peaks)}; all={peaks}')
