# 159b sizing (model-derived, offline): bytes of the sorted key set front-coded in blocks of R keys (restart = full key + len byte + 4-byte block offset;
# other keys = suffix after the shared prefix + 2 bytes). Input: trie_struct TSVs. usage: python3 -I frontcode_size.py STRUCTDIR
import sys
def unesc(b):
    out=bytearray(); i=0
    while i<len(b):
        c=b[i]
        if c==0x5c and i+1<len(b): n=b[i+1]; out.append({0x74:9,0x6e:10,0x72:13,0x30:0}.get(n,n)); i+=2
        else: out.append(c); i+=1
    return bytes(out)
for name in ["component_phrases","postal_codes","address_dictionary"]:
    keys=sorted(unesc(l.rstrip(b"\n").split(b"\t")[0]) for l in open(sys.argv[1]+"/"+name+".struct.tsv","rb"))
    for R in (16,64):
        tot=0; prev=b""
        for i,k in enumerate(keys):
            if i%R==0: tot+=len(k)+1+4
            else:
                l=0; m=min(len(k),len(prev))
                while l<m and k[l]==prev[l]: l+=1
                tot+=len(k)-l+2
            prev=k
        print(name, "keys", len(keys), "raw+NUL", sum(len(k)+1 for k in keys), "front-coded restart", R, "->", tot, "bytes")
