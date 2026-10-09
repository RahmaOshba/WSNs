# Packet-level results: LEACH and v8 / v8-Chain over IEEE 802.15.4 (INET)

These runs address thesis limitation 1, the analytical radio model.
- Same 100 nodes, the same positions (`mt19937(12345)`), and 0.5 J per node.
- Every message is a real 802.15.4 frame (CSMA/CA, ACKs, retransmissions, collisions, fragmentation).
- Energy is charged for every frame the radio really sends or receives.

Reproduce with `simulations/run_all.sh` followed by `python3 simulations/compare.py`.

## 1. Main comparison (default settings: 6LoWPAN header, global TDMA slots, 1.5 s JOIN window)

| Protocol | FND packet / analytical | HND packet / analytical | LND packet / analytical | PDR packet / analytical |
|---|---|---|---|---|
| LEACH, BS centre | 897 / 1383 (−35%) | 1051 / 1586 (−34%) | 1221 / 1842 (−34%) | 99.5 / 99.4 % |
| v8, BS centre | 1711 / 2446 (−30%) | 1805 / 2536 (−29%) | 1851 / 2566 (−28%) | 99.8 / 99.7 % |
| v8-Chain, BS centre | 1711 / 2446 (−30%) | 1805 / 2536 (−29%) | 1851 / 2566 (−28%) | 99.8 / 99.7 % |
| LEACH, BS far (50, −100) | 599 / 988 (−39%) | 767 / 1230 (−38%) | 1141 / 1652 (−31%) | 99.5 / 99.4 % |
| v8, BS far | 916 / 1406 (−35%) | 1071 / 1551 (−31%) | 1141 / 1606 (−29%) | 99.5 / 99.4 % |
| v8-Chain, BS far | 1061 / 1626 (−35%) | 1221 / 1821 (−33%) | 1311 / 1881 (−30%) | 99.4 / 99.6 % |

With the BS at the centre, v8 and v8-Chain are identical. The relay is never cheaper than the direct CH → BS link, which is also true in the analytical model.

### What holds

- **The ranking is unchanged.** At the centre: v8 = v8-Chain > LEACH. With the far BS: v8-Chain > v8 > LEACH.
- **v8's advantage grows at packet level.**

  | Comparison | Packet level | Analytical |
  |---|---|---|
  | v8 over LEACH, FND, centre | **+91%** | +77% |
  | v8-Chain over LEACH, FND, far BS | **+77%** | +65% |

  LEACH loses more because it re-clusters every round. That means more ADV, JOIN and SCHED frames, each with headers and ACKs. v8 keeps its clusters for 5 rounds.
- **PDR stays at about 99.5%.** CSMA/CA retransmissions recover almost every collision. The MAC gives up on only a few dozen frames per run (`macRetryLimitDrops`, `macBackoffDrops`).

### Why every lifetime is about 30–39% shorter

The analytical model charges only the payload. A real radio sends more:

| Cost the analytical model does not have | Effect |
|---|---|
| Headers: MAC header and FCS (9 B), PHY header (6 B), compressed 6LoWPAN header (6 B) | a 200-bit control message becomes about 370 bits on the air |
| Fragmentation: 127 B maximum frame size | a 2000-bit reading becomes 3 frames, each with its own headers and ACK |
| ACKs for every unicast frame, and retransmissions after a collision | extra TX for the receiver, extra RX for the sender |

The loss is similar for all protocols (−30% to −39%), so it comes from the radio, not from the protocol logic.

## 2. JOIN contention (`ShortJoinWindow`)

The JOINs of one set-up phase are sent with CSMA/CA inside a JOIN window.
- A member whose JOIN is lost is not in the TDMA schedule.
- Following the unified rule (I4), it sends its readings directly to the BS.

| JOIN window | LEACH centre | v8 centre | v8-Chain centre | LEACH far | v8 far | v8-Chain far |
|---|---|---|---|---|---|---|
| 1.5 s (default) | 897 | 1711 | 1711 | 599 | 916 | 1061 |
| 0.4 s (first run, being re-run with the final code) | 922 | 1741 | 1741 | 419 | 391 | 461 |

- With a 0.4 s window, about 11% of the JOINs collide.
- At the centre this costs little, because the BS is close.
- With the far BS, the direct transmissions are long and expensive (multipath, d⁴), so the lifetime collapses.

This effect cannot be seen in the analytical model. It points to a possible v8 improvement (I6): a member whose JOIN was lost sends to the nearest CH it heard instead of the BS.

## 3. Parallel cluster slots (`ParallelSlots`)

The LEACH paper gives each cluster its own CDMA code, so all clusters can use their TDMA slots at the same time. An 802.15.4 network has one shared channel. Here all clusters use their slots in parallel on that one channel:

| Slots | LEACH centre FND / PDR | v8 centre FND / PDR | v8-Chain centre FND / PDR |
|---|---|---|---|
| global (default) | 897 / 99.5 % | 1711 / 99.8 % | 1711 / 99.8 % |
| parallel per cluster | running | running | running |

The results of this configuration will be added when the runs finish.

## 4. Limits of this step

- **One topology (seed 12345).** More topologies, node counts and field sizes are the next step (limitation 2).
- **Idle listening is not charged**, as in the thesis. The radio sleeps outside its slots. The energy is the first-order model applied to the real frames, not INET's battery model.
- **The protocol logic runs in one `Brain` module.** The decisions use only information a node could have: the ADVs it heard, the JOINs that arrived, the beacon. Every message that carries this information is a real frame.
- **Security is not yet included.** It is the next step on top of this packet-level layer (see `docs/SECURITY_PLAN.md`, E4).
