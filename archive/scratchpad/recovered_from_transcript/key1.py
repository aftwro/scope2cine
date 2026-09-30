import serial,time,sys
# Wait for one key press on /dev/input/event0 and print its key code (and any scancode).
s=serial.Serial('/dev/ttyUSB0',115200,timeout=0.2)
s.reset_input_buffer(); s.write(b'\n'); time.sleep(0.3); s.read(4096)
s.write(b"hexdump -v -e '16/1 \"%02x \" \"\\n\"' /dev/input/event0\n")
b=b'';t=time.time();found=[]
while time.time()-t<30 and not found:
    b+=s.read(4096)
    for l in b.decode('utf-8','replace').replace('\r','').splitlines():
        p=l.split()
        if len(p)!=16: continue
        x=bytes(int(h,16) for h in p)
        typ=int.from_bytes(x[8:10],'little'); code=int.from_bytes(x[10:12],'little'); val=int.from_bytes(x[12:16],'little',signed=True)
        if typ==1 and val==1: found.append(code)
s.write(b'\x03'); time.sleep(0.5); s.read(8192)
print(f'{sys.argv[1]}: key code {found[0]}' if found else f'{sys.argv[1]}: nothing within 30 s')
