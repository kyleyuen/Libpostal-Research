"""Function-level report from a macOS `sample` file: inclusive/exclusive shares, category totals, immediate callers of allocation functions.
usage: profile_report.py sample.txt"""
import re, sys, collections
text = open(sys.argv[1]).read().split("Call graph:\n", 1)[1].split("Total number in stack", 1)[0]
stack, nodes = [], []
for line in text.splitlines():
    m = re.match(r"^([ +!:|`-]*)(\d+) (.+?)(?:  \(in |$)", line)
    if not m: continue
    depth, count, name = len(m[1]), int(m[2]), m[3].strip()
    name = re.sub(r"\s*\+\s*[\d,]+.*$", "", name)
    while stack and stack[-1]["depth"] >= depth: stack.pop()
    n = dict(depth=depth, count=count, name=name, children=0, anc=[x["name"] for x in stack], parent=(stack[-1]["name"] if stack else None))
    if stack: stack[-1]["children"] += count
    stack.append(n); nodes.append(n)
total = sum(n["count"] for n in nodes if not n["anc"])
incl, excl = collections.Counter(), collections.Counter()
for n in nodes:
    own = n["count"] - n["children"]; excl[n["name"]] += own
    for c in set(n["anc"] + [n["name"]]): incl[c] += own
print("total samples", total)
def cat(name):
    l = name.lower()
    if re.search(r"malloc|_free|realloc|calloc|xzm|nano|szone|zone_size|_platform_memset|__bzero", l): return "allocator + zeroing (malloc/free/realloc/memset/bzero)"
    if re.search(r"memmove|memcpy|memcmp|strlen|strncmp|strcmp|strchr|memchr|strndup|strdup", l): return "libc string/mem ops"
    if re.search(r"utf8proc|utf8_|transliterate|normalize|decompose|case", l): return "normalization/unicode"
    if re.search(r"trie_search|trie_get|trie_|phrase", l): return "trie / phrase search"
    if re.search(r"token|scan_|tokenize", l): return "tokenizing"
    if re.search(r"char_array|cstring_array|feature_array|vjoined", l): return "string arrays / feature strings"
    if re.search(r"crf_|viterbi|double_array|state_score", l): return "CRF scoring / viterbi"
    if re.search(r"fd_|feature_dict", l): return "dictionary lookup"
    return "other"
cats = collections.Counter()
for k, v in excl.items(): cats[cat(k)] += v
print("\n-- exclusive time by category (% of total; non-overlapping)")
for k, v in cats.most_common(): print(f"  {100*v/total:5.1f}%  {k}")
print("\n-- inclusive (overlapping) for pipeline stages")
for f in ["libpostal_parse_address","address_parser_parse","address_parser_normalize_string","normalize_string_utf8","tokenize","tokenized_string_new_from_str_size","tokenized_string_add_token","address_parser_context_fill","address_parser_predict","crf_tagger_score","address_parser_features","word_vocab_frequency","trie_get_data","trie_search_tokens_from_index","normalize_token","add_normalized_token","cstring_array_get_phrase","crf_tagger_predict","crf_context_viterbi","crf_context_set_num_items","libpostal_address_parser_response_destroy","address_parser_response_new","cstring_array_to_strings","char_array_add_vjoined","feature_array_add","address_expansion_in_dictionary","search_address_dictionaries_prefix","search_address_dictionaries_suffix"]:
    if incl[f]: print(f"  {100*incl[f]/total:5.1f}%  {f}")
print("\n-- top exclusive functions")
for k, v in excl.most_common(28): print(f"  {100*v/total:5.1f}%  {k}")
alloc = re.compile(r"^(_malloc|malloc|_free|free|_realloc|realloc|_calloc|calloc|_xzm_free|_xzm_xzone_malloc.*|xzm_realloc|_malloc_zone_realloc|_malloc_zone_malloc|_malloc_zone_calloc|_malloc_zone_free|_szone.*|nano.*|_nanov2.*)$")
callers = collections.defaultdict(collections.Counter)
for n in nodes:
    if alloc.match(n["name"]) and n["parent"] and not alloc.match(n["parent"]): callers[n["parent"]][n["name"]] += n["count"]
tot_alloc = sum(sum(c.values()) for c in callers.values())
print(f"\n-- allocator entry points: {100*tot_alloc/total:.1f}% of samples inclusive at the outermost allocator call; by immediate caller:")
for k, c in sorted(callers.items(), key=lambda kv: -sum(kv[1].values()))[:14]: print(f"  {100*sum(c.values())/total:5.2f}%  {k}")
