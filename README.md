# energy-efficient-wsn-clustering

Energy-efficient clustering for Wireless Sensor Networks in ns-3.41 and OMNeT++ 6.4: LEACH, HEED,
PEGASIS and three LEACH + HEED hybrids (SH-LEACH, H-LEACH, EECH-HEED) reproduced
and compared in one fair environment, and the proposed protocol **v8 / v8-Chain**.

Master's thesis — Eng. Rahma Khaled Oshba: *Energy-Efficient Secure Clustering in
Wireless Sensor Networks Using Hybrid Cryptography*.

| Folder | Content |
|---|---|
| `code/` | All simulation codes, split into `1_ORIGINAL`, `2_EDITED`, `3_IMPROVED`, `4_PROPOSED` (see `code/README.md`) |
| `omnetpp/` | The same work in **OMNeT++ 6.4**: every protocol and experiment, validated run-by-run against ns-3 (see `omnetpp/README.md`) |
| `results/` | The output of every code, in the same folders (see `results/README.md`) |
| `papers/` | The research papers, split into `1_ORIGINAL`, `2_HYBRID`, `3_RECENT` (see `papers/README.md`) |
| `figures/` | Screenshots and diagrams |
| `thesis-final/` | Defense deck, State-of-the-Art report, Literature Review (one Excel sheet), Results Comparison workbook and Simulation Parameters Guide — all built from `results/` (see `thesis-final/build/README.md`) |

Headline result (unified environment, BS at the centre): **v8 FND 2446 /
HND 2536 / LND 2566 rounds, PDR 99.72%**; with the BS far away **v8-Chain FND 1626**.
The best reproduced protocol, SH-LEACH Improved, reaches FND 1480.
