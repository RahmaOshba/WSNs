#!/bin/sh
# Run every configuration of omnetpp.ini in Cmdenv (parallel), results in results/
cd "$(dirname "$0")" || exit 1
JOBS=${JOBS:-$(nproc)}
CONFIGS=$(../src/wsn -n ../src:. -s -a 2>/dev/null | sed -n 's/^Config \([A-Za-z0-9_]*\):.*/\1/p')
for c in $CONFIGS; do
    case "$c" in General|SecurityBase) continue ;; esac
    opp_runall -j"$JOBS" ../src/wsn -n ../src:. -u Cmdenv -c "$c" > /dev/null || echo "FAILED: $c"
done
echo "done -- now run: python3 validate.py"
