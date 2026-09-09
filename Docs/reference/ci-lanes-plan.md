# Plan — three lanes, and reconciling them with what `develop` already proposes

*Written 2026-09-08. This is a PLAN: proposed work, with what is decided and what is still open
marked as such. What already exists is in `Docs/reference/ci.md`; this file is the part not built
yet, and the argument for the shape.*

---

## 1. The split

Different questions get asked of this repo, and until now they have been answered by overlapping
tools with no stated boundary. There are **four**, not three, because the fixture question splits
according to whether the stored result is still current.

| # | question | reference it compares against | instrument | verdict | budget |
|---|---|---|---|---|---|
| **S** | did the **solver** change? | a separate **frozen state file** | `tools/ci.sh` → `ci.baseline`, `volume.native` | **hard gate** | ~30 s |
| **F1a** | did the **system definition** change? | the **previous export** | fresh `gems3k-export` vs `Resources/gems3k`, via `compare_dirs` | **report** — not built | minutes |
| **F1b** | does the solver **reproduce what the exporter wrote**? | the values **stored in the fresh file** | a round trip on the GUI side | GUI-side check | seconds |
| **B** | how fast / how robust? | itself, and the freeze | benchmark lane — the freeze, `collect_metrics`, the sweeps, `iteration_jitter` | exploratory, never CI | minutes to hours |

**S** and **B** exist. **F1a** is planned (§4). **F1b** is only valid on a *fresh* export and
belongs where the export is produced — §2 measures why it cannot be run against this repo's corpus.

**Why the fixture question must be a report and not a gate.** A database or exporter change is a
legitimate event, not a regression. If it turns a build red, the pressure is to re-baseline, and
re-baselining is precisely how a bad export gets absorbed — `CLAUDE.md` §4's T-cement case, where a
defective export was nearly ratcheted into CTest before anyone looked, and the later fix would have
read as a regression against numbers that were themselves wrong. A report saying *these 4 of 24
systems moved, here is how* is the artefact that makes someone look.

## 2. What `develop` already proposes, and how it lines up

`README.md` on `develop` documents a complete process, and `.github/workflows/run-linux.yml`
automates it:

```sh
./recalc_all    gems3k gems3k-v4.5.5     # re-solve every project into a versioned directory
./collect_metrics             gems3k-v4.5.5     # iterations, convergence, speed, cold vs warm
./compare_dirs  -d -r -t ".*-dbr-[\d-]*.*" -rd dbr_diff.json -a 0.1e-6 -j gems3k -j gems3k-v4.5.5
./param_sensitivity_test  gems3k/j_TiQ_PRSV_.../...-dat.lst
```

### The distinction that decides the design: is the stored result CURRENT?

Every exported project carries its own answer inside its `-dbr` file — `IterDone`, `Gs`, `Vs`,
`Ms`, `pH`, `pe`, `Eh`. So it *looks* as though a reference has been in the repository all along,
and `recalc_all` + `compare_dirs` looks like a ready-made "did the answer change" gate.

**It is not, and the reason is that a stored result is only trustworthy while it is fresh.** There
are two genuinely different situations, and they need different references.

#### Mode 1 — a FRESH export (the GUI side)

The GUI has just computed the system, so the values it wrote are current *by construction*. Two
checks are available and both are meaningful:

| | check | answers |
|---|---|---|
| 1a | the exported **files** against the previous export | did the system DEFINITION change — species, phases, bulk composition, settings? |
| 1b | **recalculate and compare against the values stored in the file** | does GEMS3K reproduce what the GUI wrote? |

**1b is a round trip, and it is only valid here.** It is the check the GUI itself effectively
performs, and it is worth having on a fresh export because a mismatch means the export and the
solver disagree about the same system — a defect in the export, not a solver regression.

#### Mode 2 — the PRE-EXPORTED corpus in this repo

`Resources/*` holds files exported at various times over the project's history. **The results
stored in them are not current, and mostly do not reproduce.** Measured 2026-09-08 over the 66
recorded projects, native, comparing each shipped `-dbr` against a fresh solve:

| | |
|---|---|
| projects with a JSON `-dbr` | 62 (4 in text/key-value formats, not compared here) |
| stored `Gs` reproduces a fresh solve (rel ≤ 1e-9) | **18** |
| stored `Gs` differs | **30** |
| no `Gs` stored at all | **14** — the whole `gems3k-psina` set |
| stored `IterDone` == fresh `ITG` | **21** (differs 27, absent 14) |

The largest gaps are not subtle:

| project | ΔG rel | `IterDone` stored → fresh |
|---|---|---|
| `j_CASHNK_G_Chen04C-3T` | 1.85e-04 | **5818 → 82** |
| `f_CASHNK_G_Chen04C-3T` | 1.61e-04 | **5818 → 115** |
| `CASH+_G_csh_sol` | 5.43e-05 | 117 → 86 |
| `f_Kaolinite_G_pHtitr` | 1.02e-06 | 95 → 273 |

`j_CASHNK`'s stored 5818 iterations against today's 82 is a record from a different solver era
entirely. **So mode 1b applied to this corpus would report a difference on 44 of 62 projects, none
of them a regression.** That is the concrete argument for the freeze file.

> **Therefore: for pre-exported files the reference must be a SEPARATE frozen state file, compared
> against new calculations — not the values stored in the fixtures.** That file is
> `tests/ci-baseline.txt`, and this is its justification.

#### Two further reasons the stored values cannot serve as the reference

1. **`Resources/dbr_diff.json` marks `IterDone` and `NodeStatusCH` `ignored`** — the iteration
   count and the solver status, exactly what the CI lane was asked to check. `compare_dirs` was
   built to compare *exports*, and its configuration says so.
2. **The stored result is also the INPUT.** The `-dbr` holds the speciation a warm (SIA) call
   starts from. So it cannot be freely re-recorded — refreshing it to reflect a new answer also
   changes what every warm restart begins from. A reference that cannot be updated without
   changing the experiment is not a reference.
   **This is why `ci.baseline`'s SIA leg runs a cold AIA first and re-solves from that**, rather
   than warm-starting from the shipped `xDC`. Given the measurement above, warm-starting from those
   files would on most projects be measuring how stale they are.

Two more, about the pipeline rather than the data: `run-linux.yml` fails on nothing (it writes a
log and uploads a zip), and `recalc_all` **writes into the working tree** — it re-solves `gems3k/`
into `Resources/gems3k-<version>/`. A gate must not mutate what it measures.

**Conclusion, and it is not "replace it".** `compare_dirs` is the right tool for **mode 1a**, and
its own config says so by ignoring the solver's outputs. `ci.baseline` owns **mode 2**, where the
curated record, the jitter basis and the assemblage set belong. **Mode 1b belongs on the GUI side**,
against a fresh export, and is not this repo's job.

### `param_sensitivity_test` — documented, native, and in no test suite

The fourth item in the README is a genuine regression case: the Ti-in-Quartz project run twice with
different QtzRu Margules parameters, asserting both converge *and* that the Rutile end-member mole
fraction actually moves — i.e. catching a parameter change that produces no measurable effect.

Checked 2026-09-08: it uses `NEED_GEM_AIA` / `NEED_GEM_SIA` only and is **not** Optima-linked in
`tools/CMakeLists.txt`. So it meets the CI label's rule already. It is a tool, not a test — nothing
asserts its result in CTest. **Promoting it is the cheapest coverage gain available** (section 5).

---

## 3. Where `deploy-linux` fits

GEMSGUI's `deploy-linux.yml` ends with:

```sh
cd benchcomp
./gems3k-export -d -s . -p template_export.json -e <export-dir>/gems3k
```

and ships the result as a release asset. Measured 2026-09-08, that export regenerates
**24 of the 27 projects in `Resources/gems3k`** — an exact match, nothing extra:

| | |
|---|---|
| exported by GEMSGUI deploy | 24 |
| matched in `Resources/gems3k` | **24** |
| in the corpus, not exported | 3 — `f_Solvus_G_Test1`, `j_10TH_G_seawater`, `j_TiQ_PRSV` (added by hand later) |
| exported, not in the corpus | 0 |

The `io_mode` values `-j` / `-f` / `-o` / `-t` in `template_export.json` **are** the `j_` / `f_` /
`o_` / `t_` prefixes. So GEMSGUI's deploy is where most of this corpus comes from, which is what
makes the fixture question worth asking at all.

One more fact that decides the sequencing: GEMSGUI's environment takes **`gems3k>=4.5.2` from
conda-forge**, not from source. In that workflow the library is the *packaged release*.

---

## 4. Plan B — the export-comparison step

**Goal.** Answer **F1a** — *did the system definition change?* — as a readable report, on the
artefact GEMSGUI's deploy already produces.

**Scope, explicitly.** This is a **file-vs-file** comparison of two exports. It does **not**
recalculate, and it does **not** compare against the values stored in the fixtures:

* recalculating and scoring is **S**, and `ci.baseline` already does it against a frozen file;
* comparing against the *stored* values is **F1b**, which is only valid while the export is fresh
  and would report a difference on **44 of 62** of this repo's projects, none of them regressions
  (§2). If it is wanted, it belongs on the GUI side, immediately after `gems3k-export`.

Mixing them is how the two failure modes become indistinguishable in the one column that matters.

**Shape.**

1. **A script, `tools/export_compare.sh`**, taking two directories:
   `tools/export_compare.sh <fresh-export-dir> [<reference-dir>]`, reference defaulting to
   `Resources/gems3k`. It runs `compare_dirs` with the repo's own `dbr_diff.json` rules and writes a
   summary plus the full diff.
   - It **never writes into `Resources/`**. `recalc_all`'s versioned-output habit stays in the
     benchmark lane where it belongs.
   - It compares the **shipped** export against the **fresh** one. It does not re-solve anything —
     re-solving is the solver question, and mixing them is how the two failure modes become
     indistinguishable.
2. **Report both halves, because they mean different things:**
   - *systems present/absent* — the export template gained or lost a system. Structural, cheap to
     see, and the thing most likely to surprise.
   - *field differences per system* — with `compare_dirs`' per-field tolerances.
3. **Exit status: 0 unless the comparison itself failed to run.** Differences are content, not
   failure. The step publishes the report; a human decides.
4. **A second rules file, `Resources/dbr_export_diff.json`**, is probably needed rather than reusing
   `dbr_diff.json` unchanged. Open question, see §6: for an export comparison `IterDone` and
   `NodeStatusCH` are arguably *informative* (a re-export that changes the iteration count changed
   the system), where for the original purpose they were noise. Do not edit `dbr_diff.json` in
   place — it is committed and other things use it.
5. **Wiring, in order of increasing commitment:**
   - a. by hand, against a downloaded `gems3k-GEMS<version>` release asset — no CI change at all;
   - b. a `workflow_dispatch` job in this repo that takes an artifact URL;
   - c. a step in GEMSGUI's `deploy-linux.yml` after `gems3k-export`, checking out gems-benchmark
     and running the script — needs a decision from that repo's owner, so it is last.

**What it does NOT do, and must not.** Run `ci.baseline` against the fresh export. Its record is
keyed to the checked-in fixtures, so every legitimate database change would surface as an `ANSWER`
finding, indistinguishable from a solver regression — and the natural response, re-recording, would
convert *"the fixtures moved"* into silence.

---

## 4b. One comparator, two reference sources — BUILT 2026-09-08

**Proposed (owner, 2026-09-08):** `ci.sh` should compare against the **frozen state file** for
pre-exported files, and against **the results stored inside the system files** for freshly exported
systems.

**Built.** `ci_baseline --against frozen|stored`, plus `--systems DIR`. What follows is the
design and what using it found.

**Why it unifies rather than duplicates.** `include/difftest/ci_baseline.h`
already does two things — turn a solve into a `Row`, and score one `Row` against another. Reading a
`Row` out of a stored `-dbr` is simply a *third* source of the same struct, alongside "solve it" and
"parse the record file". Sharing one comparator keeps one tolerance set and one severity ranking,
which `CLAUDE.md` §6 requires of two tools scoring the same files.

It also closes a real gap: a newly exported system in `Resources/under_review` currently has
**nothing** checking it, and `CLAUDE.md` §4 says such a project is worked by hand. This gives that
hand-work an instrument, on the only reference such a system can have.

```
ci_baseline --against frozen                       # default: tests/ci-baseline.txt
ci_baseline --against stored --systems DIR         # fresh export: the -dbr's own values
```

### What a stored `-dbr` can and cannot serve as a reference

Measured on `j_CalcDolo`'s `-dbr-0-0000.json`, 2026-09-08:

| field | usable? | |
|---|---|---|
| `Gs`, `Vs`, `Ms`, `pH`, `pe`, `Eh` | **yes** | genuine results; `Gs` matches `Get_GibbsEnergy()` to every digit |
| `xPH[nPH]` | **yes** | phase amounts — the assemblage is reconstructible, so the ANSWER class works in full |
| `xDC[nDC]`, `gam[nDC]` | **yes, and this is a GAIN** | per-species amounts and activity coefficients — coverage the frozen `Row` does not have |
| `IterDone` | **report only** | see condition 3 |
| `NodeStatusCH` | **NO** | see condition 2 |

### Three conditions, each from a measurement rather than a preference

1. **The mode must be explicit, never inferred.** On this repo's corpus the stored values differ
   from a fresh solve on **44 of 62** projects (§2). A tool that guessed "stored" there would emit
   44 findings, none of them real. So `--against frozen` is the default, `--against stored` is
   opt-in, and `stored` on a project that *has* a frozen row should be refused unless forced.

2. **STATUS cannot be scored in stored mode — the field is an input, not a result.**
   `NodeStatusCH` in the shipped file reads **`1` = `NEED_GEM_AIA`**: the code asking for a cold
   solve, not the `OK_GEM_AIA` (`2`) that comes back. The exporter writes what the node should be
   *asked* to do. This is exactly why `Resources/dbr_diff.json` marks it `ignored`. So the class at
   the **top of the severity ranking in frozen mode has no counterpart in stored mode**, and the
   report must say so rather than silently compare an input against an output.

3. **Iterations are report-only in stored mode**, for two reasons that agree. `IterDone` is a real
   result, but it came from whatever GEMS3K the *exporter* linked — comparing it to a fresh `ITG`
   is only sound if that is the same build. And a freshly exported system has **no jitter
   measurement**, so its cost could not be pinned in any case: `Case::costPinnable` is false for a
   project nobody has measured. Print it, do not score it, until both are settled.

### How it is implemented — no JSON reader, and no format switch

The obvious implementation is a JSON parser for the `-dbr`. The better one falls out of how GEMS3K
already works: **`GEM_init()` loads the `-dbr` into `CNode`, and `unpackDataBr()` — which would
overwrite it from the solver's own state — is called inside `GEM_run()`, not `GEM_init()`.** So
immediately after init the node *holds the file's stored results*, and the same accessors the solve
path uses read them back: `Ph_Moles(k)` is `CNode->xPH[k]` (`node2.cpp:812`), `cVs()`/`cMs()` are
`CNode->Vs`/`Ms`.

Two consequences, both good:

* it works for **every export format the reader supports**, text ones included — which matters,
  because 4 of the corpus's 66 projects are not JSON at all and a parser-based version silently
  skipped them;
* the stored and the fresh row are built by the *same* code from the *same* fields, so they cannot
  drift apart in units or meaning.

Only two fields bypass an accessor, deliberately: `CNode->Gs`, because `Get_GibbsEnergy()` routes
through `multi_ptr()->TotalGibbsEnergy()` — the solver's state, which a bare `GEM_init` has not
populated — and `CNode->NodeStatusCH`, recorded so it can be printed and never scored.

### A fourth condition, found on first use

**A system exported WITHOUT a calculation is a different outcome from one whose result differs.**
`Resources/gems3k-proposed/T10` stores `Ms = 0`, `Vs = 0` and an empty assemblage, and comparing a
real solve against those zeros produced five `ANSWER` findings and a `pH` finding, none of which
mean anything. `hasNoStoredResult()` now detects it — `Ms <= 0` **and** an empty assemblage, so a
genuinely mass-free system could not be misread — and reports one `MISSING` line instead. Getting
this wrong would make the report noisiest exactly where a fresh export most needs attention.

### What it found

Run over the whole curated corpus, native, against the stored reference:

```
66 row(s) checked against the STORED reference, 193 finding(s)
projects that still reproduce their stored result: 11 of 66
```

Stricter than §2's "18 of 62 on `Gs`" because it also compares `Vs`, `Ms`, pH and the assemblage
with amounts — and broader, because it reaches the 4 text-format projects a JSON reader cannot.

**`j_CASHNK` is characterised rather than merely flagged**, which is the point of comparing the
assemblage with amounts:

```
CSH    1.307877697e-05 -> 3.904997342e-01 mol   (rel 1.00e+00)
CSHK   3.910880310e-01 -> 2.757390697e-05 mol   (rel 1.00e+00)
```

The two interchangeable Berman solid solutions have **swapped**: the export picked `CSHK`, today's
solver picks `CSH`. The phase-name *set* is unchanged, so nothing that scores only presence would
see it — this is `tests/TEST-CATALOGUE.md`'s *"the only case exercising the phase-extinction retry"*,
visible in the artefact for the first time.

### Consequence worth stating

In stored mode the lane answers a **weaker** question than in frozen mode — no status, no scored
cost — but over a **wider** set of quantities (species and activity coefficients) and on systems
that have no frozen record at all. The two are complementary, not redundant, and neither is a
substitute for the other.

---

## 5. Plan A — gating the library in a parent repo

Already implemented and working; the remaining work is other people's.

`tools/ci.sh` resolves `$CONDA_PREFIX` before building anything, so inside GEMS3K's or GEMSGUI's CI,
with their environment active, it runs the gate against the library they already installed and
builds none. Verified 2026-09-08: 2/2 in 26.7 s with no library build.

**Blocked today, precisely.** conda-forge's `gems3k` 4.5.5 — what GEMSGUI's environment installs —
has no `otherPMM` and no `Get_GibbsEnergy`. `ci.sh` refuses in under a second and names them. It
becomes available when the packaged GEMS3K carries those; nothing to build meanwhile.

### Also in this lane: promote `param_sensitivity_test`

Native-only, already documented on `develop`, currently asserted by nothing. Turn it into
`tests/test_param_sensitivity.cpp` with the scoring in a `difftest` header (the `solvus_sweep.h`
arrangement), register it, and give it the `ci` label. Per `tests/TEST-CATALOGUE.md`'s own rule it
must be **shown to fail** first — run it against equal Margules parameters and confirm the
"measurably different mole fraction" check goes red.

---

## 6. Open questions — decide before building, not during

1. **Rules file for the export comparison.** New `dbr_export_diff.json`, or reuse `dbr_diff.json`?
   And should `IterDone` / `NodeStatusCH` be compared there? *Recommendation: a new file, and
   compare them — an export whose iteration count moved is an export whose system moved.*
2. **The 3 hand-added projects.** `f_Solvus_G_Test1`, `j_10TH_G_seawater`, `j_TiQ_PRSV` are in the
   corpus and not in `template_export.json`. Should the template gain them, so the export and the
   corpus are one set? That is a GEMSGUI-side change.
3. **Does the report need a stored reference at all?** Comparing a fresh export against the
   checked-in `Resources/gems3k` compares against whenever those were last exported — which is not
   a dated, recorded event. A `Docs/` note recording *which* GEMSGUI version produced the current
   corpus would make the comparison interpretable. **Nobody currently knows this**, and it is worth
   establishing before the report exists rather than after.
4. ~~**Is a fixture change distinguishable from a solver regression?**~~ **Answered 2026-09-08**:
   yes — each project's row set now carries a digest of its input files, and a mismatch is its own
   `FIXTURE` class ranked above everything else. See `Docs/reference/ci.md`.
5. **Should the corpus's stored `-dbr` results be refreshed?** They mostly do not reproduce (§2:
   18 of 62). Refreshing them would make the fixtures self-consistent and make **F1b** meaningful
   here — but it also changes what every warm restart starts from, and it would rewrite 62 files
   whose current contents are, for the cold solves the corpus is used for, harmless.
   *Recommendation: no, or not without a dated record of what produced them.* The freeze file exists
   precisely so this does not have to be answered.
6. **`run-linux.yml`** (branch `develop1`) overlaps this plan. Retire it, or re-point it at
   `export_compare.sh`? It also runs `collect_metrics`, which must not be in any CI job — 100-run
   randomized timing loops that **hang indefinitely on six `gems3k-psina` projects**.

---

## 7. Order of work

| # | step | depends on | size |
|---|---|---|---|
| 1 | settle §6.1 (rules file) and §6.3 (what the corpus's provenance is) | a decision | — |
| 2 | `tools/export_compare.sh` + its rules file, run by hand | 1 | small |
| 3 | doc section in `ci.md` reconciling the three lanes | 2 | small |
| 4 | promote `param_sensitivity_test` to a labelled CTest case, verified to bite | — | small, independent |
| ~~4b~~ | ~~`--against stored`~~ — **done 2026-09-08**, §4b | — | — |
| 5 | `workflow_dispatch` job for the export comparison | 2 | small |
| 6 | decide §6.4 — retire or re-point `run-linux.yml` | 2 | small |
| 7 | propose the `deploy-linux.yml` step to GEMSGUI | 2, 5 | needs their owner |
| 8 | Plan A in GEMSGUI's CI | conda-forge `gems3k` gaining the API | not ours |

Steps 4 and 2 are independent; 4 is the smallest real coverage gain on the list.
