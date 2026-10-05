import re,sys
def load(path):
    x=open(path).read(); rows=re.findall(r'<row>(.*?)</row>',x,re.S); refs={};out=[]
    for r in rows:
        m=re.search(r'<uint64-array id="(\d+)"[^>]*>([^<]*)<',r)
        if m: refs[m.group(1)]=m.group(2); v=m.group(2)
        else: v=refs[re.search(r'<uint64-array ref="(\d+)"',r).group(1)]
        out.append((int(re.search(r'<start-time[^>]*>(\d+)<',r).group(1))/1e9,list(map(int,v.split()))))
    return out
if __name__=="__main__":
    lo,hi=float(sys.argv[1]),float(sys.argv[2])
    for p in sys.argv[3:]:
        d=load(p); s=[0]*4
        for t,v in d:
            if lo<=t<hi:
                for i in range(4): s[i]+=v[i]
        T=sum(s) or 1
        print(f"{p:32s} rows={len(d)} window[{lo},{hi})s  col0={100*s[0]/T:5.1f}% col1={100*s[1]/T:5.1f}% col2={100*s[2]/T:5.1f}% col3={100*s[3]/T:5.1f}%")
