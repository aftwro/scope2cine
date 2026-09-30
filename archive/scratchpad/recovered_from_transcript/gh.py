import json, sys, urllib.request
URL = 'http://127.0.0.1:8765/mcp'
H = {'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream'}
def post(body, sid=None):
    h = dict(H); 
    if sid: h['mcp-session-id'] = sid
    r = urllib.request.urlopen(urllib.request.Request(URL, json.dumps(body).encode(), h), timeout=600)
    sid = r.headers.get('mcp-session-id') or sid
    txt = r.read().decode()
    for line in txt.splitlines():
        if line.startswith('data:'): return json.loads(line[5:]), sid
    return (json.loads(txt) if txt.strip() else None), sid
_, sid = post({'jsonrpc':'2.0','id':1,'method':'initialize','params':{'protocolVersion':'2025-06-18','capabilities':{},'clientInfo':{'name':'cli','version':'1'}}})
post({'jsonrpc':'2.0','method':'notifications/initialized'}, sid)
if sys.argv[1] == 'list':
    r, _ = post({'jsonrpc':'2.0','id':2,'method':'tools/list'}, sid)
    for t in r['result']['tools']: print(t['name'], '-', json.dumps(t['inputSchema'].get('properties',{}))[:200])
else:
    r, _ = post({'jsonrpc':'2.0','id':2,'method':'tools/call','params':{'name':sys.argv[1],'arguments':json.loads(sys.argv[2])}}, sid)
    for c in r.get('result',{}).get('content',[]): print(c.get('text',''))
    if 'error' in r: print(r['error'])
