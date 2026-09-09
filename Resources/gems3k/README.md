# `Resources/gems3k` — the main regression corpus

27 projects (26 loadable). Native and AOP both solve 26 of them; `f_PitzerTHE_G_KCaSO4` fails
`GEM_init` for every mode (file-read error, present since 2026-07-28 — not investigated further).
Every loadable project is exercised by CTest's `ci.baseline` case (`tests/ci-baseline.txt`); 18 of
the 27 are also named individually inside `optima_regression.{aop,rop}`
(`include/difftest/optima_regression.h`). Full detail for any named case lives in
`tests/TEST-CATALOGUE.md`.

**Prefix letter = thermodynamic-data backend, not a format choice.** `f_`/`j_`/`o_`/`t_` are the
four `io_mode` export values (`f_`=JSON+ThermoFun, `j_`=JSON+DATACH grid, `o_`=text/key-value+
ThermoFun, `t_`=text/key-value+DATACH grid). An `f_`/`j_` (or `o_`/`t_`) pair is a
**thermodynamic-data A/B of the same system** — CLAUDE.md §3: "never pool them," they can differ by
more than any solver change under measurement. Pairs are listed together below unless they genuinely
diverge (`f_`/`j_CASHNK` do — see its row).

| directory | what it is | `TEST-CATALOGUE.md` reference |
|---|---|---|
| `f_CalcDolo_G_CalcColumn_0_0_1_25_0` / `j_CalcDolo_G_CalcColumn_0_0_1_25_0` | Calcite/dolomite column system, 7 IC / 28 DC / 7 phases. | `f_CalcDolo` is the sole project in `optima_regression.rop` (liveness smoke case); both rows also in `optima_regression.aop` and `ci.baseline`. |
| `f_CASHNK_G_Chen04C-3T_0_0_1_25_0` / `j_CASHNK_G_Chen04C-3T_0_0_1_25_0` | Cement C-A-S-H: two interchangeable Berman multi-site solid solutions, 6 IC / 35 DC / 6 phases. **The `f_`/`j_` pair genuinely differs here** — answers 0.14 J apart, traced to 63 duplicated symbols between the ThermoFun and DATACH-grid databases (see `key-internals.md` in the GEMS3K repo). | `j_CASHNK` is the only case exercising the phase-extinction retry and its stability-check exemption; corner-case table row "Two interchangeable phases"; both rows in `ci.baseline` (`f_` jitter 5.2x, `j_` 135.8x — the corpus's most jitter-unstable count). |
| `f_ClaySorMo_G_TestUO2I001_7_0_1_25_0` / `j_ClaySorMo_G_TestUO2I001_7_0_1_25_0` | Clay surface-complexation / sorption model, small UO2 sorption system, 11 IC / 44 DC / 4 phases. | No dedicated named case beyond `optima_regression.aop` and `ci.baseline`. |
| `f_Flowline_G_series1_0001_0_1000_400_1` / `j_Flowline_G_series1_0001_0_1000_400_1` | 1000 bar / 400 °C flowline state point, 8 IC / 51 DC / 20 phases, one solution phase. | `optima_regression.aop`, `ci.baseline`; cited (not registered) in the absent-phase-count measurement: 15 absent phases → AOP 1.15x native. |
| `f_GEOTHERM_G_Test1_0_0_1_75_0` / `j_GEOTHERM_G_Test1_0_0_1_75_0` | Geothermal fluid/mineral system, 25 IC / 154 DC / 112 phases — largest phase count in the pair-covered set. | `optima_regression.{aop,rop}` — the corpus's only project where ROP exhausts its fixed budget and fails while AOP converges; dominates `optima_regression.aop`'s runtime; `ci.baseline` calls it "the corpus's most expensive native solve (~690 it) that is still stable". |
| `f_Kaolinite_G_pHtitr_0_0_1_25_0` / `j_Kaolinite_G_pHtitr_0_0_1_25_0` | Kaolinite pH-titration state point (distinct recipe from the `o_/t_` variant below). | `f_Kaolinite` in `optima_regression.aop`; both rows in `ci.baseline` (jitter 5.9x/8.0x). |
| `o_Kaolinite_G_pHtitrKa_0_0_1_25_0` / `t_Kaolinite_G_pHtitrKa_0_0_1_25_0` | Kaolinite pH-titration, "Ka" recipe — ships a `DUL=DLL=0` exclusion on Quartz. | The corpus's canonical corner case ("a phase formally excluded" — yes, Quartz here); reused by both `MetastabilityCase`s inside `optima_regression` with a 100000x-wider `dG` tolerance because AOP finds a genuinely different, lower local optimum (quartz vs native's metastable SiO2(am)); also `optima_regression.aop`, `ci.baseline`. |
| `f_PitzerTHE_G_KCaSO4_10_0_0_100_0` | Pitzer K-Ca-SO4 system. **Broken** — fails `GEM_init` for every mode since 2026-07-28 (own `-fun.json` differs from the working `j_` sibling; not investigated further). | Explicitly excluded from `ci.baseline` ("cannot be read at all since 2026-07-28"); absent from `optima_regression`. |
| `j_PitzerTHE_G_KCaSO4_10_0_0_100_0` | Pitzer K-Ca-SO4 at 100 °C, 8 IC / 36 DC / 24 phases, one Pitzer aqueous phase, rest pure salts. Its `f_` twin is broken, so this is the only working half. | `optima_regression.aop`; `ci.baseline`. |
| `f_Solvus_G_series1_0001_0_500_400_0` / `j_Solvus_G_series1_0001_0_500_400_0` | Sanidine–albite ternary feldspar solvus, 0.5 kbar, below the miscibility gap. | `j_Solvus_G_series1` drives `solvus.native`/`solvus.aop`/`solvus.critical` (400–700 °C sanidine-albite miscibility-gap sweep, 301 points, scored against a GEM-Selektor GUI reference); both rows also in `optima_regression.aop`, `ci.baseline`. |
| `f_Solvus_G_Test1_0_0_1000_400_0` | Single ternary feldspar (no interchangeable twin), 400 °C / 1000 bar, ThermoFun-backed; built to exercise a phase dissolving slowly under a warm restart (`pa_MbTrendPhaseDecay`). | Not in any registered CTest case — explored by hand via `decay_probe`. Present only in `ci.baseline`. |
| `o_Solvus_G_series2_0002_0_1000_640_0` / `t_Solvus_G_series2_0002_0_1000_640_0` | Same sanidine–albite–anorthite system, a third state point: 1000 bar / 640 °C, inside the miscibility gap, text-format export. | `optima_regression` as `o_Solvus640`/`t_Solvus640`; `ci.baseline` (only coverage of the TEXT export reader alongside `o_Kaolinite`). |
| `f_TestPNTDB_G_Neutral_0_0_1_75_0` / `j_TestPNTDB_G_Neutral_0_0_1_75_0` | Large system, 42 IC / 690 DC / 126 phases — the largest fully-covered project in the corpus. | `f_`: `large_f_TestPNTDB` (first cold AOP solve above the dimension-reduction gate) and `warm_f_TestPNTDB` (native-only warm-restart case). `j_`: `fallback_j_TestPNTDB` (forces the dimension-reduction fallback/rescue path). Both in `optima_regression.aop`, `ci.baseline`. |
| `f_TestSUP98_G_TestSystem_0_0_1_25_0` / `j_TestSUP98_G_TestSystem_0_0_1_25_0` | Very large system, 82 IC / 923 DC / 145 phases. | Deliberately excluded from `optima_regression`/AOP convergence tests — ships its own `pa_OptimaMaxSeconds` skip guard; AOP produces no result even at 300 s ("considered and NOT worth adding"). Present only in `ci.baseline` (jitter 1.3x/1.8x). |
| `j_10TH_G_seawater_1_0_1_25_0` | 12-component THEREDA/Pitzer seawater brine, 12 IC / 122 DC / 87 phases, 3 solution phases. No `f_` counterpart. | Base fixture for `evaporation.aop` — 49 cold-started evaporation steps, scored on mass/ionic-strength monotonicity and precipitation order (halite/carbonates/celestite); one of the few projects with a usable T-grid (81 points). `ci.baseline`. |
| `j_TiQ_PRSV_G_MySystem_0_0_10000_900_0` | Dry, high-T mineral+gas assemblage with **no aqueous phase at all** (PR78 cubic-EOS fluid), 10 kbar / 900 °C, 4 IC / 6 DC / 3 phases. No `f_` counterpart. | Corner-case table row "no aqueous phase at all"; `optima_regression.aop`; one of `volume.native`'s five curated rows — the fixture the 2026-09-04 volume defect was found on. |

Three projects (`f_Solvus_G_Test1`, `j_10TH_G_seawater`, `j_TiQ_PRSV_G_MySystem`) are not part of the
automated GEMSGUI export set — added by hand. `f_ClaySorMo`/`j_ClaySorMo` have no description beyond
directory-name/size inference plus the one-line source comment in `ci_baseline.h` — nothing further
was found in `Docs/` or `SESSION_LOG.md`.
