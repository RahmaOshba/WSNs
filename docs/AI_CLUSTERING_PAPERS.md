# AI for cluster-head selection and cluster formation: papers and how they can improve v8

Requested by the supervisor after the defense: collect AI-based work (reinforcement learning, deep
RL, fuzzy logic, swarm intelligence, neural networks / GNN) on CH selection and cluster formation,
and use it to improve the results of the proposed protocol (v8 / v8-Chain).

How to read the table:
- **Result** is what the authors report, each in its own simulation set-up. "(abstract)" means we checked the abstract or summary, not the full text. The numbers are not comparable with each other or with ours until they are re-implemented in our unified environment.
- **Weakness** is our reading.
- **For v8** says whether and how the idea fits our protocol:
  - ★★★ = strong candidate.
  - ★★ = useful as a part or as a baseline to compare against.
  - ★ = background only.
- Venue or year marked "(preprint)" means the paper is not yet confirmed as peer-reviewed.

## 1. Reinforcement learning (tabular Q-learning / SARSA)

| # | Paper | Idea | Result (authors) | Weakness | For v8 |
|---|---|---|---|---|---|
| 1 | Förster & Murphy, **CLIQUE: Role-free clustering with Q-learning for WSNs**, IEEE ICDCS 2009 ([pdf](https://es-static.fbk.eu/people/murphy/Papers/icdcs09.pdf)) | Each node learns with Q-learning whether to act as CH, per packet. There is no election phase. | Up to 25% less energy than random CH selection | Small networks; exploration costs energy; no LND / FND study | ★ the idea of "no election messages" is close to v8's single-pass score |
| 2 | Jurado-Lasso, Jurado, Fafoutis, **LEACH-RLC: Enhancing IoT data transmission with optimized clustering and reinforcement learning**, arXiv 2401.15767 (2024, v2 2025) (preprint) ([arXiv](https://arxiv.org/abs/2401.15767)) | The BS picks CHs and members with MILP. An **RL agent at the BS learns when to re-cluster**. | Longer lifetime, less energy and less control overhead than LEACH and LEACH-C | Centralised: the BS needs the state of every node; the MILP is heavy for large networks (at the BS only) | ★★★ **v8 re-clusters every fixed 5 rounds**; an RL agent can choose the interval adaptively (see idea A) |
| 3 | **Q-learning-based CH selection with hard-cap enforcement**, Wireless Personal Communications (Springer) 2026 ([link](https://link.springer.com/article/10.1007/s11277-026-12229-4)) | ε-greedy Q-table nomination plus a hard cap of 5 CHs per round | Beats LEACH, SEP, HEED and Fuzzy-CHEF on HND and PDR | FND not significantly better than HEED (p = 0.067) | ★★ reward design; v8 already controls the CH count with the epoch / G-set |
| 4 | **Game-theoretic and RL-based CH selection for energy-efficient WSN**, arXiv 2508.12707 (2025) (preprint) ([arXiv](https://arxiv.org/pdf/2508.12707)) | Multistage clustering plus multi-agent RL; the node with the most battery in each sub-cluster becomes CH | Improvement over the authors' baselines (preprint) | Mostly single-hop; game model adds messages | ★ |
| 5 | **An unequal clustering and multi-hop routing protocol based on fuzzy logic and Q-learning in WSNs** (open access, 2025) ([PMC](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC11854616/)) | Fuzzy logic for CH election and cluster radius; Q-learning for the CH-to-BS multi-hop route | Improvement over the authors' baselines (abstract) | Two learning blocks to tune | ★★ the Q-learning relay is a learned alternative to the v8-Chain energy rule (idea D) |
| 6 | **Multi-faceted RL frameworks for dynamic CH selection and energy-efficient routing** (LO-SARSA + policy gradient), SN Computer Science 2025 ([link](https://link.springer.com/article/10.1007/s42979-025-04031-z)) | SARSA for CH selection, policy gradient for routing | Improvement over the authors' baselines (abstract) | Heavier learning; little detail on overhead | ★ |

## 2. Deep reinforcement learning

| # | Paper | Idea | Result (authors) | Weakness | For v8 |
|---|---|---|---|---|---|
| 7 | **DS-HHP: two-layer hierarchical routing with dual-hexagonal topology** (DQN CH selection + SARSA routing), Scientific Reports 2026 ([link](https://www.nature.com/articles/s41598-026-50641-9)) | Each node is a DQN agent. State = residual energy, distance to BS, local density, rounds since last CH. | Better energy balance and lifetime than the authors' baselines (abstract) | A DQN on every node needs memory, computation and training | ★★ **the state is almost the v8 score inputs**; train at the BS / offline and give nodes a small table (idea B) |
| 8 | **QPSODRL: quantum PSO and deep RL clustering and routing** (open access, 2026) ([PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC12886828/)) | QPSO forms clusters; D3QN (dueling double DQN) routes | Improvement over the authors' baselines (abstract) | Two heavy components | ★ |
| 9 | Wang, Chen, Hu, Fan, **A distributed cluster-based routing protocol using fuzzy logic and deep RL for WSNs**, Cluster Computing 28(8):526, 2025 | Fuzzy CH election plus DRL routing | Improvement over the authors' baselines (not checked in full text) | DRL training cost | ★★ |

## 3. Fuzzy logic

| # | Paper | Idea | Result (authors) | Weakness | For v8 |
|---|---|---|---|---|---|
| 10 | Gupta, Riordan, Sampalli, **Cluster-head election using fuzzy logic for WSNs**, CNSR 2005 | **The BS elects the CHs** with fuzzy rules on energy, concentration and centrality | Longer lifetime than LEACH | The BS needs every node's information every round | ★★ v8 already sends each node's energy to the BS (16-bit field), so a BS-side fuzzy score costs no extra messages |
| 11 | Kim et al., **CHEF: cluster head election using fuzzy logic**, 2008 | Distributed fuzzy (residual energy, local distance) | Better than LEACH | Random number → CH count can fall below optimal | ★ (a baseline used by many papers) |
| 12 | Bagci & Yazici, **An energy aware fuzzy approach to unequal clustering in WSNs (EAUCF)**, Applied Soft Computing 13(4):1741–1749, 2013, doi:10.1016/j.asoc.2012.12.029 | Fuzzy competition radius: **smaller clusters near the BS** and for low-energy CHs (hot-spot problem) | Beats LEACH, CHEF, EEUC on FND and HND | Needs radius tuning; CH-to-BS multi-hop | ★★★ **for the far-BS case of v8-Chain**: CHs that relay for others should have smaller clusters (idea C) |
| 13 | Sert, Bagci, Yazici, **MOFCA: multi-objective fuzzy clustering algorithm for WSNs**, Applied Soft Computing 30, 2015 | Multi-objective fuzzy unequal clustering | Improvement over the authors' fuzzy baselines | More parameters | ★ |
| 14 | **Energy-efficient fuzzy-logic-based clustering technique for hierarchical routing protocols in WSNs (FL-EEC/D)** (open access) ([PMC](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC6387035/)) | Five fuzzy descriptors (energy, location, density, compactness, BS distance); Gini index for energy balance | Better lifetime and energy balance than fuzzy election, k-means and LEACH | Rule base grows with the descriptors | ★★ the **Gini index** is a good extra metric for our results |

## 4. Swarm intelligence and metaheuristics

| # | Paper | Idea | Result (authors) | Weakness | For v8 |
|---|---|---|---|---|---|
| 15 | Rao, Jana, Banka, **A PSO based energy efficient cluster head selection algorithm for WSNs (PSO-ECHS)**, Wireless Networks 2017, doi:10.1007/s11276-016-1270-7 | PSO at the BS; fitness = intra-cluster distance, sink distance, residual energy | Improvement over the authors' baselines (abstract) | Centralised; the PSO runs every round | ★★★ strong BS-side baseline; same fitness can tune the v8 score weights (idea B) |
| 16 | **Energy-efficient CH selection using an improved grey wolf optimizer (EECHIGWO)**, Computers (MDPI) 12(2):35, 2023, doi:10.3390/computers12020035 | Improved GWO with sink distance, residual energy, CH-balancing factor and intra-cluster distance | Fewer dead nodes and more rounds than the authors' baselines (abstract) | Centralised; premature convergence needs extra operators | ★★ |
| 17 | Kaddi et al., **Energy-efficient clustering in WSNs using grey wolf optimization and enhanced CSMA/CA**, Sensors 24(16):5234, 2024, doi:10.3390/s24165234 | GWO CH selection plus a MAC improvement | Improvement over the authors' baselines (abstract) | Mixes MAC and clustering gains | ★ |
| 18 | **Energy efficient multi-criterion binary GWO based clustering for heterogeneous WSNs**, Soft Computing 2023 ([link](https://link.springer.com/article/10.1007/s00500-023-09316-0)) | Five objectives (CH energy, compactness, separation, ...) | Stability period +56% | Heterogeneous set-up | ★ |
| 19 | **Energy–lifetime aware CH selection using an adaptive hybrid GWO–PSO framework**, Springer 2026 ([link](https://link.springer.com/article/10.1007/s10791-026-10254-2)) | Uses GWO when energy is high and PSO when energy is low | Better than GWO-ABC, PSO-DE, QPSO-Fuzzy | Many hyper-parameters | ★ |

## 5. Neural networks, GNN and unsupervised learning

| # | Paper | Idea | Result (authors) | Weakness | For v8 |
|---|---|---|---|---|---|
| 20 | Gurumoorthy, Subhash, Pérez de Prado, Woźniak, **Optimal CH selection in WSN with CNN-based energy level prediction**, Sensors 22(24):9921, 2022 | A CNN predicts the nodes' energy; CHs are chosen from the prediction | Improvement over the authors' baselines (abstract) | Needs training data; prediction error | ★★ v8's energy gate could use **predicted** instead of current energy |
| 21 | Saadati, Mazinani, Khazaei, Seyyed Mahdavi Chabok, **Energy efficient clustering for dense WSN by applying graph neural networks with coverage metrics**, Ad Hoc Networks 156:103432, 2024 | A GNN (edge regression) forms **equal-size static clusters**; CH selection is distributed inside each cluster | Improvement in energy and coverage over the authors' baselines (abstract) | Static clusters; GNN trained offline | ★★ GNN cluster formation at the BS could replace v8's nearest-CH join (balanced clusters) |
| 22 | **Hybrid deep learning (deep recurrent GNN) CH selection with metaheuristic optimization**, Wireless Personal Communications 2026 ([link](https://link.springer.com/article/10.1007/s11277-026-12184-0)) | GNN + RNN predict CH suitability from degree, energy, delay and distance | Improvement over the authors' baselines (abstract) | Heavy model | ★ |
| 23 | Mwangi, Ndia, Muketha, **An extended K-means cluster head selection algorithm for efficient energy consumption in WSNs**, IJNSA 2023 | K-means clusters; CH by energy, BS distance, density, RSSI / SNR | Better than LEACH, Mod-LEACH, TSILEACH | k must be chosen; centralised | ★ |

## 6. Surveys (background)

| # | Paper |
|---|---|
| 24 | Alsheikh, Lin, Niyato, Tan, **Machine learning in wireless sensor networks: algorithms, strategies, and applications**, IEEE Communications Surveys & Tutorials 16(4), 2014 ([arXiv](https://arxiv.org/abs/1405.4463)) |
| 25 | **Optimized clustering algorithms for large wireless sensor networks: a review**, Sensors 2019 ([PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC6359437/)), with a reinforcement-learning section |

## 7. How these can improve v8: ranked plan

What v8 already does (and the AI must not break):
- Single-pass score `T(r)·(E/E0)·(1 + deg/degmax)`.
- Clusters reused for 5 rounds.
- Backup CH, I1–I5, and the energy-aware relay (v8-Chain).
- Its strength is that elections and control messages are cheap. So the AI should run **at the BS**, or be a small table on the nodes, and must not add messages per round.

| Idea | From | What changes in v8 | Why it may help (our own results) | Node cost |
|---|---|---|---|---|
| **A. RL-adaptive re-clustering interval** | LEACH-RLC (#2) | The BS runs a Q-learning agent. State = alive nodes, energy variance, mean CH energy. Action = re-cluster interval (e.g. 3, 5, 10, 15 rounds). Reward = delivered packets per joule. | The fixed interval is a trade-off we measured: interval 5 gives FND 1645 (v3) vs 1513 for 15 (v4). The best interval changes during the lifetime. | none (the BS decides; the interval rides on the existing beacon) |
| **B. Learned / optimised CH score weights** | PSO-ECHS (#15), EECHIGWO (#16), DS-HHP state (#7) | Keep the single-pass election. Learn the weights of energy, degree and **distance to the BS** (not in the v8 score today) with PSO/GWO offline, or with RL at the BS. Nodes get the weights in the beacon. | v8's far-BS FND (1406 v8, 1626 v8-Chain) is the weak case; distance to the BS is exactly what is missing there | none (a few weights in the beacon) |
| **C. Fuzzy unequal clusters for the far BS** | EAUCF (#12), fuzzy + Q-learning (#5) | Members join with a radius that shrinks for CHs that relay for others (near the BS) or have low energy | Relaying CHs die first in v8-Chain far-BS; smaller clusters reduce their load | none (rules are fixed) |
| **D. Learned relay decision** | #5, #8, #9 | Replace the greedy energy rule of v8-Chain with Q-learning on the CH-to-CH relay (reward = energy saved) | Could beat the greedy rule when the BS is far | small Q-table per CH |
| **E. BS-side swarm CH selection (baseline)** | PSO-ECHS (#15), GWO (#16–#19) | The BS picks the CH set with PSO / GWO every set-up | A strong AI baseline to compare v8 against; shows whether centralised optimisation beats our cheap distributed rule | the CH list is sent in the beacon (control bits) |

Proposed order:
1. **A** (cheap, directly about a v8 parameter).
2. **B** (fixes the far-BS weakness).
3. **E** as the AI baseline for the comparison.
4. **C / D** if time allows.

Every idea will be implemented as a new protocol engine in `omnetpp/src/protocols/`. Each one is evaluated in the same unified environment and topologies as v8:
- FND, HND, LND and PDR;
- energy-balance Gini index (from #14);
- control overhead.

The cost of carrying the decisions (beacon bits) is charged.
