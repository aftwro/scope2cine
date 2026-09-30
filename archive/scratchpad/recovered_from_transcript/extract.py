#!/usr/bin/env python3
"""Move a block of camapp.c into its own file.
usage: extract.py START_PREFIX END_PREFIX NEWFILE SECTION_TITLE HEADER_COMMENT_FILE
Block = lines from the one starting with START_PREFIX up to (not incl.) the one starting with END_PREFIX.
Static functions/variables in the block that are used elsewhere lose 'static' and get declared in
camapp.h under '/* NEWFILE */'. Returns nonzero on anything unexpected; writes nothing then."""
import re, sys
start, end, newfile, title, headfile = sys.argv[1:6]
L = open('camapp.c').read().split('\n')
def find(p):
    idx = [i for i, l in enumerate(L) if l.startswith(p)]
    if len(idx) != 1: sys.exit(f'prefix {p!r} matched {len(idx)} lines')
    return idx[0]
a, b = find(start), find(end)
blk, rest = L[a:b], L[:a] + L[b:]
text, rtext = '\n'.join(blk), '\n'.join(rest)
protos, externs = [], []
# functions
for m in list(re.finditer(r'^static ((?:const )?[\w\s\*]*?\b(\w+)\([^)]*\))\s*$', text, re.M)):
    name = m.group(2)
    if re.search(r'\b' + name + r'\b', rtext):
        protos.append(m.group(1).strip() + ';')
text = re.sub(r'^static ((?:const )?[\w\s\*]*?\b(\w+)\([^)]*\))\s*$',
              lambda m: m.group(1) if re.search(r'\b' + m.group(2) + r'\b', rtext) else m.group(0), text, flags=re.M)
# variables (one line, no braces)
lines = text.split('\n')
for i, l in enumerate(lines):
    if not l.startswith('static ') or '(' in l.split('=')[0] or '{' in l:
        continue
    m = re.match(r'static ([^;]*);', l)
    if not m: continue
    parts = [p.strip() for p in m.group(1).split(',')]
    tm = re.match(r'(.*?\b)(\**\s*\w+(?:\[[^\]]*\])*)\s*(?:=.*)?$', parts[0])
    if not tm: continue
    base = tm.group(1).strip()
    decls = [re.sub(r'\s*=.*', '', tm.group(2))] + [re.sub(r'\s*=.*', '', p) for p in parts[1:]]
    names = [re.findall(r'(\w+)(?:\[[^\]]*\])*$', d)[0] for d in decls]
    if any(re.search(r'\b' + n + r'\b', rtext) for n in names):
        lines[i] = l[len('static '):]
        externs.append('extern ' + base + ' ' + ', '.join(decls) + ';')
text = '\n'.join(lines)
head = open(headfile).read()
open(newfile, 'w').write(head + text.strip('\n') + '\n')
open('camapp.c', 'w').write('\n'.join(rest))
h = open('camapp.h').read()
h = h.replace('\n#endif', '\n/* ' + newfile + ': ' + title + ' */\n' + '\n'.join(protos + externs) + '\n\n#endif', 1)
open('camapp.h', 'w').write(h)
print(f'moved lines {a+1}-{b}; exported {len(protos)} functions, {len(externs)} variable lines')
for x in protos + externs: print('  ', x)
