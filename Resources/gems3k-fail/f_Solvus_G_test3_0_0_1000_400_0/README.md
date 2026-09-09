# `f_Solvus_G_test3_0_0_1000_400_0`

Sanidine-albite-anorthite solvus, **two** ternary feldspars (`Alkali feldspar`, `Plagioclase`),
9 IC (`Al Ca Cl H K Na O Si Zz`), 60 DC, 21 PH, ThermoFun-backed (`-f`), at
**673.15 K / 1e8 Pa (400 C / 1000 bar)**.

Supplied by Dmitrii on 2026-09-02 as `Resources/gems3k/test3/`; renamed here to the corpus
convention, which is the GEMS record key verbatim - `Solvus  G  test3  0  0  1000  400  0` - and
moved to **`Resources/gems3k-fail/`**, which is where projects that native cannot solve live.
Nothing in the exported data was changed; only the file prefixes and the two `.lst` files that name
them.

Same chemical system as `Resources/gems3k/f_/j_Solvus_G_series1` (500 bar / 400 C) and
`o_/t_Solvus_G_series2` (1000 bar / 640 C) - a third state point, 1000 bar / 400 C. Distinct from
`Resources/gems3k/f_Solvus_G_Test1_0_0_1000_400_0`, which is the same state point with ONE feldspar
(57 DC / 20 PH) and exists for the phase-decay work. Those four live in `gems3k/` because native
solves them; this one does not, hence `gems3k-fail/`.

## Why it is worth keeping: native diverges here, AOP does not

Reported symptom (Dmitrii): in GEMS-GUI this system gives a **divergence in the dual solution** and
only works if a little H2 is added (0.0001 g). Reproduced exactly, and quantified:

| | native (AIA) | AOP | ROP |
|---|---|---|---|
| as exported | **FAIL** - `W14IPM: IPM Main Descent: Divergence in dual solution approximation (u)`, 72 it | **OK, 115 it** | **OK, 115 it** |
| `+ 0.0001 g H2` (`bIC[H] += 9.92e-5 mol`) | OK, 90 it | OK, 116 it | OK, 116 it |

With the H2 added, **all three modes agree to every printed digit** (pH 4.847974, Eh -0.360425,
`Vs = 4.709112e-05`, `Ms = 9.578953e-02`, `G = -2.474668290e+02`). Without it, AOP/ROP still converge
and land on essentially the same state - pH 4.847953 against 4.847974, `G` -2.474662873e+02 against
-2.474668290e+02 - i.e. **AOP's unassisted answer is the one the H2 workaround produces**, modulo the
tiny composition change. The one coordinate that differs a lot is `Eh` (+0.242187 without H2,
-0.360425 with), which is the point - see below.

This makes it the second project on record where AOP solves what native cannot
(`3Bent-H2O_G_Mont` is the first), and the first where the failing case is an ordinary,
well-posed system somebody actually wants to run.

## The mechanism, measured: Eh is undetermined without a redox buffer

The aqueous phase carries `H2@` and `O2@` among its 36 species, but at the exported composition
neither is present at a meaningful level, so nothing pins the charge/redox row. `mb_class_probe
--jitter=9` (nine cold solves under a ~1e-15 relative `bIC` perturbation):

| | native converged | Eh spread | pH spread | G spread |
|---|---|---|---|---|
| as exported | **3 / 19** | **2.007e-02 V** | 2.197e-07 | 5.641e-10 |
| `+ 0.0001 g H2` | **19 / 19** | **1.801e-08 V** | 1.378e-07 | 6.850e-12 |

A **million-fold** change in Eh determinacy from 5e-5 mol of H2, with pH and G essentially unchanged
in both. So this is not a solver defect in the ordinary sense: pH and `G` are solidly determined
(2e-7 and 6e-10) while `Eh` is not, and native's dual divergence is what an unconstrained redox row
looks like from inside IPM. Adding H2 supplies the buffer that pins it.

Same family as `Resources/gems3k-fail/07PSIna_G_iron`, where the worst rung of a trace-Fe ladder is
exactly the composition at which `H2(aq)` collapses to 8.7e-27 with `O2(aq)` still 0.

## This is the T16 "Eh-determinacy pair" the plan asked for

Plan v5 lists T16 as a wanted export: one system with a real redox couple at meaningful abundance and
one deliberately without. **This project plus its `bIC[H] += 9.92e-5` variant is that pair**, on one
chemical system with everything else bit-identical - which is stronger than two separate exports.
The variant needs no new file: add `2 * (1e-4 / 2.01588)` mol to `bIC[H]` in the `-dbr`.

Useful because several conclusions in `Docs/gems3k-optima-plan-v5.md` turn on "was that Eh
difference real or was Eh undetermined", and until now that could only be answered by a scratch tool.

## Caveats before using it as a fixture

- **Do not assert on `Eh` as exported.** It is the undetermined coordinate; assert on `pH`, `G`,
  `Vs`, `Ms` and the feldspar limb compositions instead.
- **Native FAILs as exported**, so any test must either expect that or use the H2 variant.
- Not yet added to `ctest`. The obvious use is a determinacy assertion (jitter spread above/below a
  threshold for the two variants) rather than another convergence case.
