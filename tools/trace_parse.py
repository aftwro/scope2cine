#!/usr/bin/env python3
"""Parse an mitrace log into call records: (pid, name, args[8], dumps{argidx: [words]}, ret).

Usage: trace_parse.py <log> [name-regex]   -> prints matching calls with dumps trimmed at trailing zeros.
"""
import re, sys

CALL = re.compile(r'^\[(\d+)\] [\d.]+ (MI_\w+)\(([^)]*)\)')
DUMP = re.compile(r'^    a(\d)@([0-9a-f]+):((?: [0-9a-f]{8})+)')
RET = re.compile(r'^  \[(\d+)\] -> (MI_\w+) = (-?\d+)')


def parse(path):
    calls, open_by_pid = [], {}
    cur = None
    for line in open(path, errors='replace'):
        m = CALL.match(line)
        if m:
            cur = {'pid': int(m[1]), 'name': m[2], 'args': [int(x, 16) for x in m[3].split(', ')],
                   'in': {}, 'out': {}, 'ret': None}
            calls.append(cur)
            open_by_pid[cur['pid']] = cur
            continue
        m = RET.match(line)
        if m:
            cur = open_by_pid.get(int(m[1]))
            if cur is not None and cur['name'] == m[2]:
                cur['ret'] = int(m[3])
            continue
        m = DUMP.match(line)
        if m and cur is not None:
            words = [int(w, 16) for w in m[3].split()]
            (cur['out'] if cur['ret'] is not None else cur['in'])[int(m[1])] = words
    return calls


def trim(words):
    n = len(words)
    while n and words[n - 1] == 0:
        n -= 1
    return words[:n]


if __name__ == '__main__':
    pat = re.compile(sys.argv[2] if len(sys.argv) > 2 else '.')
    for c in parse(sys.argv[1]):
        if not pat.search(c['name']):
            continue
        print(f"[{c['pid']}] {c['name']}({', '.join(hex(a) for a in c['args'])}) = {c['ret']}")
        for k, d in (('in', c['in']), ('out', c['out'])):
            for i, w in d.items():
                print(f"    {k} a{i}: {' '.join('%x' % x for x in trim(w)[:40])}")
