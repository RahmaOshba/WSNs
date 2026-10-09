# OMNeT++ version of the thesis simulations

The full thesis work, from the first baselines to the proposed protocol, ported from ns-3.41 to
**OMNeT++ 6.4** (plain OMNeT++, no extra framework needed):

| Group | Protocols |
|---|---|
| `1_ORIGINAL` | LEACH, HEED, PEGASIS, SH-LEACH, H-LEACH, EECH-HEED, each in its own paper's settings |
| `2_EDITED` | the same protocols (plus HEED with the fairness penalty) in the unified environment |
| `3_IMPROVED` | SH-LEACH, H-LEACH, EECH-HEED after our fix |
| `4_PROPOSED` | v1 → v8 and v8-Chain, plus the side experiment |
| extra | far BS, v8 ablation (I1–I5), v8-Chain routing modes, robustness over 8 topologies, the 17 security scenarios × 6 |

**Validation:** all **200 runs give exactly the ns-3 FND / HND / LND** (and the same PDR) as in
`results/summary_all.csv`. The full table is in [`VALIDATION.md`](VALIDATION.md).

## How it works

```
WsnNetwork (simulations/omnetpp.ini picks the protocol)
 ├── controller : Controller   runs the protocol; 1 round = 1 s of simulation time
 ├── bs         : BaseStation  the sink
 └── node[N]    : SensorNode   created at deployment; shows energy / role / cluster,
                               records per-node results (death round, rounds as CH)
```

- `src/protocols/<group>/<name>.cc`: the protocol logic, copied unchanged from
  `code/<group>/<name>.cc`. It uses the same first-order radio model, the same random number
  generators and seeds, and the same per-round steps. That is why the results are identical.
- Only the ns-3 glue was replaced (`tools/port_ns3.py`):
  - NetAnim is replaced by the OMNeT++ GUI.
  - The `-D` compile switches become the ini parameter `defines`.
  - After every round the protocol reports to the `Controller`. The Controller advances
    simulation time by one round, updates the node modules and records the statistics.
- `src/WsnRuntime.h` is the small interface between a protocol and OMNeT++.

## 1. Install OMNeT++ 6.4 (Ubuntu 24.04)

Full instructions: `doc/InstallGuide.pdf`, chapter "Ubuntu", inside the OMNeT++ download.

```bash
sudo apt update
sudo apt install -y make diffutils pkg-config ccache clang lld gdb lldb \
    bison flex perl sed gawk python3 python3-pip python3-venv python3-dev \
    libxml2-dev zlib1g-dev doxygen graphviz xdg-utils libdw-dev
sudo apt install -y qt6-base-dev qt6-base-dev-tools qmake6 libqt6svg6 \
    qt6-wayland libwebkit2gtk-4.1-0                     # GUI (Qtenv + IDE)

cd ~
wget https://github.com/omnetpp/omnetpp/releases/download/omnetpp-6.4.0/omnetpp-6.4.0-linux-x86_64.tgz
tar xzf omnetpp-6.4.0-linux-x86_64.tgz
cd omnetpp-6.4.0
python3 -m venv .venv --upgrade-deps --clear --prompt "omnetpp/.venv"
source .venv/bin/activate
python3 -m pip install -r python/requirements.txt
source setenv
./configure WITH_OSG=no          # no 3D needed
make -j$(nproc)
```

Every new terminal needs `source ~/omnetpp-6.4.0/setenv` first. You can add that line to `~/.bashrc`.

## 2. Build

```bash
cd Routing-Protocols/omnetpp
make                 # creates src/wsn
```

## 3. Run

```bash
cd simulations
./run -c v8_chain_center                 # GUI (Qtenv): press Run / Fast, watch the clusters
./run -u Cmdenv -c v8_chain_center       # command line, prints FND/HND/LND at the end
./run_all.sh                             # every configuration (≈ 1 min on 8 cores)
python3 validate.py                      # compare everything with the ns-3 results
python3 summarize.py SecurityPlan        # mean FND/HND/LND/PDR of one config (seeds averaged)
```

In the GUI:
- Every cluster has its own colour, and a CH has a bigger icon.
- Lines go from each member to its CH. A dashed line goes from a CH to the BS.
- Dead nodes are black.
- The label of each node shows its residual energy.
- The controller's label shows the round, the number of alive nodes and the number of CHs.

### Configurations (`simulations/omnetpp.ini`)

| Config | What |
|---|---|
| `leach_ORIGINAL` … `v8_chain_farBS` | one per ns-3 program, same name as `code/<group>/<name>.cc` |
| `FarBS` | 12 protocols with the BS at (50, −100) |
| `Ablation` | v8 without I1 … I5 |
| `Routing` | v8-Chain `CHAIN_MODE` 0 / 1 / 2, centre and far BS |
| `Robustness` | v5b, v8, v8-Chain × 8 topology seeds |
| `Security_LEACH_center` … `Security_v8_far` | the 17 security scenarios (see the comments in the ini) |
| `SecurityPlan`, `SecurityPlanSweep` | the lifetime-saving ECC + AES options of `docs/SECURITY_PLAN.md` |

`**.controller.defines` takes the same switches as the ns-3 `-D` flags (`code/README.md`), for example:

```ini
[Config MyTest]
**.controller.protocol = "v8_chain_center"
**.controller.defines  = "BS_Y=-100 I4=0 SEC_BITS=104 SEC_NJ_PER_BIT=5"
```

A wrong switch name stops the run with the list of switches that protocol has.

### Results

The results are written to `simulations/results/`:

- `*.sca` (scalars):
  - `FND`, `HND`, `LND`, `PDR`, `totalEnergyUsed`, `totalGenerated`, `totalDelivered`.
  - Protocol counters such as `totalHandovers` or `totalRelayed`.
  - Per node: `deathRound`, `roundsAsCH`, `timesElectedCH`, `residualEnergy`.
- `*.vec` (per round): `alive`, `dead`, `chCount`, `residualEnergy`, `energyUsed`,
  `generated`, `delivered`, `pdr`.
  - The per-node `energy` vector is off by default because it is large. Turn it on in the ini.

Open them in the OMNeT++ IDE (Analysis tool), or export them:

```bash
opp_scavetool export -f 'name =~ FND OR name =~ HND OR name =~ LND OR name =~ PDR' -o summary.csv results/*.sca
opp_scavetool export -f 'name =~ alive' -o alive.csv results/v8_chain_center-*.vec
```

To get the same CSV files the ns-3 programs wrote (`*-results.csv`, `*-node-energy.csv`,
`*-node-lifetime.csv`), uncomment `**.controller.csvDir` in the ini.

## Adding a new protocol version

1. Write it as an ns-3-style program in `code/`, the same way as the existing ones.
2. Port it:

   ```bash
   python3 omnetpp/tools/port_ns3.py code/4_PROPOSED/v9_x.cc omnetpp/src/protocols/4_PROPOSED/v9_x.cc 4_PROPOSED
   ```

3. Add a `[Config v9_x]` with `**.controller.protocol = "v9_x"` to the ini.
4. Run `make`.

You can also write the engine directly. The only requirement is a `Run(wsn::Hook&)` function that:
- calls `HOOK.Round(...)` once per round;
- calls `HOOK.Finish(...)` at the end;
- is registered with `wsn::Registrar` (see the end of any file in `src/protocols`).

## Scope

This is the same analytical model as the thesis: first-order radio energy, an ideal channel
and a TDMA schedule. It is not a packet-level PHY/MAC simulation. That is exactly what makes
the two simulators agree to the round. A packet-level layer (real frames between the node
modules over a lossy channel) can be added on top of this structure later without changing
the protocol logic.
