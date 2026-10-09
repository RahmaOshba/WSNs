# Security plan: ECC + AES for v8 / v8-Chain while preserving the network lifetime

The goal is to protect the data and the clustering control messages of v8-Chain with hybrid
cryptography (ECC for key agreement, AES for everything else), with a lifetime loss that is as small
as possible. With every lifetime-saving step applied, the full ECC + AES protocol keeps about
**92%** of the unsecured FND, and still outlives every unsecured baseline and hybrid.

All numbers come from the OMNeT++ port (`omnetpp/`, configs `SecurityPlan` and
`SecurityPlanSweep`; `python3 summarize.py SecurityPlan`). They are means over the 8 robustness
topologies, with 100 nodes, 0.5 J per node, and the BS at the centre (50,50) or far (50,−100).

## 1. What must be protected (threat model)

| Threat | Where in v8-Chain | Protection |
|---|---|---|
| Eavesdropping on sensor data | member → CH, CH → (relay CH) → BS | AES-128-CCM* encryption |
| Injected or modified frames | every frame | CCM* MIC (message integrity code) |
| Replay of old frames | every frame | nonce / frame counter (replay window) |
| **Fake BS beacon** (the v8 average-energy beacon) | I5 energy gate: a forged average blocks all elections or lets weak nodes become CH | the beacon is authenticated by the BS (broadcast key with µTESLA-style key disclosure) |
| Fake CH advertisement (sinkhole) or HELLO flood | election and join, neighbour discovery | the CH proves it was authorised by the BS (MAC under its node key); HELLO under the network key |
| Fake relay / selective forwarding | v8-Chain CH-to-CH relay | data stays end-to-end encrypted CH → BS; the relay forwards without decrypting; the BS checks delivery per round |
| Node capture | any node | per-node keys, so one captured node does not expose the others; the cluster key is refreshed at every re-clustering (every 5 rounds) |

Out of scope for this stage: an insider that lies about its own energy, jamming, and physical attacks.

## 2. The scheme

| Phase | Operation | Cryptography | When |
|---|---|---|---|
| Deployment | each node does one key agreement with the BS → node key K_i | **ECC once**: X25519 (RFC 7748) or P-256 ECDH, then HKDF (RFC 5869) | once per node lifetime |
| Deployment | the network key K_net (HELLO, beacon) is delivered wrapped under K_i | AES Key Wrap (NIST SP 800-38F) | once |
| Every re-clustering (5 rounds) | the BS beacon is authenticated | AES-CMAC (SP 800-38B) / µTESLA | every set-up |
| Every re-clustering | the new CH is authorised by the BS; the BS sends each member the cluster key K_c | AES-CMAC + AES Key Wrap (`SEC_KEYDIST=1`) | every set-up |
| Every round | member → CH | AES-128-CCM* with K_c, **MIC-32** | every frame |
| Every round | CH (after fusion) → relay CH → BS | AES-128-CCM* with the CH's K_i (end-to-end) | one frame per cluster |

Public-key cryptography is used **only once per node**. Everything that repeats is symmetric.
This is the hybrid design of the thesis proposal.

## 3. What costs lifetime, and how each step recovers it

Measured on v8-Chain, mean of 8 topologies:

| Option | Per-frame overhead | ECC once | Centre FND | Far FND | PDR |
|---|---|---|---:|---:|---:|
| P0 no security | – | – | **2448** | **1652** | 99.7 / 99.5 % |
| P1 AES only (pre-loaded keys) | 104 bit, 5 nJ/bit | – | 2054 (−16%) | 1393 (−16%) | 99.8 / 99.5 % |
| P2 scheme as presented at the defense | 104 bit (MIC-64) | 48 mJ (software X25519) | 1829 (**−25%**) | 1232 (**−25%**) | 99.7 / 99.5 % |
| P3 + hardware ECC + MIC-32 | 72 bit | 5 mJ | 2056 (−16%) | 1411 (−15%) | 99.8 / 99.5 % |
| P4 + implicit nonce (round number) | 40 bit | 5 mJ | 2116 (−14%) | 1437 (−13%) | 99.8 / 99.5 % |
| P5 + low-power hardware AES (1 nJ/bit) | 40 bit, 1 nJ/bit | 5 mJ | **2262 (−8%)** | **1525 (−8%)** | 99.8 / 99.6 % |

Reference points without any security: unsecured LEACH has FND 1383 (centre) and 988 (far). The best
unsecured hybrid (SH-LEACH Improved) has FND 1480 at the centre, and EECH-HEED Improved has 1410 far.

The three lifetime-saving steps:

1. **ECC only once, on the hardware accelerator.**
   - A software X25519 costs ≈ 48 mJ. That is about 10% of a 0.5 J node, so it alone costs about 10% of the lifetime (`SecurityPlanSweep`: FND falls almost linearly with the one-time ECC energy).
   - Current IoT MCUs (nRF52840 CryptoCell, CC26x2, EFM32) run ECC in hardware. Hardware crypto outperforms software by well over 100%, which matters for node lifetime (Kietzmann et al., EWSN 2021).
   - We keep the cost as a parameter (`SEC_SETUP_MJ`). We will measure it on a real board (step E3 below) instead of assuming it.
   - Running ECC for every new CH (FND 1101) or for every packet (FND 2–44) destroys the lifetime, so these are rejected.

2. **Shorter security fields.** Every extra bit is paid at E_elec on every transmission and reception.
   - MIC-32 instead of MIC-64 is a standard IEEE 802.15.4 security level (ENC-MIC-32).
   - The **implicit nonce** removes the 4-byte frame counter. The CCM* nonce is built from the node ID and the round/slot number, which every node already knows from the TDMA schedule. IEEE 802.15.4e TSCH does the same with the Absolute Slot Number.
   - Together the overhead drops from 104 to 40 bits per frame. This is a protocol-specific saving, because v8 rounds are synchronised.

3. **Low-power hardware AES.**
   - The AES engines of the radio/MCU cost far less than software. Software AES at 50 nJ/bit gives FND 1061.
   - Kietzmann et al. measured 0.25–4 µJ per short AES input on the nRF52840 and EFM32 accelerators.
   - At 1 nJ/bit, the per-frame cipher cost becomes small compared with E_elec = 50 nJ/bit.

Two more points:

- **Key distribution costs almost nothing in v8.** v8 reuses the clusters for 5 rounds, so the key frame is paid only once per set-up (`SEC_KEYDIST=1` alone: FND 2446 vs 2446).
- **The energy-aware relay already counts the crypto cost.** The v8-Chain relay rule and the direct-to-BS rule add the security energy to the cost of each path (code switch `SEC_PK_BS_MJ`). Routing therefore adapts by itself when the cost of reaching the sink grows.

## 4. Experiments to do next

| Step | What | How (OMNeT++) |
|---|---|---|
| E1 (done) | energy cost of each design option | `SecurityPlan`, `SecurityPlanSweep` |
| E2 | add the implicit-nonce and MIC-32 options as named switches in the code, with the CCM* frame format documented | small change in `v8_chain_*.cc` + port |
| E3 | measure the real ECC / AES energy on a board (nRF52840 + Power Profiler Kit, or CC2652) and replace the assumed mJ / nJ values | lab measurement → `SEC_SETUP_MJ`, `SEC_NJ_PER_BIT` |
| E4 | **attack simulation**: packet-level frames between the node modules plus an attacker module (fake beacon, fake CH, replay, selective-forwarding relay). Measure PDR and lifetime under attack, with and without the scheme | new modules on top of `omnetpp/` |
| E5 | compare with published secure clustering protocols (lifetime cost of their security) | literature + same environment |

## 5. Standards and sources

- IEEE 802.15.4-2020: CCM* security levels (MIC-32/64/128). TSCH uses the ASN in the nonce.
- RFC 7748 (X25519). RFC 5869 (HKDF). NIST SP 800-56A (ECDH). SP 800-38B (CMAC), 38C (CCM), 38F (Key Wrap).
- Wander, Gura, Eberle, Gupta, Shantz, "Energy analysis of public-key cryptography for wireless sensor networks", PerCom 2005. Source of the ECC/RSA energies of the thesis scenarios.
- Kietzmann, Boeckmann, Lanzieri, Schmidt, Wählisch, "A Performance Study of Crypto-Hardware in the Low-end IoT", EWSN 2021 ([IACR ePrint 2021/058](https://eprint.iacr.org/2021/058.pdf)).
- Perrig et al., "SPINS: Security Protocols for Sensor Networks" (µTESLA), Wireless Networks 2002.
- Zhu, Setia, Jajodia, "LEAP+: Efficient security mechanisms for large-scale distributed sensor networks", ACM TOSN 2006 (per-node, cluster and network keys).
- Earlier scenario results (17 scenarios × LEACH / PEGASIS / v8): `results/README.md`, folder `results/4_PROPOSED/extra/security_proposal/`.

Note: the 0.5 J battery is the usual simulation convention. With real batteries (kJ), the one-time
ECC cost becomes negligible and the per-frame overhead (step 2) is what matters.
