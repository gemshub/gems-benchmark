# `Resources/gems3k-psina` — the size ladder

28 projects. A controlled size ladder, 8 → 1566 species, used to characterize where AOP/Optima's
cost and reliability diverge from native as problem size grows. Two chemistry families:

- **`07PSIna_G_*`** — exported variants of one underlying "PSIna" system (`simple`/`edt`/`mid`/
  `complex`/`vcomplex` recipe complexity, mostly at 25 °C with a few 80/101/150 °C points), forming
  the original 8 → 1566-species ladder.
- **`T8_aq*` / `T14_ball*` / `T8ax2_nIC*`** — a different fixed chemistry, varied along one axis at
  a time (aqueous species count, "ballast" absent-solid count, and IC count respectively). `T8_aq*`
  is the original size-only ladder that calibrated the `kDimReduceAutoMinDC=200`
  dimension-reduction gate; `T14_ball*` and `T8ax2_nIC*` are the two newer (2026-09-07) ladders.
  **Read `tests/TEST-CATALOGUE.md` item 6b before using either of the newer two.**

All 28 are exercised by CTest's `ci.baseline` case (native + SIA modes,
`tests/ci-baseline.txt`) — the only case running the full directory. Individually, only
`T8_aq101`/`T8_aq201` (`dimreduce.gate`) and `T14_ball120` (`trace_phases.native`/`.aop`) have
dedicated CTest cases beyond that baseline.

| directory | what it is | `TEST-CATALOGUE.md` reference |
|---|---|---|
| `07PSIna_G_simple_2_0_1_25_0` | Smallest ladder rung, 8 species, 25 °C; AOP actually *faster* than native (0.3x). | size-ladder table, `07PSIna_G_simple_2` row |
| `07PSIna_G_simple_0_0_1_25_0` | 23-species rung, 25 °C; AOP 0.7x native cost. | size-ladder table, `07PSIna_G_simple_0` row |
| `07PSIna_G_simple_1_0_1_25_0` | 30-species rung, 25 °C; AOP 0.7x native cost. | size-ladder table, `07PSIna_G_simple_1` row |
| `07PSIna_G_edt_1_0_1_25_0` | 33-species rung, 25 °C; AOP crosses over to costing more than native (2.0x). | size-ladder table, `07PSIna_G_edt_1` row |
| `07PSIna_G_mid_1_0_1_25_0` | 265-species rung, 25 °C; AOP cost explodes to 34x native (4184 it), though it still converges. | size-ladder table, `07PSIna_G_mid_1` row |
| `07PSIna_G_complex_1_0_1_25_0` | 1392-species rung, 25 °C; native converges in 498 it, AOP **never** converges (40 min timeout) — a frozen-iterate failure, not a cost limit. | size-ladder table, `07PSIna_G_complex_1` row |
| `07PSIna_G_complex_1_0_1_80_0` | Same `complex_1` recipe at 80 °C; native OK, AOP no result — same frozen-iterate failure class. | "six more projects" table, `complex_1_0_1_80` entry |
| `07PSIna_G_vcomplex_0_0_1_25_0` | Largest main-ladder rung, 1566 species, 25 °C; native converges (646 it), AOP **never** converges. | size-ladder table, `07PSIna_G_vcomplex` row |
| `07PSIna_G_vcomplex_0_0_1_80_0` | Same `vcomplex_0` recipe at 80 °C; native OK, AOP no result — same frozen-iterate class. | "six more projects" table, `vcomplex_0_0_1_80` entry |
| `07PSIna_G_vcomplex_2_0_1_80_0` | A second genuine native-failure project — native itself fails `GEM_init`. Whether this is a true native failure vs. a model inconsistency is not yet established. | "six more projects" table, `vcomplex_2_0_1_80` entry |
| `07PSIna_G_edt_2_0_1_25_0` | 1566-species rung, same size as `vcomplex` but a genuinely different system (different DCH/DBR) — a size replicate, not a third size point. | size-ladder table footnote; "six more projects" list |
| `07PSIna_G_simple_0_0_0_101_0` | 23-species system at 101 °C; native OK (45 it) but AOP costs 60x native (2731 it) — worst ratio at that size. | "six more projects" table, `simple_0_0_0_101` entry |
| `07PSIna_G_simple_0_0_0_150_0` | Same system at 150 °C — the only temperature-isolated pair in the corpus; AOP degrades further to outright FAIL (1032 it). | "six more projects" table, `simple_0_0_0_150` entry |
| `T14_ball000` | Ballast ladder rung 0: one fixed bulk composition / 19 stable solids, 0 additional absent ("ballast") solid phases riding along as extra unknowns. `G` identical across all five rungs and six modes. | item 6b, `T14_ball{000,005,020,060,120}` bullet |
| `T14_ball005` | Same composition, 5 absent ballast solids added. | item 6b, same bullet |
| `T14_ball020` | Same composition, 20 absent ballast solids. | item 6b, same bullet |
| `T14_ball060` | Same composition, 60 absent ballast solids. | item 6b, same bullet |
| `T14_ball120` | Largest ballast rung (1287 species / 141 phases, 120 absent solids). Native drops 5 trace phases it is actually supersaturated with respect to (PSSC `SKIP_INFEASIBLE`, §77.6 in the GEMS3K repo's key-internals doc), AOP finds all five — its real value is this assemblage-loss defect, not the cost curve. | item 6b bullet; also `trace_phases.native` / `trace_phases.aop`, built specifically around this project |
| `T8_aq101` | 130 aqueous species (61 IC / 21 phases fixed); straddles the `kDimReduceAutoMinDC=200` gate from below. Same `G` as other rungs to 9–10 digits. | item 6; CTest `dimreduce.gate` |
| `T8_aq201` | 230 aqueous species; straddles the 200-species gate from above, paired with `aq101` to calibrate it. | item 6; CTest `dimreduce.gate` |
| `T8_aq401` | ~430-species rung of the size-only ladder; part of the AOP-cliff curve (51–130x native cost region). | item 6 |
| `T8_aq801` | ~830-species rung; near the top of the AOP-cliff curve (up to 130x native). | item 6 |
| `T8_aq1138` | Largest size-only rung, 1167 aqueous species; AOP cost blows out to 2053x native, HOP stays roughly immune (2.2–5.1x). | item 6 |
| `T8ax2_nIC17` | IC-count ladder rung: DC pinned at exactly 130, only 18 ICs survive selection (nominal 17 + trace `B`). | item 6b, `T8ax2_nIC{17,25,35,45,61}` bullet |
| `T8ax2_nIC25` | Same 130-species system, 25 ICs. | item 6b, same bullet |
| `T8ax2_nIC35` | Same 130-species system, 35 ICs. | item 6b, same bullet |
| `T8ax2_nIC45` | Same 130-species system, 45 ICs; matches `nIC61` in species/`G` to ten digits — used as a control pair. | item 6b, same bullet |
| `T8ax2_nIC61` | Same 130-species system, 61 ICs; uniquely lacks `H+` and reports pH exactly 0. | item 6b, same bullet |
