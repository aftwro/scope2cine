#!/usr/bin/env python3
"""Read compiler output on stdin; for every function reported undeclared/implicit that camapp.c
defines as static, drop 'static' there and add its prototype to camapp.h under '/* camapp.c */'."""
import re, sys
errs = sys.stdin.read()
names = sorted(set(re.findall(r"implicit declaration of function ‘(\w+)’", errs)))
c = open('camapp.c').read(); h = open('camapp.h').read()
done = []
for n in names:
    m = re.search(r'^static ((?:const )?[\w\s\*]*?\b' + n + r'\([^)]*\))\s*$', c, re.M)
    if not m:
        print('not found as static function in camapp.c:', n); continue
    c = c[:m.start()] + m.group(1) + c[m.end():]
    c = re.sub(r'^static ([^\n]*\b' + n + r'\([^)]*\));\s*$', r'\1;', c, flags=re.M)   # forward decls
    h = h.replace('/* camapp.c */\n', '/* camapp.c */\n' + m.group(1).strip() + ';\n', 1)
    done.append(n)
open('camapp.c', 'w').write(c); open('camapp.h', 'w').write(h)
print('exported from camapp.c:', done)
