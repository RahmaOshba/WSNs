#!/usr/bin/env python3
"""Port one thesis ns-3 program (code/<group>/<name>.cc) to an OMNeT++ protocol engine.

The protocol logic is copied unchanged. Only the ns-3 glue is replaced:
  * ns-3 includes, NetAnim and CommandLine code are removed;
  * the file is wrapped in its own namespace (all engines link into one binary);
  * every compile-time switch  #ifndef X / #define X v / #endif  becomes a variable
    that the ini parameter `defines` can set (same names as the -D switches);
    constants derived from a switch are recomputed when the run starts;
  * main() becomes Run(wsn::Hook&): after every round it reports to the
    Controller (which advances simulation time), and at the end it reports
    FND/HND/LND/PDR exactly as the ns-3 program computed them;
  * cout goes to the OMNeT++ log (parameter verbose), CSV files to csvDir.

usage: port_ns3.py <ns3 file> <output .cc> <group>
"""
import re
import sys

src_path, out_path, group = sys.argv[1], sys.argv[2], sys.argv[3]
name = re.sub(r'\.cc$', '', src_path.split('/')[-1])
lines = open(src_path, encoding='utf-8').read().split('\n')

# ---------------------------------------------------------------- split at main
main_idx = next(i for i, l in enumerate(lines) if re.match(r'^int main\s*\(', l))
head, main = lines[:main_idx], lines[main_idx:]


def drop_function(body, pattern):
    """Remove a top-level function whose first line matches pattern (brace matched)."""
    out, i = [], 0
    while i < len(body):
        if re.match(pattern, body[i]):
            depth, started = 0, False
            while i < len(body):
                depth += body[i].count('{') - body[i].count('}')
                started = started or '{' in body[i]
                i += 1
                if started and depth == 0:
                    break
            continue
        out.append(body[i])
        i += 1
    return out


head = drop_function(head, r'^static void ApplyVisualState\s*\(')
head = [l for l in head if not re.match(r'^// -+ NetAnim visualization -+\s*$', l)]

# ---------------------------------------------------------------- head: includes, macros, constants
includes, body = [], []
for l in head:
    if l.startswith('#include'):
        if '"ns3/' not in l:
            includes.append(l)
        continue
    if re.match(r'^\s*using namespace (ns3|std)\s*;', l) or l.startswith('NS_LOG_COMPONENT_DEFINE'):
        continue
    body.append(l)

switches = []          # (name, type, default)
out_body, i = [], 0
while i < len(body):
    m = re.match(r'^#ifndef\s+(\w+)\s*$', body[i])
    if m:
        j = i + 1
        comments = []
        while j < len(body) and body[j].strip().startswith('//'):
            comments.append(body[j]); j += 1
        d = re.match(r'^#define\s+' + m.group(1) + r'\s+(\S+)\s*(//.*)?$', body[j]) if j < len(body) else None
        if d and body[j + 1].strip().startswith('#endif'):
            val = d.group(1)
            typ = 'double' if re.search(r'[.eE]', val) else 'int'
            switches.append((m.group(1), typ, val))
            out_body += comments
            out_body.append('static %s %s = %s;   // switch (ini: defines = "%s=...")%s' %
                            (typ, m.group(1), val, m.group(1), ('  ' + d.group(2)) if d.group(2) else ''))
            i = j + 2
            continue
    out_body.append(body[i])
    i += 1
body = out_body

runtime = {s[0] for s in switches}
recompute = []
decl = re.compile(r'^(static\s+)?(constexpr|const)\s+([\w:<>]+)\s+(\w+)\s*=\s*(.+?);(\s*//.*)?$')
for k, l in enumerate(body):
    m = decl.match(l)
    if not m:
        continue
    expr = m.group(5)
    if any(re.search(r'\b%s\b' % re.escape(n), expr) for n in runtime):
        body[k] = 'static %s %s = %s;%s' % (m.group(3), m.group(4), expr, m.group(6) or '')
        recompute.append('%s = %s;' % (m.group(4), expr))
        runtime.add(m.group(4))

body = [re.sub(r'\b(std::)?cout\b', 'wsn::Out()', l) for l in body]

# ---------------------------------------------------------------- main
drop = re.compile(r'CommandLine\s+cmd|\bcmd\.|NodeContainer\s+visualNodes|\bvisualNodes\.|ListPositionAllocator|'
                  r'positionAlloc->|MobilityHelper|\bmobility\.|AnimationInterface|\banim\.|ApplyVisualState\(&anim|'
                  r'Simulator::(Stop|Run|Destroy)|visualTime\s*=|const\s+uint32_t\s+leaderId\s*=')
m_out, i = [], 0
while i < len(main):
    l = main[i]
    if re.search(r'^\s*const\s+(std::)?vector<\w+>\s+visualSnapshot\s*=', l):
        # snapshot + Simulator::Schedule([...]{ ApplyVisualState ... }); for NetAnim
        while not re.match(r'^\s*\}\);\s*$', main[i]):
            i += 1
        i += 1
        continue
    if drop.search(l):
        # a 'for (...)' header on the previous line belongs to the dropped statement
        if m_out and re.match(r'^\s*for\s*\(.*\)\s*$', m_out[-1]):
            m_out.pop()
        i += 1
        continue
    m_out.append(l)
    i += 1
main = m_out

main[0] = 'static int Run(wsn::Hook& HOOK)'
# NetAnim mentions in the console output
main = [re.sub(r'"Visualization = [^"]*"', lambda m: '"Visualization = OMNeT++ (Qtenv)\\n"', l) for l in main]
main = [l for l in main if not re.search(r'<<\s*"\s*\w+-clustering\.xml\\n";', l)]
main = [re.sub(r',\s*\w+-clustering\.xml', '', l) for l in main]
main = [re.sub(r'\b(std::)?cout\b', 'wsn::Out()', l) for l in main]
main = [re.sub(r'\bofstream\s+(\w+)\("([^"]+)"\)', r'ofstream \1(wsn::CsvPath("\2"))', l) for l in main]

# init code right after the opening brace
init = ['    InitSwitches(HOOK);', '    HOOK.SetBaseStation(BSX, BSY);']
brace = next(k for k, l in enumerate(main) if l.strip() == '{')
main[brace + 1:brace + 1] = init

# per-round report right after the round result is computed
rr = [k for k, l in enumerate(main) if re.search(r'\bRoundResult\s+r\s*=', l)]
assert len(rr) == 1, 'expected one RoundResult r in main of ' + src_path
k = rr[0]
while not main[k].rstrip().endswith(';'):
    k += 1
ind = re.match(r'^(\s*)', main[rr[0]]).group(1)
main.insert(k + 1, ind + 'HOOK.Round(round, r, nodes);   // OMNeT++: one round = one step of simulation time')

# final report before the last return
totals = sorted(set(re.findall(r'\b(total[A-Z]\w*)\b', '\n'.join(main))) - {'totalGenerated', 'totalDelivered', 'totalUsed'})
ret = max(k for k, l in enumerate(main) if re.match(r'^\s*return 0;', l))
fin = ['    {   // OMNeT++: final metrics, exactly as computed above',
       '        wsn::FinalView fv;',
       '        fv.FND = fnd ? FND : 0; fv.HND = hnd ? HND : 0; fv.LND = LND;',
       '        fv.pdr = overallPdr; fv.energyUsed = totalUsed;',
       '        fv.generated = totalGenerated; fv.delivered = totalDelivered;',
       '        HOOK.Finish(fv);']
fin += ['        HOOK.Scalar("%s", static_cast<double>(%s));' % (t, t) for t in totals]
fin += ['    }']
main[ret:ret] = fin

# ---------------------------------------------------------------- write
def squeeze(ls):
    out = []
    for l in ls:
        if not l.strip() and out and not out[-1].strip():
            continue
        out.append(l)
    return out


body, main = squeeze(body), squeeze(main)
sw_init = ['static void InitSwitches(wsn::Hook& hook)', '{']
sw_init += ['    %s = static_cast<%s>(hook.Define("%s", %s));' % (n, t, n, v) for n, t, v in switches]
sw_init += ['    ' + r for r in recompute]
sw_init += ['}']

ns = 'wsn_' + name
out = ['// OMNeT++ port of code/%s/%s.cc (thesis ns-3 program).' % (group, name),
       '// Generated by omnetpp/tools/port_ns3.py: the protocol logic below is the',
       '// original code, unchanged; see the header of that script for what was adapted.',
       '',
       '#include "WsnRuntime.h"',
       ''] + includes + ['', 'using namespace std;', '', 'namespace %s {' % ns] + body + sw_init + [''] + main + \
      ['', '} // namespace %s' % ns, '',
       'static wsn::Registrar %s_registrar("%s", "code/%s/%s.cc", &%s::Run, {%s});' %
       (ns, name, group, name, ns, ', '.join('"%s"' % s[0] for s in switches)), '']
open(out_path, 'w', encoding='utf-8').write('\n'.join(out))
print('%-40s switches: %s' % (name, ' '.join(s[0] for s in switches) or '-'))
