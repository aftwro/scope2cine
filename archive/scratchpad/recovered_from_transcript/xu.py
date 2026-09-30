import fcntl, os, struct, ctypes, sys
IOC = (3<<30)|(16<<16)|(ord('u')<<8)|0x21
fd = os.open('/dev/video0', os.O_RDWR)
def q(unit, sel, query, size):
    buf = ctypes.create_string_buffer(size)
    arg = bytearray(struct.pack('BBBxHxxQ', unit, sel, query, size, ctypes.addressof(buf)))
    try:
        fcntl.ioctl(fd, IOC, arg); return buf.raw
    except OSError as e: return 'ERR %s' % e.strerror
for unit in (2, 6):
    for sel in range(1, 17):
        info = q(unit, sel, 0x86, 1); ln = q(unit, sel, 0x85, 2)
        if isinstance(ln, str): print(f'u{unit} s{sel}: INFO={info} LEN={ln}'); continue
        n = struct.unpack('<H', ln)[0]
        out = f'u{unit} s{sel}: INFO=0x{info[0]:02x} LEN={n}'
        if 0 < n <= 64:
            for name, c in (('CUR',0x81),('MIN',0x82),('MAX',0x83),('DEF',0x87)):
                r = q(unit, sel, c, n); out += f' {name}=' + (r if isinstance(r,str) else r.hex())
        print(out, flush=True)
