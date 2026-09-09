# Proposed test systems — as built, checked 2026-09-04

Five GEM-Selektor exports staged here against the specs in
`GEMS3K/Docs/literature/PROPOSED-TEST-SYSTEMS.md`. **All five load and solve; native,
AOP and HOP agree on `G` to 11 significant figures on every one.** This file records
what each actually measures once run, and the exact command to run it.

The driver for all of them is `tools/proposed_sweep.cpp` (built as
`build-solvus/bin/proposed_sweep`) — one parameter axis (`--axis=tk|p|bic:NAME`), a
fresh `TNode` per point unless `--warm` is given, and per-point reporting of status,
iterations, `G`, pH, Eh, named phase amounts + logSI, named species amounts, per-IC
mass-balance residuals split major/trace (`--resid`), and water's H:O degeneracy
(`--ho`).

| dir | as exported | verdict |
|---|---|---|
| `T1`  | 8 IC / 114 DC / 18 PH, 25 °C. Am, C, Np, Pd, U all at **1e-9 mol** against H = 111 | **fit for purpose, and it answers T1 negatively** |
| `T2b` | 7 IC / 30 DC / 7 PH, 25 °C. 1 kg H₂O + Ca/Fe/Mg at 1e-6 + 0.3 M O₂ + 0.7 M N₂ | **fit for purpose — it is the whole knob, not one arm of a pair** |
| `T5`  | 5 IC / 43 DC / 9 PH, 25 °C. Hematite + magnetite + U at 1e-8 | **fit for purpose; refutes its own hypothesis; carries a sharp AOP stress case** |
| `T10` | 4 IC / 27 DC / 4 PH, **5 °C**. 1 kg H₂O + U at 1e-8, nothing else | **fit for purpose; pairs with T5 to settle T16** |
| `T11` | 5 IC / 21 DC / 5 PH. 1 kg H₂O + 0.136 mol CO₂ + Si trace. **9 T × 26 P grid** | **the richest of the five** |
| `T12` | 4 IC / 18 DC / 2 PH. `aq_gen` + **`fluid_prsv` (PRSV cubic EOS)**, 2.27 mol CO₂ in 1 kg H₂O. **61 T × 101 P grid at 0.5 K / 0.5 bar** | **works, shows the root switch cleanly — and exposed a native-path bug** |

Two mechanical points before the per-system notes:

- **`T1-fun.json` and `T10-fun.json` are zero-byte.** Harmless — both `.lst` files use
  `-j`, so the ThermoFun file is never opened. Delete them or leave them; nothing reads
  them.
- **Naming.** The corpus convention is `<f|j|o|t>_<Project>_<system>_<T>_<P>_...`.
  These are bare `T1`…`T11`, which is fine for staging but would need renaming before
  adoption into `Resources/gems3k*` and CTest, because several harness scripts glob on
  that shape (and one, `dir_sweep.sh`, assumes the `.lst` basename matches the
  directory name — which it does here, so that one is already safe).

---

## READ THIS BEFORE SWEEPING T OR P: the grid tolerances bound the step size

All five exports carry **`Ttol` = 0.5 K** and **`Ptol` = 50000 Pa (0.5 bar)**.
`check_grid_T`/`check_grid_P` (`GEMS3K/datach_api.cpp`) **snap a requested T or P to a
grid node whenever it is within the tolerance of one**, and use that node's tabulated
standard-state properties directly instead of interpolating. So:

| | grid spacing | tolerance | smallest meaningful step |
|---|---|---|---|
| T1 / T2b / T5 | 5 K | 0.5 K | **0.5 K** |
| T11 (T axis) | 25 K | 0.5 K | 0.5 K |
| **T11 (P axis)** | **1 bar** | **0.5 bar** | **1 bar** |

**T11's `Ptol` is exactly half its P grid spacing, so every pressure snaps to a node and
the P axis is effectively quantised to 1 bar.** Sub-bar steps do not refine anything:
`G0`/`V0` stay piecewise-constant while the pressure entering the gas-phase EOS moves
continuously, and the two disagree.

**This is not a theoretical caution — it produced a spurious result here.** A first pass
refined the T11 crossing at ±1e3, ±1e2 and ±1e1 Pa and found native carrying 3–5e-6 mol
of a gas phase whose own `logSI` was negative, while AOP put it at its floor. At exact
grid nodes that disappears completely: gas is `0.000000e+00` at every pressure with
`logSI < 0`. The "vestigial marginally-unstable phase" was an artefact of stepping below
`Ptol`, and it was nearly recorded as a solver finding.

**Rule for these systems: step T by ≥ 0.5 K and P by ≥ 1 bar, and prefer exact grid
nodes.** Bulk-composition axes (`--axis=bic:NAME`) are unaffected — no tolerance applies
to `bIC`, so the `bic` sweeps and the 1e-15 jitter tests below are all legal at any step.

`proposed_sweep` now **refuses** a sub-tolerance T or P sweep unless `--force` is given.

---

## These carry the CURRENT GEM-Selektor defaults — including active smoothing

All five ship identical settings, matching the GUI Controls tab exactly: `pa_DK` 1e-6,
`pa_IIM` 9999, `pa_LLG` 30000, **`pa_AG` 0.7**, **`pa_DGC` −0.0065**, `pa_DHB` 1e-13,
`pa_DT` 0, `pa_DG` 1000, `pa_DS` 1e-20, `pa_DcMin` 1e-33, `pa_DF`/`pa_DFM` 0.01,
`pa_PC` 2, `pa_DW` 1, `pa_PSTALL` 1.

**`pa_AG` 0.7 + `pa_DGC` −0.0065 puts native's smoothing on branch B2 — active.** Most of
the existing corpus ships `pa_AG` 1 / `pa_DGC` 0, where the filter is exactly inert, so
"smoothing is mostly a no-op here" is a fact about the corpus's age rather than about
GEMS3K. It will stop being true as projects are re-exported.

A/B'd on scratch copies patched to `pa_AG` 1 / `pa_DGC` 0: **every AOP number is identical
to the digit** (the Optima path forces `TF` = 1 regardless), and native moves by up to
~40 % — T2b at excess O = 1e-3 goes 76 → 54, T11's first titration step 142 → 134 —
without changing any conclusion below. The one thing it does move materially is native's
vestigial gas amount in T11's approach band, by an order of magnitude, which is what
identifies that amount as noise.

**When comparing native against AOP on these systems, remember the two are not running the
same objective**: native smooths, AOP does not.

---

## T1 — wide-dynamic-range trace system

Five trace ICs at 1e-9 against H = 111 mol: a dynamic range of **1e11**. `Pd(cr)` is
present at 6.8e-10 mol (68 % of total Pd) at logSI = +0.004, and `NpO2(am_hyd)` sits at
logSI = −0.037, so the trace elements are genuinely solubility-controlled rather than
sitting inertly in solution.

```
cd T1
mb_class_probe T1-dat.lst
proposed_sweep T1-dat.lst --axis=bic:Pd --from=1e-6 --to=1e-13 --steps=8 --log \
               --modes=native --resid --phase="Pd(cr)"
```

**It reproduces the code-vs-paper divergence exactly** — `mb_class_probe` reports
`code(rel-to-all) FAIL | paper(rel trace / abs major) PASS`, and native's SIA warm
restart fails at 113 iterations. So T1 joins the family of projects (11 of 56 as of
2026-09-03) that return an answer failing their own mass-balance test.

**But it settles the T1 question in the opposite direction from the proposal.** Across
the Pd ladder — 1e-6 down to 1e-13, crossing `Pd(cr)`'s dissolution between 1e-9 and
1e-10 — the **trace** class holds ~2e-16 relative and ~2e-25 mol absolute at every rung,
while the **major** class runs 1.5e-13 to 1.5e-12 relative against `pa_DHB = 1e-13`.
The failing IC is O, then H. A relative-for-trace rule buys nothing here because the
trace ICs are already solved to 1e-16; the binding problem is a **major** IC held to a
relative bar it cannot meet.

This confirms the 2026-08-29 inversion on a purpose-built system, which is stronger than
the corpus could manage — it is now known not to be an artefact of the corpus lacking
trace ICs.

## T2b — water-dominance contrast pair

**This one file is the entire pair, not the (b) arm of one.** The contrast is produced
by the excess O, and the knob is continuous over eleven orders:

```
cd T2b
proposed_sweep T2b-dat.lst --axis=bic:O --from=56.10992 --to=55.50992 --steps=9 \
               --modes=native,aop --ho --phase=gas_gen
```

`1 − A_HO²/(A_HH·A_OO)` is 0 for a water-only system (H₂O's 2×2 outer product
`[[4,2],[2,1]]` is exactly singular — the documented near-null direction), so it measures
precisely what non-water H/O carriers provide:

| bIC[O] | excess O₂ | 1 − degeneracy | native it | AOP it |
|---|---|---|---|---|
| 56.10992 | +0.6 | 1.16e-04 | 134 | 37 |
| 55.60992 | +0.1 | 3.23e-06 | 68 | 37 |
| 55.51992 | +0.01 | 3.22e-08 | 52 | 47 |
| 55.51002 | +1e-4 | 3.08e-12 | 341 | **877** |
| 55.50993 | +1e-5 | 2.32e-14 | 368 | **3593** |
| 55.50992 | 0 | 4.66e-15 | 1179 | 1194 |

**AOP is flat at ~35–47 iterations across four decades of the knob and then rises
monotonically to 3593 — a 103× penalty attributable to the H:O degeneracy alone.**

The attribution is clean, and it needed checking: Eh flips +0.70 → −0.35 V when the
excess O runs out, so the cost spike could have been the redox transition rather than the
conditioning. It is not — **AOP's peak is at excess O = 1e-5, where Eh is still
+0.645 V**, and the whole monotone run 35 → 47 → 104 → 877 → 3593 happens at near-constant
Eh (+0.703 → +0.645). This is the first time the H:O rank deficiency has been a
*controlled* knob rather than an incidental property.

**Caution: do not set `bIC[N_atm] = 0`.** Exactly zero makes native fail (`status=4`) at
every O value tried; 1e-6 behaves identically to 0.7. N₂ is not the carrier — O₂ is — so
N_atm can be reduced to a token amount but not removed.

## T5 — redox-buffered trace system

**The buffer is real and holds across the whole T grid.** Fe₂O₃ (0.0626 mol) and Fe₃O₄
(0.0432 mol) are both present at logSI = 0 at every T from 278 to 368 K, with Eh tracking
smoothly −0.258 → −0.215 V and `UO₂(am_hyd)` at logSI = −0.003.

```
cd T5
proposed_sweep T5-dat.lst --axis=tk --from=278.15 --to=368.15 --steps=10 \
               --modes=native,aop --phase="Fe2O3(alpha)" --phase="Fe3O4(cr)"
```

**It refutes the hypothesis it was built to test.** Kulik 2013 App. 2.5 holds that the
buffer pins the dual while trace primal amounts stay ill-determined, so the two error
channels can be measured apart. Perturbing `bIC[Fe]` or `bIC[O]` by 1e-14 … 1e-9
relative moves **neither**: Eh is unchanged to six decimals *and* U speciation is
unchanged to **nine significant figures**, including `UO2+2` at 1.2e-16 mol — eighteen
orders below the bulk. Once the buffer pins Eh and pH, every aqueous U species follows by
mass action with no freedom left. The channels are not separable on a buffered system
because the buffer determines both.

**A sharp AOP stress case, worth keeping on its own account.** Bracketed at legal 0.5 K
steps, AOP climbs gently — 149, 165, 167, 182, 198, 189 iterations from 361.15 to
363.65 K — and then jumps to **10002 (its cap)** at 364.15 and stays there for every
temperature up to 368.15, while native holds flat at 60–61 throughout and both return
the identical `G`. **The threshold is between 363.65 and 364.15 K, i.e. resolved to the
0.5 K tolerance limit** — it cannot be bracketed finer on this export without a denser
T grid. A 164× penalty on a system native finds trivial.

## T10 — unbuffered-redox system

T5 minus the iron: same water, same U at 1e-8, same two U solids, no buffering mineral
pair and no gas phase to pin the redox state.

```
cd T10
mb_class_probe T10-dat.lst --jitter=9
proposed_sweep T10-dat.lst --axis=bic:U --from=1e-5 --to=1e-12 --steps=8 --log --modes=native
```

**T5 + T10 is the T16 Eh-determinacy pair, and better than two purpose-built exports
because everything except the buffer is matched.** Under a `bIC` jitter of at most 9e-15
relative:

| | Eh spread | pH spread |
|---|---|---|
| **T5** (buffered) | **1.09e-11 V** | 3.9e-11 |
| T2b (0.3 mol O₂) | 9.24e-10 V | 1.2e-09 |
| T11 | 3.24e-04 V | 6.7e-07 |
| **T10** (unbuffered) | **1.17e-02 V** | 2.8e-05 |

Nine orders between the two arms. And the primal channel is at least as sensitive as the
dual — a **one-part-in-a-billion** change in `bIC[O]` takes T10 from Eh = −0.190 V to
+0.757 V and `U(OH)4(aq)` from 9.98e-9 mol to **exactly zero** (U flips wholesale from
U(IV) to U(VI)); at 1e-13, essentially numerical noise, the dominant U species has
already dropped 3×. The same perturbations move T5 not at all.

Taken together with T5 this says something the suite could not say before: **where the
dual is pinned the primal is pinned too, and where the dual is loose the primal is
looser.** The two error channels co-vary rather than separating.

**One mismatch to know about:** T10's T grid is a single point at **278.15 K** while
T5's dbr is at 298.15 K. T5's grid starts at 278.15, so run T5 at `--tk 278.15` for an
exactly matched pair, or re-export T10 at 298.15.

## T11 — boundary crossing at fine resolution

The richest of the five, and the only project in any Resources directory with a **real
pressure grid** (26 points, 0–2.5 MPa; every other project in all three corpora has
`nPp = 1`). At its shipped state point (150 °C, 20 bar) the gas phase sits at
logSI = −0.019 — right on the exsolution boundary.

```
cd T11
proposed_sweep T11-dat.lst --axis=p --from=2500000 --to=100000 --steps=25 \
               --modes=native --phase=gas_gen
# refine at the crossing:
proposed_sweep T11-dat.lst --axis=p --from=1850000 --to=2050000 --steps=5 \
               --modes=native,aop --phase=gas_gen
```

**The crossing penalty is large, and native and AOP are penalised on opposite sides of
the boundary.** At exact grid-node pressures (1 bar steps — see the tolerance section):

| P (MPa) | logSI(gas) | gas (mol) | native it | AOP it |
|---|---|---|---|---|
| 2.2 | −0.0599 | 0 | 87 | 159 |
| 2.1 | −0.0398 | 0 | 101 | 208 |
| 2.0 | −0.0187 | 0 | 142 | **350** |
| 1.9 | 0.0000 | 1.90e-03 | **507** | 168 |
| 1.8 | 0.0000 | 1.45e-02 | 464 | 40 |
| 1.7 | 0.0000 | 2.76e-02 | 229 | 38 |

The gas appears between 2.0 and 1.9 MPa — adjacent grid nodes, so that is the finest the
export can resolve. **Native peaks at the first step where the phase is actually present
(507, 3.6× the step before); AOP peaks on the absent side approaching the boundary (350)
and then collapses to 40 once the phase is properly established.** Two different failure
surfaces around one boundary, which is the sort of thing only a crossing test shows.

Further out along the same path, on the same 1 bar grid, native's cost keeps climbing —
1043 at 1.6 MPa, 5098 at 1.5 MPa, 2705 at 1.3 MPa, **outright failure at 1.2 and
1.1 MPa**, 7356 at 1.0 MPa — up to 67× its single-phase cost, with the characteristic
scattered pattern (a failure with converging neighbours on both sides).

There is a **second** boundary lower down: below ~0.45 MPa at 150 °C the liquid is gone
entirely (`gas_gen` = 55.6 mol, the whole system flashed to vapour), and native's cost
drops back to 55–80 iterations. So the path crosses two appearance/disappearance
boundaries and has a cheap regime on either side of an expensive middle.

### T11, the better axis: titrate CO₂ at fixed 20 bar

**This is how T11 should be run.** The P axis is quantised to 1 bar by `Ptol`; a
composition axis carries no tolerance at all, so the same boundary can be crossed at any
resolution. CO₂ is a substance, not an IC, so C and O must move together 1:2:

```
cd T11
proposed_sweep T11-dat.lst --axis=bic:C --couple=O:2                --from=0.1363351 --to=0.1451351 --steps=12                --modes=native,aop --phase=gas_gen
```

Starting from the shipped composition (logSI = −0.0187) and titrating CO₂ in, at fixed
20 bar / 150 °C:

| bIC[C] | logSI(gas) | native it | AOP it | gas: native | gas: AOP |
|---|---|---|---|---|---|
| 0.1363351 | −0.0187 | 142 | 344 | 0 | 0 |
| 0.1387351 | −0.0129 | 147 | 484 | 0 | 0 |
| 0.1403351 | −0.0091 | 197 | 685 | 3.67e-06 | 2.36e-12 |
| 0.1419351 | −0.0053 | 220 | 1329 | 8.45e-07 | 4.04e-12 |
| 0.1435351 | −0.0016 | 386 | **3912** | 2.16e-05 | 1.35e-11 |
| 0.1443351 | −0.0001 | **1109** | 628 | 1.71e-04 | 1.41e-04 |
| 0.1451351 | 0.0000 | 426 | **114** | 1.18e-03 | 1.18e-03 |

**AOP's cost rises monotonically over eleven points as the boundary is approached from
the undersaturated side — 344 → 3912, an 11.4× penalty — and then collapses to 114 the
moment the phase is admitted.** No scatter at all. Native does the opposite: flat at
142–220 through the approach, peaking at 1109 exactly at the appearance step. Same
asymmetry the P sweep showed, now clean enough to ratchet on.

**And the marginal-phase finding is real, contrary to what the P sweep suggested.** In
the approach band native reports **8e-7 to 2e-5 mol of a gas phase whose own `logSI` is
negative**, and the amounts are non-monotone (3.67e-6, 4.43e-6, 8.45e-7, 1.02e-5,
2.16e-5) — noise at the 1e-5 level on a phase that should not be there. AOP over the same
band holds it at 2.4e-12 → 1.4e-11, at its floor and **rising smoothly**. Both give the
same `G` to 9 significant figures, so it is energetically irrelevant — but it is a real
difference in reported composition, on an axis with no tolerance to blame it on, and it
is the reverse of the usual pattern where AOP is the one leaving material at `dcFloor`.

This axis is what T11's spec actually wanted: the per-crossing penalty **as a function of
proximity to the boundary**, refinable without limit. Prefer it to the P sweep, and use
the P grid for what only it can give — a genuine pressure axis, which no other project in
any Resources directory has.

---

## T12 — cubic-EOS root transition

Built 2026-09-04 to the grid specified in `PROPOSED-TEST-SYSTEMS.md`, with the composition
corrected to an aqueous phase **plus** a CO₂–H₂O fluid rather than a pure fluid. The
tolerances are right this time: `Ttol` 0.2 K against a 0.5 K spacing, `Ptol` 2e4 Pa against
5e4 Pa — so unlike T11 **both axes are genuinely resolvable at their grid step**.

At the shipped point (25 °C, 50 bar) both phases are present: aqueous 56.7 mol + fluid
1.084 mol, the fluid 99.87 % CO₂.

```
cd T12
# the root switch: isothermal P scan across CO2 condensation at 25 C
proposed_sweep CO2-dat.lst --axis=p --from=5000000 --to=10000000 --steps=21                --modes=native --phase=fluid_prsv
# volume self-consistency across solvers
../../../../GEMS3K/debug-optima-vs-reaktoro/volcheck CO2-dat.lst
```

**The root switch is unmistakable.** Molar volume falls smoothly 5.23e-4 → 3.69e-4 m³/mol
from 50 to 62.5 bar, then **drops by a factor of 2.96 to 1.24e-4** — right at CO₂'s own
vapour pressure at 25 °C (63.9 bar) — while the phase *amount* barely moves (0.938 → 0.928
mol). That is the vapour→liquid root selection flipping (`lnf` minimum, `s_solmod2.cpp`).
Native converges at every point, 47–55 iterations, with no cost penalty at the transition.

### It exposed a real bug: `Ph_Volume`/`vPS` is wrong for a cubic-EOS fluid, on the native path

`pm.VXc = Σ pm.FVOL[k]`, and `Vs`/`vPS[]` are packed from those with the same conversion —
so `Σ Ph_Volume(k)` must equal `cVs()`. It does not, and only for EOS phases:

| project | fluid model | native Σ/Vs | AOP | HOP |
|---|---|---|---|---|
| T11, T5, T2b | ideal gas | 1.0000 | 1.0000 | 1.0000 |
| **T12** | **PRSV** | **1.1523** | 1.0000 | 1.0000 |
| **`j_TiQ_PRSV`** | **PR78** | **0.0433** | 1.0000 | 1.0000 |

Optima's value tracks the project's own tabulated `V0` to 0.2–0.5 % at every pressure and
switches root at the same grid node; native's is ~1.83× larger and switches one node early.
Against NIST, `V0` is right: CO₂ at 70 bar/25 °C is 63.5 cm³/mol, `V0` says 65.6, native
says 119.5. At 62 bar native implies Z = 0.94 — near-ideal-gas, impossible for CO₂ well
below its critical point. **Total `Vs` is correct on every path; it is the per-phase
breakdown that is wrong.** Not ThermoFun — a `-j` variant reading the same DATACH grid
reproduces both values unchanged.

### Two smaller ones

**AOP fails from 95 bar upward** (`status=12`) at only 25–41 iterations — a check rejecting
the answer, not a budget problem, with `succeeded=true` and no warning logged. **HOP
succeeds at every one of those points.** Not diagnosed.

**Its Eh differences are NOT attributable**: jitter spread 0.850 V under a 9e-15 `bIC`
perturbation, with no redox couple present (O₂@ 1e-12 or 0; H₂@/CH₄@/CO@ exactly 0). Same
as T10.

**Its solver settings differ from the other five in 25 classic `pa_*` fields**: `pa_DK`
1e-5, `pa_IIM` 9000, `pa_AG` 1 / **`pa_DGC` +0.01** (smoothing branch B1, not B2), `pa_DHB`
1e-12, **`pa_DT` 1**, **`pa_DG` 100**, `pa_DS` 1e-15, **`pa_PC` 1 — PSSC disabled**,
**`pa_DW` 0**, and floors 1–8 orders looser (`pa_DcMin` 1e-25, `pa_XwMin` 1e-10).

Running it under the other systems' profile (`settings_T12` in CTest) shows the settings
are a **cost** knob and not an accuracy one: native costs ~2.4× the iterations at the base
point (49 → 117, up to 12× at some pressures), AOP is unchanged (181 → 183), and every
phase amount and `G` agrees. AOP's failure above 95 bar is present under both profiles.
Note a profile is a coherent set — applying half of one gives a configuration native cannot
solve.

The `pa_DG` probe also **root-causes the volume bug**: native's EOS-fluid volume is exactly
inversely proportional to `pa_DG` over four decades (5.670275e-03 / -04 / -05 / -06 at
`pa_DG` 10 / 100 / 1000 / 10000) while AOP's is invariant. It is being reported in the
internal rescaled frame, never converted back — and happens to be right near `pa_DG` ≈ 160,
which is probably why it went unnoticed.

## Now in CTest — `proposed.aop`

`tests/test_proposed_systems.cpp` + `include/difftest/proposed_systems.h`. Three cases,
0.36 s total:

| case | scores | measured |
|---|---|---|
| `hoknob_T2b` | AOP's cost at the degenerate end of the H:O knob vs the open end | **97×** |
| `crossing_T11` | native and AOP peaking on **opposite sides** of the boundary | AOP 11.4× / native 7.8× / AOP 0.33× settled |
| `eh_determinacy_T5_T10` | Eh spread under a 1e-9 `bIC[O]` change, buffered vs unbuffered | **1.4e13×** |

Verified to discriminate: `test_proposed_systems native` (not registered as a CTest) fails
4 of the AOP-specific checks. `crossing_T11` also **asserts** the vestigial-phase
asymmetry — native holding 2.16e-5 mol at `logSI` = −0.0016 where AOP holds 1.35e-11 —
rather than tolerating it.

## Suggested next steps

0. **T11's crossing should be sampled by CO₂ titration at fixed P, not by the P axis.**
   Composition carries no grid tolerance, so it refines without limit and gives a
   perfectly monotone AOP penalty curve (344 → 3912 → 114 across the boundary). The P
   grid stays valuable for what only it can provide — a real pressure axis — but it
   cannot resolve the crossing finer than 1 bar, so T11's spec of 1e-5/1e-6 steps is
   unreachable that way and needs no re-export to be met.
1. **T2b and T11 are the two ready to adopt as CTest cases now.** Both give a monotone,
   attributable curve; both need only `proposed_sweep`, which is tracked. T2b's ratchet
   would be AOP's iteration count at the degenerate end against its count at the flat
   end; T11's would be the crossing-step ratio.
2. **T5 + T10 should be adopted as a pair** and scored on Eh spread under jitter — that
   is the T16 entry, and the pairing is what makes it meaningful.
3. **T5's 363.9 K AOP cliff deserves its own diagnosis** before it is ratcheted. 189 →
   10002 across 0.25 K on a system native solves in 61 is a sharper threshold than
   anything currently in the corpus.
4. **T1 needs no further work** — it has answered its question. Keep it as an additional
   member of the mass-balance family; do not build the trace-ladder variant, because the
   ladder has now been run and the trace class is not where the problem is.
