#!/usr/bin/env python3
"""Compare the OMNeT++ results (results/*.sca) with the ns-3 results of the thesis
(../../results/summary_all.csv): FND, HND, LND must be identical, PDR equal to the
precision stored in the summary."""
import csv
import glob
import os
import re
import sys

here = os.path.dirname(os.path.abspath(__file__))
ref = {}
with open(os.path.join(here, '..', '..', 'results', 'summary_all.csv')) as f:
    for row in csv.DictReader(f):
        ref[row['run']] = row

SEC_PREFIX = {'Security_LEACH_center': 'sec_leach_center', 'Security_LEACH_far': 'sec_leach_far',
              'Security_PEGASIS_center': 'sec_pegasis_center', 'Security_PEGASIS_far': 'sec_pegasis_far',
              'Security_v8_center': 'sec_v8_center', 'Security_v8_far': 'sec_v8chain_far'}
ROBUST = {'v5b_energy_aware_repair': 'v5b_center'}


def key(config, itervars):
    if config == 'FarBS':
        return itervars['protocol'] + '_farBS'
    if config == 'Ablation':
        return 'v8_without_' + itervars['without']
    if config == 'Routing':
        place = 'center' if itervars['place'] == 'v8_chain_center' else 'farBS'
        return '%s_mode%s' % (place, itervars['mode'])
    if config == 'Robustness':
        p = itervars['protocol']
        return '%s_seed%s' % (ROBUST.get(p, p), itervars['seed'])
    if config in SEC_PREFIX:
        return SEC_PREFIX[config] + '_' + itervars['scenario']
    return config


rows, ok, bad, missing = [], 0, 0, 0
for path in sorted(glob.glob(os.path.join(here, 'results', '*.sca'))):
    config, itervars, sc = None, {}, {}
    for line in open(path):
        m = re.match(r'^attr configname (\S+)', line) or re.match(r'^config configname (\S+)', line)
        if m:
            config = m.group(1)
        m = re.match(r'^itervar (\S+) (.*)$', line)
        if m:
            itervars[m.group(1)] = m.group(2).replace('\\"', '').replace('"', '').strip()
        m = re.match(r'^scalar \S+\.controller (FND|HND|LND|PDR) (\S+)', line)
        if m:
            sc[m.group(1)] = float(m.group(2))
    if not sc:
        continue
    k = key(config, itervars)
    r = ref.get(k)
    if r is None:
        missing += 1
        rows.append((k, sc, None, 'no reference'))
        continue
    pdr_ref = float(r['PDR'])
    # the summary stores some PDRs with fewer decimals (e.g. 0.994000 = 99.40 %)
    digits = max(4, len(r['PDR'].split('.')[1].rstrip('0'))) if '.' in r['PDR'] else 0
    same = all(int(sc[m]) == int(r[m]) for m in ('FND', 'HND', 'LND')) and \
        abs(round(sc['PDR'], digits) - pdr_ref) <= 0.6 * 10 ** -digits
    ok += same
    bad += not same
    rows.append((k, sc, r, 'OK' if same else 'DIFFERENT'))

print('%-45s %15s %15s %15s %10s  %s' % ('run', 'FND omnet/ns3', 'HND', 'LND', 'PDR', ''))
for k, sc, r, status in rows:
    f = lambda m: '%d/%s' % (sc[m], r[m] if r else '-')
    print('%-45s %15s %15s %15s %10.6f  %s' % (k, f('FND'), f('HND'), f('LND'), sc['PDR'], status))
print('\n%d runs identical to ns-3, %d different, %d without reference' % (ok, bad, missing))
sys.exit(1 if bad else 0)
