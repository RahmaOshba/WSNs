#!/usr/bin/env python3
"""Mean FND / HND / LND / PDR of one configuration, grouped by its iteration
variables (the topology seed is averaged out).   usage: python3 summarize.py SecurityPlan"""
import collections
import glob
import os
import re
import statistics
import sys

config = sys.argv[1]
here = os.path.dirname(os.path.abspath(__file__))
groups = collections.OrderedDict()
for path in sorted(glob.glob(os.path.join(here, 'results', config + '-*.sca'))):
    text = open(path).read()
    if not re.search(r'^attr configname %s$' % re.escape(config), text, re.M):
        continue
    itervars = [(k, v.replace('\\"', '').replace('"', '').strip())
                for k, v in re.findall(r'^itervar (\S+) (.*)$', text, re.M)]
    key = tuple((k, v) for k, v in itervars if k not in ('seed', 'repetition', 'secDefines'))
    vals = {m: float(re.search(r'^scalar \S+\.controller %s (\S+)' % m, text, re.M).group(1))
            for m in ('FND', 'HND', 'LND', 'PDR')}
    groups.setdefault(key, []).append(vals)

print('%-70s %4s %7s %7s %7s %8s' % ('run', 'n', 'FND', 'HND', 'LND', 'PDR %'))
for key, runs in sorted(groups.items()):
    mean = lambda m: statistics.mean(r[m] for r in runs)
    print('%-70s %4d %7.0f %7.0f %7.0f %8.2f' % (' '.join('%s=%s' % kv for kv in key), len(runs),
                                                  mean('FND'), mean('HND'), mean('LND'), 100 * mean('PDR')))
