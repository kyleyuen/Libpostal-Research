# City-selection exclusion audit

Audit performed 2026-10-10 before v14 handoff. I parsed each `CITIES` literal
from the eleven prior OSM builders (v3 through v13), normalized names with
Unicode `casefold()`, and compared it to every city in v14's predeclared pool.
The prior builders contained 627 distinct normalized city names; the v14 pool
contains 67; overlap was empty. All 15 selected names are in that pool.

Prior files read:

```text
profiling/golden-v3-osm/build_osm.py
profiling/golden-v4-osm-unseen/build_unseen4.py
profiling/golden-v5-osm-unseen/build_unseen5.py
profiling/golden-v6-osm-unseen/build_unseen6.py
profiling/golden-v7-osm-unseen/build_unseen7.py
profiling/golden-v8-osm-unseen/build_unseen8.py
profiling/golden-v9-osm-unseen/build_unseen9.py
profiling/golden-v10-osm-unseen/build_unseen10.py
profiling/golden-v11-osm-unseen/build_unseen11.py
profiling/golden-v12-osm-unseen/build_unseen12.py
profiling/golden-v13-osm-unseen/build_unseen13.py
```

This checks exact city-name reuse in the recorded builder lists. The separate
address audit independently compares all 1,801 v14 strings with the retained
golden-v1, novel and v3–v13 inputs and found zero exact overlaps. It does not
prove geographic disjointness at arbitrary radii or a different source.
