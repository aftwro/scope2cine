import serial,time
# Log every key-down on /dev/input/event0 to a file until killed.
s=serial.Serial('/dev/ttyUSB0',115200,timeout=0.2)
s.reset_input_buffer(); s.write(b'\x03\n'); time.sleep(0.5); s.read(8192)
s.write(b"hexdump -v -e '16/1 \"%02x \" \"\\n\"' /dev/input/event0\n")
out=open('/tmp/claude-1000/-home-aftwr-microscopeproject/9d550714-17fb-41e2-87ba-ede55841232b/scratchpad/keys.log','a',buffering=1)
out.write('listener started\n')
buf=''
while True:
    buf+=s.read(4096).decode('utf-8','replace').replace('\r','')
    *lines,buf=buf.split('\n')
    for l in lines:
        p=l.split()
        if len(p)!=16: continue
        x=bytes(int(h,16) for h in p)
        typ=int.from_bytes(x[8:10],'little'); code=int.from_bytes(x[10:12],'little'); val=int.from_bytes(x[12:16],'little',signed=True)
        if typ==1 and val==1:
            out.write(f'{time.strftime("%H:%M:%S")} key code {code}\n')
