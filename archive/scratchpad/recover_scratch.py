#!/usr/bin/env python3
"""Rebuild scratchpad files from the Claude Code session transcript.

The session scratchpad lived in /tmp (tmpfs) and was wiped when WSL restarted.
Every file the assistant created there with the Write tool, changed with the
Edit tool, or wrote with a shell heredoc (cat > FILE <<'EOF') is replayed here
in transcript order. Calls that the harness reported as errors are skipped.
Files produced by other programs (objdump listings, Ghidra projects, captured
frames) cannot be rebuilt this way.

Usage: recover_scratch.py TRANSCRIPT.jsonl OUT_DIR
"""
import json, os, re, sys

src, out = sys.argv[1], sys.argv[2]
MARK = re.compile(r'/(?:scratchpad|scratch)/(.+)$')
# cat > FILE <<'EOF' ... EOF  and  cat <<'EOF' > FILE ... EOF  (tee FILE works like cat > FILE)
HEREDOC = re.compile(r"(?:cat\s*>|tee)\s*(?P<f1>[^\s<>;&|]+)\s*<<-?\s*(?P<q1>['\"]?)(?P<d1>\w+)(?P=q1)[^\n]*\n(?P<b1>.*?)\n(?P=d1)(?=\n|$)"
                     r"|cat\s*<<-?\s*(?P<q2>['\"]?)(?P<d2>\w+)(?P=q2)\s*>\s*(?P<f2>[^\s<>;&|]+)[^\n]*\n(?P<b2>.*?)\n(?P=d2)(?=\n|$)", re.S)
CD = re.compile(r"^\s*cd\s+(\S+)\s*&&")

uses, errors = [], set()
with open(src) as f:
    for line in f:
        try:
            m = json.loads(line)
        except ValueError:
            continue
        msg = m.get('message') or {}
        content = msg.get('content')
        if not isinstance(content, list):
            continue
        for c in content:
            if c.get('type') == 'tool_use':
                uses.append(c)
            elif c.get('type') == 'tool_result' and c.get('is_error'):
                errors.add(c.get('tool_use_id'))

files, log = {}, []
def rel(path):
    m = MARK.search(path or '')
    return m.group(1) if m and '/microscopeproject/scratch/' not in path else None

for u in uses:
    if u.get('id') in errors:
        continue
    name, inp = u.get('name'), u.get('input') or {}
    if name == 'Write':
        r = rel(inp.get('file_path'))
        if r:
            files[r] = inp.get('content', '')
            log.append(('write', r))
    elif name == 'Edit':
        r = rel(inp.get('file_path'))
        if r and r in files:
            old, new = inp.get('old_string', ''), inp.get('new_string', '')
            if old in files[r]:
                files[r] = files[r].replace(old, new) if inp.get('replace_all') else files[r].replace(old, new, 1)
                log.append(('edit', r))
            else:
                log.append(('edit-miss', r))
    elif name == 'Bash':
        cmd = inp.get('command', '')
        cd = CD.match(cmd)
        base = cd.group(1) if cd else ''
        for m in HEREDOC.finditer(cmd):
            path = m.group('f1') or m.group('f2')
            quoted = m.group('q1') if m.group('f1') else m.group('q2')
            body = m.group('b1') if m.group('f1') else m.group('b2')
            if not path.startswith('/') and base:
                path = base.rstrip('/') + '/' + path
            r = rel(path)
            if r:
                if not quoted:        # unquoted delimiter: the shell removed these escapes when it ran
                    body = body.replace('\\$', '$').replace('\\`', '`').replace('\\\\', '\\')
                files[r] = body + '\n'
                log.append(('heredoc', r))

for r, text in files.items():
    p = os.path.join(out, r)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, 'w') as f:
        f.write(text)
with open(os.path.join(out, 'RECOVERY_LOG.txt'), 'w') as f:
    for kind, r in log:
        f.write(f'{kind}\t{r}\n')
print(f'{len(files)} files rebuilt, {len(log)} operations replayed')
