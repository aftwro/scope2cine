import re,struct,sys,subprocess
f=open('bootconfig/bin/cardv','rb').read()
out=subprocess.run(['arm-linux-gnueabihf-readelf','-W','-l','bootconfig/bin/cardv'],capture_output=True,text=True).stdout
segs=[(int(p[1],16),int(p[2],16),int(p[4],16)) for p in (l.split() for l in out.splitlines()) if p and p[0]=='LOAD']
def v2o(v):
    for off,va,sz in segs:
        if va<=v<va+sz: return v-va+off
lines=open(sys.argv[1]).read().splitlines()
targets={int(x,16) for x in sys.argv[2:]}
pend={}
func=''
for l in lines:
    m=re.match(r'^([0-9a-f]+) <(.*)>:',l)
    if m: func=m.group(2); pend={}; continue
    m=re.match(r'\s+([0-9a-f]+):\s+[0-9a-f ]+\s+(\S+)\s+(.*)',l)
    if not m: continue
    a=int(m.group(1),16); op=m.group(2); args=m.group(3)
    mm=re.match(r'(r\d+|ip|lr), \[pc, #\d+\]\s+@ \(([0-9a-f]+)',args)
    if op.startswith('ldr') and mm:
        o=v2o(int(mm.group(2),16))
        if o is not None: pend[mm.group(1)]=struct.unpack_from('<I',f,o)[0]
        continue
    mm=re.match(r'(r\d+|ip), pc$',args)
    if op=='add' and mm and mm.group(1) in pend:
        t=(pend.pop(mm.group(1))+a+4)&0xffffffff
        if t in targets: print(hex(t),'ref at',hex(a),'in',func)
