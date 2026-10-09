# Packet-level runs: the thesis protocols over real IEEE 802.15.4 (INET)

The main OMNeT++ port (`../src`) uses the same analytical model as the thesis:
- energy from the first-order radio model;
- an ideal channel;
- no MAC.

That was limitation 1 of the thesis. This project runs **LEACH** and **v8 / v8-Chain** with
**every message as a real frame** over INET's IEEE 802.15.4:
- CSMA/CA MAC with ACKs and retransmissions;
- 250 kb/s, one shared channel, interference and collisions;
- UDP/IPv4.

## What is the same as the thesis

- **Deployment:** 100 nodes, 100 × 100 m, 0.5 J, `mt19937(12345)`. The positions are identical to the analytical runs.
- **BS:** at the centre or at (50, −100).
- **Protocol decisions:** LEACH: `code/2_EDITED/leach_EDITED.cc`. v8: `code/4_PROPOSED/v8_chain_center.cc`. They are reimplemented in `src/Brain.cc`, step by step:
  - the threshold and G-set;
  - the single-pass score, I1–I5, backup CH, handover, orphan re-join;
  - the energy-aware relay.
- **Energy:** the same first-order radio model, 2000-bit readings, 200-bit control frames.

## What is new (packet level)

| Analytical model (thesis) | Packet level (this project) |
|---|---|
| A node joins the nearest CH | A node joins the nearest CH **whose ADV it actually received** |
| The CH knows its members | The CH knows the members **whose JOIN arrived** |
| Every reading sent to the CH arrives | A reading counts only if the frame **reaches the CH / BS** |
| Energy = TX + RX of one 2000-bit packet | Energy of **every frame on the air**, from the radio: MAC header and FCS (9 B), PHY header (6 B), network header, **ACKs**, **retransmissions** |
| A 2000-bit packet | **3 IEEE 802.15.4 frames**: max 127 B per frame, so a 250-byte reading is fragmented; losing one fragment loses the reading |
| Control message = 200 bits | 200 bits + the headers above: about 370 bits on the air |
| No time | TDMA rounds in simulation time: set-up (ADV, JOIN with CSMA, schedule), one data slot per node, CH slots |

**Network headers.** The frames are carried over UDP/IPv4, which is how INET delivers them.
- `headerModel = "6lowpan"` (default) charges the IPv4 + UDP headers as a 6LoWPAN compressed header (6 B), which is what real 802.15.4 sensor networks use.
- `headerModel = "ipv4"` charges the full 28 B.

**TDMA sleep.** As LEACH and v8 assume, a node's radio sleeps when the node has nothing to send or
receive. The MAC wakes it to send, and it wakes 5 ms before its slot. Only the frames a node is
meant to hear are charged. Idle listening is not charged, as in the thesis model.

**One shared channel.** The default (`slotMode = "global"`) gives every node its own data slot.
`ParallelSlots` lets all clusters use their slots at the same time, as the LEACH paper does with
one CDMA code per cluster. On one 802.15.4 channel this causes contention.

## Results

Packet level vs. the analytical model (FND):

| | LEACH | v8 | v8-Chain |
|---|---|---|---|
| BS centre | 897 / 1383 | 1711 / 2446 | 1711 / 2446 |
| BS far | 599 / 988 | 916 / 1406 | 1061 / 1626 |

- Every lifetime is 30–39% shorter, because of the headers, fragmentation, ACKs and retransmissions.
- The ranking is unchanged, and v8's advantage over LEACH grows.
- Details, JOIN contention and parallel slots: [`RESULTS.md`](RESULTS.md).

## Build and run

Needs OMNeT++ 6.4 and INET 4.6 (see `../README.md`).

```bash
source ~/omnetpp-6.4.0/setenv
source ~/inet4.6/setenv
cd ~/WSNs/omnetpp/packet-level
make
cd simulations
./run -c v8_chain_center              # GUI
./run -u Cmdenv -c v8_chain_center    # command line
./run_all.sh                          # the 6 configurations (≈ 15–25 min, in parallel)
python3 compare.py                    # packet level vs. the analytical results of the thesis
```

| Config | What |
|---|---|
| `leach_center`, `v8_center`, `v8_chain_center` | BS at (50, 50) |
| `leach_farBS`, `v8_farBS`, `v8_chain_farBS` | BS at (50, −100) |
| `ParallelSlots` | the 3 protocols with parallel cluster slots (contention between clusters) |

Results in `results/*.sca` (brain module):
- `FND`, `HND`, `LND`, `PDR`;
- `macRetryLimitDrops`, `macBackoffDrops`: frames the MAC gave up;
- `corruptedFrames`: collisions seen;
- `handovers`, `failovers`, `directToBS`, `relayed`;
- per round: `alive`, `chCount`, `pdr`, `residualEnergy`.
