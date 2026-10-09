#!/usr/bin/env python3
"""Packet level (INET, IEEE 802.15.4) vs. analytical model (thesis, results/summary_all.csv)."""
import csv
import glob
import os
import re

here = os.path.dirname(os.path.abspath(__file__))
ref = {}
with open(os.path.join(here, '..', '..', '..', 'results', 'summary_all.csv')) as f:
    for row in csv.DictReader(f):
        ref[row['run']] = row

# packet-level config -> analytical run of the thesis
PAIRS = [('leach_center', 'leach_EDITED'), ('v8_center', 'v8_center'), ('v8_chain_center', 'v8_chain_center'),
         ('leach_farBS', 'leach_EDITED_farBS'), ('v8_farBS', 'v8_farBS'), ('v8_chain_farBS', 'v8_chain_farBS')]

def scalars(config):
    out = {}
    for path in glob.glob(os.path.join(here, 'results', config + '-*.sca')):
        for line in open(path):
            m = re.match(r'^scalar \S+\.brain (\S+) (\S+)', line)
            if m:
                out[m.group(1)] = float(m.group(2))
    return out

print('%-17s | %-24s | %-24s | %-24s | %-15s | %s' % ('protocol', 'FND  packet / analytical', 'HND  packet / analytical',
                                                       'LND  packet / analytical', 'PDR pkt / ana', 'MAC retry / backoff drops'))
print('-' * 140)
for cfg, run in PAIRS:
    s, r = scalars(cfg), ref.get(run)
    if not s or not r:
        print('%-17s | (not run yet)' % cfg)
        continue
    def cell(m):
        p, a = s[m], float(r[m])
        return '%5d / %5d  (%+5.1f%%)' % (p, a, 100 * (p - a) / a if a else 0)
    print('%-17s | %-24s | %-24s | %-24s | %5.1f%% / %5.1f%% | %d / %d' % (
        cfg, cell('FND'), cell('HND'), cell('LND'), 100 * s['PDR'], 100 * float(r['PDR']),
        s.get('macRetryLimitDrops', 0), s.get('macBackoffDrops', 0)))
