modC[J] =: cNu;
xa_[{Aqua}] =: cNu * 1;

from 1
to 50
step 1

---

# T-cement — a Portland-cement hydration system (`gems3k-fail`)

**Re-exported 2026-09-08 with the missing Mg and K hosts added, and promoted from
`Resources/under_review` into `gems3k-fail` the same day.** Everything below is current for
that export **on ThermoFun (`-f`)**, which is the shipped configuration; the superseded
findings are kept, marked, because two of them are the reason the re-export happened.

**Two CTest cases are registered — `cement_water.native` and `cement_water.SIA`.** The four
Optima modes are **deliberately not registered**: they cost 25–60 s per point here against
native's ~12 ms, so one 50-point sweep is 20–50 minutes. Run them by hand when working on
Optima's cost on this system, which is the open thread this project exists for.

## What this project is

Exported from GEMS as `cem_resin:G:cement:2:0:1:25:0`. **207 species / 15 ICs / 61 phases**
(was 203 / 15 / 57), a single state point at **298.15 K / 1 bar**. The DATACH carries a real
T grid (17 nodes, 273.15–353.15 K) but only one P node, so a P sweep is not available.

**The `.lst` reads `-f` — ThermoFun.** The 2026-09-08 re-export arrived as `-j` (DATACH
grid); that was flagged and **`-f` was restored** on the owner's instruction, after checking
that ThermoFun covers all 207 species (0 missing) and that it loads and solves.

**Both configurations work, and they agree on the chemistry.** Measured like-for-like over
the full 50-point native sweep, 2026-09-08:

| | `-f` (ThermoFun, **shipped**) | `-j` (DATACH grid) |
|---|---|---|
| answers | 49/50 (loses 32 g) | 50/50 |
| jitter max (4 trace-seed values) | union `{30, 32}`, **worst draw 1** | union `{31, 32}`, **worst draw 1** |
| `Mass balance cannot be improved` | 34 of 50 | 0 |
| iterations, whole sweep | 12 078 | 11 370 |
| **worst relative `|dG|`, 49 shared points** | **7.3e-08** (at 33 g) | |
| worst relative `|dVs|` | 1.3e-03 (at 34 g) | |
| assemblage differences | **3 of 49 points**, all trace phases at the presence floor: `Friedels` only in `-j` at 26 g and 29 g (~1e-11), `Kuzels` only in `-f` at 30 g (1.7e-12) | |

So the **answers are the same** — `G` to seven digits, and the only assemblage differences are
three phases sitting at the 1e-12 presence threshold, which is in/out noise rather than
chemistry. What differs is **how hard each path works and which marginal point it lands on**:
the `-f` path strains far more in mass-balance refinement (34 warnings vs 0) and happens to
land on a losing draw at 32 g, where `-j` happens to land on a winning one.

**Neither is more correct, and `-j` is not more robust** — both have a jitter max of **1 lost
answer** over the same 30–32 g region. `-f` is simply unlucky at the shipped composition.

That said, they are still an `f_`/`j_` thermodynamic-data pair (`../../CLAUDE.md` §3): **do not
pool their rows**, and do not read a difference between them as a solver effect.

### The recipe — unchanged by the re-export, and recoverable from `b`

| entry | quantity |  | entry | quantity |
|---|---|---|---|---|
| `xa_ Aqua` | 15 g | | `xa_ CaCO3` | 4 g |
| `xa_ C2S` | 10 g | | `xa_ CaSO4` | 4 g |
| `xa_ C3A` | 5 g | | `xa_ K2SO4` | 0.9 g |
| `xa_ C3S` | 60 g | | `xa_ MgO` | 1 g |
| `xa_ C4AF` | 5 g | | `xa_ O2` | 1 g |
| | | | `bi Cl, Cs, Na, Sr` | 1e-09 M each |

15 g water + 90.9 g dry = **105.9 g**, exactly the `Ms` the solver reports. w/s = 0.165.

A GEMS3K export carries only the bulk composition, not the recipe. It is recoverable
because **`Aqua` is this recipe's only hydrogen-bearing input**:

    b[H] = 2 n_w   =>  n_w = b[H]/2 = 0.832625600 mol = 15.000000 g
    O_dry = b[O] - b[H]/2 = 2.105513358 mol

so setting the water to `m` grams is `b[H] = 2m/M`, `b[O] = O_dry + m/M`, every other `b[i]`
untouched, with `M` from the project's own DATACH `ICmm` (18.015300035 g/mol). That
`b[H]/2 · M` reproduces the 15 g Aqua to better than 1e-6 g **and** the remainder comes to
90.900000 g is what establishes the "only Aqua carries H" property rather than assuming it.
`include/difftest/cement_water_sweep.h` implements this; `tests/test_cement_water.cpp`
checks it four ways.

## Why this is a DIFFICULT test — and where its answers still mean nothing

The re-export fixed the phase-set defect. It did **not** make this an easy or fully
trustworthy system, and three of the four difficulties below are unchanged.

**1. Sixteen solution phases, eight of them non-ideal.** Unchanged by the re-export:

| model | code | phases |
|---|---|---|
| extended Debye–Hückel, Helgeson `b_gamma` | `H` (`SM_AQDHH`) | `aq_gen`, **121 species** |
| binary Guggenheim / Redlich–Kister, **non-ideal** | `G` (`SM_REDKIS`) | `ettringite-AlFe`/`-FeAl`, `monosulph-AlFe`/`-FeAl`, `SO4_OH_AFm`/`OH_SO4_AFm`, `SO4_CO3_AFt`/`CO3_SO4_AFt` |
| ideal solution | `I` (`SM_IDEAL`) | `gas_gen`, `C3(AF)S0.84H`, `CSHQ` (6 DC), `straetlingite`, `ettringite`, `hydrotalc-pyro`, `MSH` |

The eight non-ideal ones are **four twin pairs** differing only in which of Al/Fe sits on
which site, and both members are routinely present at once — near-degenerate by
construction, and exactly what a `dG` comparison cannot police.

**2. The aqueous phase is absent over most of the axis — and that is now CORRECT.** It is
below 1e-7 mol from 1 g to 33 g and only becomes a real phase past 33 g. For a water-starved
cement paste that is the right answer; before the re-export the same region carried a
fictitious 0.037 mol brine (see the history below).

**3. Where a solution exists, the model's domain is what limits it — 17 of 50 points.**
Extended Debye–Hückel is a dilute-to-moderate-electrolyte model; past ~1 molal it is
stretched and past ~3 molal it is outside any defensible domain.

| band | aqueous phase | ionic strength | status |
|---|---|---|---|
| 1–33 g | **3e-09 – 1e-07 mol** | reported 1.6–34.7 molal | **no free solution.** pH, Eh and I are not observables of anything — the phase is absent, so its composition is undetermined. Reported pH runs to 23.6 |
| 34–50 g | 0.04–0.93 mol | **0.17–0.23 molal** | **the only band inside the model** — 17 of the 50 points |

So **`G` and the phase assemblage are worth scoring across the whole axis; pH, Eh and ionic
strength only on the wet branch.** `tests/test_cement_water.cpp` asserts no aqueous property
outside it, and asserts the band's own extent so it cannot silently move.

The measured demonstration, from the solvers themselves: on the *first* export, HOP and SHP
at 11 g produced **identical assemblages**, agreed on `G` to **7e-10 relative**, and
reported **pH = 14.026 vs −35.678** — fifty units apart, on 4.6e-09 mol of "solution".

The one apparent exception is deliberate. The test checks `dG/dn(H2O)` at the **dry** end,
where the reported ionic strength is nonsense, and that check is legitimate because with
lime and portlandite both present `μ(H2O)` is fixed by `CaO + H2O → Ca(OH)2` — a relation
between **solid** standard potentials. It is a check on the solid data and never touches
the activity model. Measured **−292.984 kJ/mol**, flat to six decimals over 1–5 g; at the
wet end the same derivative is **−237.195 kJ/mol**, water's own (textbook −237.14).

**4. The gas phase dominates the volume and sets the redox.** The recipe adds 1 g of O2 and
essentially none of it reacts: of 3.1523e-02 mol of gas at 15 g, **3.1251e-02 mol is O2**,
plus 2.7e-04 mol H2O vapour and a trace of CO2. At 298.15 K / 1 bar that is **781 cm³ of gas
against 40 cm³ of condensed matter — ~95 % of the system volume.** So:

- **`Vs` here is an ideal-gas readout of the added O2, not mineralogy.** A cross-solver `Vs`
  agreement says almost nothing about the hydrates. The test asserts the 95 % share.
- The O2 **buffers the redox**, pinning `Eh` near +0.41 V and fixing the Fe(II)/Fe(III)
  split — which is what the four Al/Fe twin pairs respond to.

## RESOLVED 2026-09-08 — the low-water aqueous phase was a container, not a solution

Kept because it is the reason for the re-export and because the test that caught it is now
written the other way round.

**The symptom.** On the first export the aqueous phase was **larger at 1 g of water
(3.69e-02 mol) than at 15 g (4.13e-09)** — adding water destroyed the solution. It was not
the water that was larger: of the 1 g, 0.888 g went to portlandite and 0.112 g to the
aqueous phase, whose **solvent `H2O@` was 4.7 % of it**; the rest was `MgSO4@` (67 %) and
`KOH@` (24 %), at **a(H2O) = 1.7e-10**.

**The cause was the phase set, not the solver** — all six modes agreed on it. K's only solid
host anywhere in the 57 phases was `KSiOH`, one end-member inside the **CSHQ hydrate**
(absent below 10 g); every Mg host (`OH-hydrotalcite`, `hydrotalc-pyro`, `MSH`) was hydrated
too (none below 6 g). So the recipe's K2SO4 and MgO had nowhere to go and the solver kept a
brine alive purely to hold them. Meanwhile `Per`, `Brc`, `Mgs`, `Sy`, `K2SO4`, `syngenite`
and `K-Ox` were all **in the shipped database and none in the system**, while `Na2O` was in
the system despite Na being a 1e-09 trace seed.

**The fix.** The 2026-09-08 re-export added **`Periclase` (MgO), `arcanite` (K2SO4),
`syngenite` and `Magnesite`**. Now:

| water | aq (mol) | where Mg is | where K is |
|---|---|---|---|
| 1–5 g | **3.05e-09** | `Periclase` 2.48e-02 mol — the whole Mg inventory | `arcanite` 5.16e-03 mol — the whole K inventory |
| 9 g | 3.85e-09 | `OH-hydrotalcite` 6.20e-03 | `arcanite` |
| 15 g | 4.10e-09 | `OH-hydrotalcite` | `syngenite` 3.93e-03 |
| 40 g | 3.77e-01 | dissolved | dissolved (`K+` 1.31e-03) |

Anhydrous host → hydrated host → dissolution, which is the chemically sensible progression.
**All 1 g of water now goes to portlandite**, where 0.112 g of it used to be stranded.

**And it cured the native solver**, which is the confirmation that the fictitious phase was
what made the linearized system near-singular:

| | first export | re-export |
|---|---|---|
| native answers | 49/50 (lost 31 g) | **50/50** |
| SIA | 49/50, 2 warm abandonments, 1269 it | **50/50, 0 abandonments, 75 it** |
| `Mass balance cannot be improved` | **47 of 50 steps** | **0** |

So *"native cannot re-solve its own answer"* was a property of that phase on this fixture,
not of the warm path.

## The 30–32 g region: a closed miscibility gap on a doubly-defined phase

Diagnosed 2026-09-08, and **stated carefully** — the first version of this section claimed more
than the measurements support, and the `-j` control caught it. Read both halves.

**Four of the solution phases are defined TWICE.** Verified against the DCH stoichiometry
matrix *and* the `PMc` interaction parameters:

| pair | end-members | stoichiometry | Guggenheim a0, a1 |
|---|---|---|---|
| `monosulph-AlFe` / `monosulph-FeAl` | `monosulphate1205`, `Fe-monosulph05` | **identical** | **identical** (1.26, 1.57) |
| `SO4_OH_AFm` / `OH_SO4_AFm` | `C4AH13`, `monosulphate12` | **identical** | **identical** (0.188, 2.49) |
| `SO4_CO3_AFt` / `CO3_SO4_AFt` | `tricarboalu03`, `ettringite03_ss` | **identical** | **identical** (1.67, 0.946) |
| `ettringite-AlFe` / `ettringite-FeAl` | — | **identical** | **identical** (2.10, −0.169) |

The same solid solution entered twice — the standard GEMS idiom for a **miscibility gap**,
since a symmetric Guggenheim model needs two phase instances for the two limbs of a solvus.
Where the gap is open the copies hold different compositions; **where it closes they hold
EQUAL amounts, and their columns in the linearized system become identical.**

### ESTABLISHED: the collapse causes a large, reproducible COST penalty

Native, both thermodynamic data sources, ratio between the two members of each pair:

| water | `monosulph` ratio | `SO4_OH_AFm` | ITG `-f` | ITG `-j` |
|---|---|---|---|---|
| 26 g | 3.06e+05 | 1 | 176 | 169 |
| 27 g | 2.47e+05 | 1 | 201 | 200 |
| 28 g | 1.68e+05 | 1 | 183 | 181 |
| 29 g | 1.08e+05 | 1 | 197 | 192 |
| **30 g** | **1 — gap shut** | 1 | **1317** | **1295** |
| **31 g** | **1** | 1 | **1136** | **1349** |
| 32 g | 1 | 1 | **`E07IPM`** | 262 |
| 33 g | absent | absent | 284 | 297 |

The monosulphate solvus narrows monotonically over 26→29 g and snaps shut at 30 g — **exactly**
where cost jumps from ~190 to ~1300 iterations, a **~7x** penalty. **It reproduces to three
significant figures across `-f` and `-j`**, which is what makes it a property of the phase
geometry rather than of either G0 set.

### NOT ESTABLISHED: that the collapse is what loses the answer

**Under `-j`, 32 g has BOTH pairs collapsed and converges in 262 iterations.** So the
degeneracy is a *predisposing condition*, not a sufficient cause. Whatever kills the `-f`
solve at 32 g needs the collapse **and** something else.

The something else is visible in the assemblage: 32 g is a simultaneous multi-phase turnover.
Between 31 g and 33 g, with both twin pairs still collapsed:

| phase | 31 g | 32 g (`-j`) | 33 g |
|---|---|---|---|
| `monosulph-AlFe` = `-FeAl` | 3.82e-03 | **1.72e-05** (÷222) | absent |
| `SO4_OH_AFm` = `OH_SO4_AFm` | 7.67e-06 | **6.42e-04** (×84) | absent |
| `Anhydrite` | 3.73e-03 | absent | absent |
| `Friedels` | 9.39e-12 | absent | absent |
| `syngenite` | 6.49e-04 | absent | absent |
| `SO4_CO3_AFt` | absent | absent | 2.77e-02 |
| `C4AcH11` | absent | absent | 1.29e-03 |

Two rank-deficient pairs are changing by two orders of magnitude in opposite directions while
three more phases leave and the carbonate-AFt family prepares to enter. **Whether the solver
threads that is marginal, and a small G0 difference decides it** — which is exactly consistent
with the same point flipping under a 1e-18 mol nudge (see the jitter table below). If the
collapse alone were sufficient, 30 g and 31 g would fail too; they do not. If it were
irrelevant, they would not cost 7x.

### What this is and is not

**Not a fixture defect.** Removing one of each pair would remove the model's ability to unmix.

**Where it connects.** `j_CASHNK` is the corpus's existing case of this class — *"two
interchangeable Berman multi-site solid solutions; the only case exercising the phase-extinction
retry"*. **T-cement has four pairs.** And the asymmetry is the lead: the **Optima path has
explicit machinery for interchangeable phases** — its log carries `phase-extinction retry -
fixing 2 species of a vanishing interchangeable phase at the floor` — **while native has none,
and native is what pays the 7x and loses the 32 g answer.**

## The knife edges are SMALLER, not gone — read the jitter max

Re-running the whole sweep at four values of one trace seed (Cl at 1e-09 as shipped, then
1.001e-09 / 2e-09 / 5e-09 — perturbations of order 1e-18 mol):

| Cl seed | answers | loses |
|---|---|---|
| 1e-09 *(shipped)* | **50/50** | — |
| 1.001e-09 | 49/50 | 31 g |
| 2e-09 | 50/50 | — |
| 5e-09 | 49/50 | 32 g |

10 g is genuinely fixed — that was the free-solution-collapse boundary. **31 g and 32 g are
still knife edges**; they merely happen to converge at the shipped seed, and the response is
still non-monotone. Union down from `{10, 31, 32}` to `{31, 32}`, worst draw from **3 to 1**.

**So a ceiling for this fixture still comes from the worst draw, not the shipped one.** The
test sets `errCount <= 1` and says so. Setting it to 0 from the shipped 50/50 would repeat
exactly the mistake the first version of this file recorded.

## Status: promoted, with one thread open

Promoted into `gems3k-fail` on 2026-09-08. `tools/freeze.sh` will now pick it up by default,
so the standard grows by one project × 6 modes — **that should be a deliberate re-baseline**,
like the `recheck.py` claims were.

Still open:

- **Optima's cost on this system** — 25–60 s per point. Not budget-limited (`pa_IIM`
  9999 → 200000 made the 15 g solve *shorter*, 22500 → 6786 iterations, and it still failed;
  disarming `pa_OptimaStallWindow` gave a byte-identical 6786), not near-tolerance
  (`pa_OptimaTol` 1e-8 / 1e-6 / 1e-4 give identical runs), and eight settings variants all
  fail. **The concrete lead**: `pa_OptimaDimReduce` AUTO engages (207 species vs the 200
  gate) and burns **11000 iterations selecting zero species** before discarding the
  pre-solve — a third of the budget, fixable with a per-project key. **Every Optima number
  here is from the PRE-re-export files and must be re-measured.**
- The **17-of-50 validity band** is inherent to the chemistry and the activity model, not a
  defect, but most of the axis can only ever be scored on `G` and assemblage.
- **30 g and 32 g remain verdict-unstable**, so any ratchet stays jitter-max based.
