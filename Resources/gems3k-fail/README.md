# `Resources/gems3k-fail` — projects native finds hard

17 projects. Per `CLAUDE.md` §3, all of them exist because **native** finds them hard, except
**`T-cement`**, added 2026-09-08 for the opposite reason — native solves it in ~12 ms while every
Optima mode costs 25–60 s per point. Full detail for any named case lives in
`tests/TEST-CATALOGUE.md`.

Most directories carry their own `info.md`. Where present, it usually has two parts: a
GEM-Selektor sweep macro at the top (the axis a hand-driven sweep was run over — not a
description), and, after a `---` separator, an actual markdown writeup. Three directories
currently have no `info.md`: `10TH_G_00001_0_0_1_25_0`, `3Bent-H2O_G_MySystem_1_0_1_25_0`,
`f_Solvus_G_test3_0_0_1000_400_0`.

| directory | what makes it hard / notable | `TEST-CATALOGUE.md` reference |
|---|---|---|
| `07PSIna_G_iron_1_0_1_25` | Trace-Fe(II/III) log-dose ladder; pervasively triggers harmless MBR "stall" warnings but `GEM_run()` always reports success — a cosmetic-only stress case. | `ci.baseline` (Group 2, native/SIA rows) |
| `07PSIna_G_ironsi_1_0_1_25` | Same Fe ladder as `_iron` but with `FeOOH(alpha)`/`Fe2O3(alpha)` competing for iron; genuinely fails 4/61 sweep steps (`W08IPM: PSSC` inconsistency); disabling stall detection shifts which steps fail rather than revealing a superset. | `ci.baseline` (Group 2, native/SIA rows) |
| `10TH_G_00001_0_0_1_25_0` | Large THEREDA invariant-point brine (2031.74 mol, 27 DC / 14 phases); its 1e-13 *absolute* `pa_DHB` tolerance is knife-edge relative to system size — native's converged/failed verdict flips under a 1e-15 `bIC` jitter, where AOP's residual sits 2.25e6x tighter. | CTest `mb_10TH_THEREDA` (mass-balance-residual check, Optima-only) and `shp_10TH_THEREDA` |
| `3Bent-H2O_G_Mont_0_0_1_25_0` | Bentonite pore-water system; fails immediately as shipped (`E07IPM: Degeneration in R matrix`). AOP later solved it by dropping an unstable montmorillonite (logSI gap 1.734) — a model inconsistency both solvers ultimately agree on, not a rescuable solver bug. | none found |
| `3Bent-H2O_G_MySystem_1_0_1_25_0` | Sibling bentonite system, fails immediately (`W08IPM: PSSC`). Unlike `_Mont`, remains unsolved by native, AOP *and* ROP — a continuation walk shows no consistent equilibrium exists beyond ~0.45% of its shipped solute loading, independent of solver/anchor/step count. | none found |
| `Al-species_G_sys_2_0_0_101` | Temperature sweep, 25–200 °C; converges everywhere at default `pa_DK`, but shows scattered, non-threshold failures once `pa_DK` is tightened 100x. | CTest `fail_Al-species` (also `volume.native`, "pinned by size") |
| `CASH+CsSr_G_sys_0_0_1_25_0` | CASH+ cement sibling, Ca(OH)2-dose sweep (0.02–0.25 mol); converges across the full 231-step sweep despite near-universal MBR stall warnings. | CTest `fail_CASH+CsSr` |
| `CASH+_G_csh_sol` | Cement CSH solid-solution system; converges cleanly at default settings but fails ~99/231 sweep steps in a still-unexplained scattered pattern once `pa_DK` is tightened 10x — AOP needs 16x fewer iterations where both converge. | CTest `fail_csh_sol` (also `ci.baseline`); candidate second `sweep_regression` case, not yet built |
| `CSHSnplus_G_CSH1_5_bufs` | Sn-doped CSH system; converges across its log-dose sweep except one isolated MBR-iteration-limit failure — the corpus's deliberate "AOP is *worse*" counter-case (185 → 1298 iterations; ROP never converges). See also plan v5 §101's cold-Optima interaction with `pa_OptimaEarlyStabilityAt`. | CTest `fail_CSHSnplus` |
| `Cu-Pourbaix_G_pHtitr` | Copper Pourbaix pH-titration, 121 steps; 61/121 fail natively in a scattered pattern centered on the natural equilibrium pH, which breaks the accompanying Python bisection tool's monotonicity assumption. AOP converges on all 121. | `sweep_regression.aop` (full titration sweep) and CTest `fail_CuPourbaix` |
| `FeNaCl_FyGt_Precip` | Fe(OH)3/goethite precipitation-**allowed** variant from a real LHS-sampled user script; sweeping Fe logarithmically near the solubility onset, 8/51 steps fail natively (genuine convergence difficulty, not a warm-start artefact). Reproduction needs `--cold-every-step`. | none found |
| `FeNaCl_FyGt_Precip_HighpH` | Was a byte-identical duplicate of `_Precip` until repaired 2026-09-05b (added the 10^0.5 m NaOH its own recipe calls for); now genuinely high-pH (11.41→14.29) and native collapses outright while AOP converges in 43–62 iterations. | none found |
| `FeNaCl_FyGt_SI` | Ferrihydrite/goethite saturation-index variant (precipitation suppressed, `DUL=0`); NaOH sweep near pH≈14.47, 1/16 steps fail — matches the original 1-in-10000 sample rate. Reproduction needs `--cold-every-step`. | CTest `fail_FeNaCl_SI` |
| `FeNaCl_FyGt_TransitionZone` | Same Fe-suppressed family, wider high-pH NaOH sweep (1.2–3.2 mol); failures turn dense and scattered above pH≈14.11 with no clean threshold, 38–57/101 steps fail depending on cold/warm start. | CTest `fail_FeNaCl_TZ` |
| `f_Solvus_G_test3_0_0_1000_400_0` | Two-feldspar solvus at 400 °C / 1000 bar; native diverges (`W14IPM` dual-solution divergence) while AOP/ROP converge — root cause is an *undetermined* Eh (unbuffered redox), not a true solver defect. Doubles as the Eh-determinacy pair with `T16` in `gems3k-proposed`. | CTest `det_test3_asexported` / `det_test3_h2buffered` |
| `LBE-6_G_M1-6_HSCdb` | Pb/Bi-eutectic system, no aqueous phase at all. Its `info.md` temperature sweep (0–1000 °C) found a real ThermoFun `Cp`-extrapolation SIGSEGV at 410 °C, since fixed upstream. | `ci.baseline` (Group 2, native/SIA rows) |
| `T-cement` | Portland-cement hydration, water/solid ratio sweep (1–50 g water vs. fixed 90.9 g clinker). **The opposite case in this corpus**: native solves in ~12 ms while every Optima mode costs 25–60 s/point, so no Optima CTest case is registered. See its own `info.md` for the re-export history (missing Mg/K hosts). | `cement_water.native` / `cement_water.SIA` |
