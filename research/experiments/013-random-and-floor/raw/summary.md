| set | addresses | baseline µs/addr | candidate µs/addr | speedup (median) | speedup (P10, interference-resistant) | baseline addr/s | candidate addr/s |
|---|---:|---:|---:|---:|---:|---:|---:|
| ascii_only | 7459 | 33.06 | 11.38 | 2.906x | 2.903x | 30,251 | 87,904 |
| floor_non_ascii_only | 5404 | 34.16 | 14.67 | 2.329x | 2.313x | 29,275 | 68,171 |
| full_v1 | 12863 | 34.88 | 13.22 | 2.638x | 2.628x | 28,666 | 75,632 |
| random_1 | 2000 | 34.76 | 12.86 | 2.703x | 2.807x | 28,773 | 77,773 |
| random_2 | 2000 | 34.62 | 12.88 | 2.688x | 2.745x | 28,885 | 77,640 |
| random_3 | 2000 | 34.65 | 12.77 | 2.713x | 2.736x | 28,859 | 78,309 |
| random_4 | 2000 | 34.64 | 12.98 | 2.669x | 2.757x | 28,865 | 77,039 |
| random_5 | 2000 | 34.80 | 12.88 | 2.701x | 2.701x | 28,738 | 77,616 |
| random_6 | 2000 | 34.64 | 12.91 | 2.683x | 2.727x | 28,871 | 77,447 |
| random_7 | 2000 | 35.05 | 13.01 | 2.694x | 2.746x | 28,530 | 76,873 |
| random_8 | 2000 | 35.03 | 13.09 | 2.677x | 2.648x | 28,544 | 76,418 |

random 2,000-address subsets of the real set: n=8 speedup min 2.669x median 2.691x max 2.713x
