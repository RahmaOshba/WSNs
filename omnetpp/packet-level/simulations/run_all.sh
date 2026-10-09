#!/bin/sh
# Run the packet-level configurations (Cmdenv, in parallel); results in results/
cd "$(dirname "$0")" || exit 1
for c in leach_center v8_center v8_chain_center leach_farBS v8_farBS v8_chain_farBS; do
    ./run -u Cmdenv -c "$c" > "results-$c.log" 2>&1 &
done
wait
echo "done -- now run: python3 compare.py"
