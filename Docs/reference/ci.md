# The CI lane — separating the gate from the benchmark

*Added 2026-09-08. Stage 1: **native only** (AIA + SIA).*

This repo has two jobs and they want opposite things from a run.

| | **benchmark lane** | **CI lane** |
|---|---|---|
| question | how does the algorithm behave, and can it be made better? | did this commit change an answer or an iteration count? |
| budget | minutes to hours | seconds |
| solver paths | all six modes | native AIA and SIA only |
| projects | everything, including the fragile ones | only where a stability measurement admits them |
| wall time | measured, in totals | never measured, never asserted |
| a red run means | interesting — go and look | something regressed — read the row |

Everything that existed before this document is the benchmark lane and **is unchanged**.
`ctest` with no selector still runs the whole suite. Every tool behaves as it did.
The CI lane is additive: one label, one new test, one new tool, one script.

---

## Running it

```bash
tools/ci.sh                      # resolve or build GEMS3K, build the gate, run it
tools/ci.sh --prefix DIR         # use THIS GEMS3K install; never builds one
tools/ci.sh --build-gems3k       # build GEMS3K even if an install was found
tools/ci.sh --no-gems3k          # never build one; fail if none is found
tools/ci.sh --full               # the whole suite instead — i.e. the benchmark lane
```

**Inside a parent repo that has already installed GEMS3K** — GEMS3K's own CI, or GEMSGUI's —
the whole invocation is:

```bash
conda activate <that repo's environment>
path/to/gems-benchmark/tools/ci.sh
```

and **no library is built**. On this development machine, standalone, ThermoFun also has to be
pointed at or it resolves from `/usr/local`, whose prefix carries a stale `include/GEMS3K`:

```bash
conda activate gems-benchmark
tools/ci.sh --extra-prefix "$CONDA_PREFIX/../xgems-jupyter"
```

---

## Which GEMS3K it uses, and when it builds one

gems-benchmark is meant to be **pulled into other repos**, and in those the library is already
built and installed by the time anything here runs. Rebuilding it there would be wasted minutes
and — worse — a *different* library from the one the parent repo just produced, which is the one
actually under test.

| order | source | builds? |
|---|---|---|
| 1 | `--prefix DIR`, or `$GEMS3K_PREFIX` | no |
| 2 | `$CONDA_PREFIX`, if it carries a GEMS3K | **no** — the parent-repo and CI-runner case |
| 3 | this build directory's own previous build | no |
| 4 | otherwise, build from source | yes |

`--build-gems3k` forces a build even when an install is found; `--prefix` and `--no-gems3k` both
mean *do not build*, and say so plainly rather than failing three steps later.

When it does build, it looks for the source in `../GEMS3K`, then `..` (this repo nested *inside*
GEMS3K), then `../../GEMS3K` — by the marker file `GEMS3K/ms_multi.h`, because a bare
`CMakeLists.txt` says nothing. `--gems3k-src DIR` overrides.

### Whichever branch is taken, three things are checked, not assumed

1. **The install is native-only.** Refused if `GEMS3KTargets.cmake` declares `Optima::Optima` —
   with Optima present the whole benchmark tree starts registering Optima cases and compiling
   `-DUSE_OPTIMA_SOLVER` into binaries the CI label was chosen to exclude.
2. **The install carries the API the gate compiles against**, checked *up front* by asking the
   header for each member. Without this the run spends two minutes compiling and then fails
   inside `src/metrics_collector.cpp` with *"class TNode has no member named otherPMM"* — a
   message pointing at this repo's source rather than at the library that is actually too old.
   **Not by version number**, because the version does not track the API: measured 2026-09-08,
   three installs all report `4.5.5` and carry *different* APIs.

   | install | missing |
   |---|---|
   | `envs/GEMSGUI` | `otherPMM`, `Get_GibbsEnergy` |
   | `envs/gems-benchmark` | `Get_GibbsEnergy` |
   | a stock build from current source | none |

   So **GEMSGUI's environment cannot run this gate today** — its GEMS3K predates two members the
   gate needs. The lane says so in under a second and names them.
3. **The library the gate actually loaded came from that prefix** — printed *and* asserted,
   because several installs coexist and `libGEMS3K.so` carries a classic `DT_RPATH` that
   `LD_LIBRARY_PATH` cannot override, so a wrong resolution is silent.

## When it does build one, it builds at stock defaults

It builds from the checked-out source and **passes no solver option at all** — only `CMAKE_INSTALL_PREFIX`, and where the
machine demands it the dependency prefixes and the compiler.

That costs nothing to obey, because GEMS3K's own defaults already *are* the lane's requirements
— checked, not assumed:

| option | default | where |
|---|---|---|
| `USE_OPTIMA_SOLVER` | **OFF** | `GEMS3K/GEMS3K/CMakeLists.txt:27` |
| `USE_THERMOFUN` | **ON** | `GEMS3K/CMakeLists.txt:35` |

### Why that matters more than it looks

With Optima absent from the library, `difftestFindDeps.cmake` auto-detects
`GEMS3K_HAS_OPTIMA = FALSE` from the imported target's own declared dependencies. So
`tests/CMakeLists.txt` **registers no Optima case at all** and passes `-DUSE_OPTIMA_SOLVER` to
nothing. **The lane is native by construction rather than by selection** — `ctest -L ci` and
plain `ctest` in a CI tree are both Optima-free. And there is no `Optima::Optima` to resolve, so
the `GEMS3KConfig.cmake` packaging gap that bites every Optima-enabled install cannot arise.

### Why ThermoFun stays on

Because the corpus needs it: **21 of the 66 recorded projects ship a `-fun.json`** — 14 in
`gems3k`, 4 in `gems3k-fail`, 3 in `gems3k-proposed`. Turning it off would not make the lane
"more default", it would make a third of the record unreadable. It is on *because it is the
GEMS3K default*, which is the whole point of passing no configuration.

### The two flags it does pass, and why they are not solver config

`BUILD_SOLMOD=OFF` and `BUILD_TOOLS=OFF`. Neither changes a line of solver behaviour — they
change how much gets compiled. `tsolmod4rkt` is a standalone library the gate never links, and at
stock `ON` it does not currently build here at all (`ThermoFun/ThermoFun.h: No such file`);
`tools/build-gems3k.sh` has passed `BUILD_SOLMOD=OFF` for the same reason since before the CI lane
existed. Both are overridable with `--gems3k-cmake`.

### A GEMS3K bug this design found immediately

The first stock build failed, and it was not the environment:

```
GEMS3K/ipm_chemical.cpp:1605: error: 'insBudgetTried' was not declared in this scope
```

`std::vector<char> insBudgetTried` was declared **inside** the `#ifdef USE_OPTIMA_SOLVER` block in
`ms_multi.h` (opened at line 1981), but it is used on the **native** path —
`ipm_chemical.cpp:1605,1610`, assigned in `ipm_main.cpp:803`. So **GEMS3K did not compile at its
own default** `USE_OPTIMA_SOLVER=OFF`. Introduced in `d7c9a10`.

How it went unnoticed is visible in the file: the declaration and its doc comment were pasted into
the *middle of `CalculateEquilibriumStateOptima()`'s own comment*, which resumes on the line
straight after it (`// solve; with none registered ...`). Nothing about the surrounding text reads
as Optima-specific.

Fixed 2026-09-08 by **closing and reopening** the conditional block around the declaration rather
than moving it up: the member keeps its exact position in the class, so an Optima build's layout is
byte-for-byte unchanged. Moving it would have been an ABI change, and `CLAUDE.md` §5 records what
those cost — *any binary predating one is stale and will not say so* — which mattered here because
another session had a gate running against that library at the time.

**This is the value of the lane in one incident.** Nobody had built this branch without Optima,
because the benchmark always builds *with* it; a CI job whose whole point is the default
configuration found it on its first run.

### It never touches `/tmp/g3k-prefix`

It installs into its own prefix inside the build directory (`build-ci/gems3k-prefix`).
`/tmp/g3k-prefix` is the **benchmark's** install, it is Optima-enabled, and other sessions run
gates against it. A CI script must not write there. (It is also in `/tmp`, so a reboot deletes
it — that is not hypothetical, it happened while this was being written.)

### And it proves the result, rather than trusting the default

```
== proving the linked GEMS3K is native-only
   GEMS3KTargets.cmake declares no Optima::Optima - native only
```

Same principle as `tools/trace_selfcheck.sh`, which this script does **not** call and does not
depend on: a build flag is a proxy, the artefact is the thing. If a future GEMS3K flips the
default, or someone points `--prefix` at an Optima build, this is what notices — and it matters,
because with Optima present the whole benchmark tree starts registering Optima cases and
compiling `-DUSE_OPTIMA_SOLVER` into binaries the CI label was chosen to exclude. The script
**refuses to run** in that case.

It then prints *and asserts* the `libGEMS3K` the gate actually resolved, because there are
several coexisting installs and `libGEMS3K.so` carries a classic `DT_RPATH` that
`LD_LIBRARY_PATH` cannot override — a wrong resolution is silent.

## What carries the `ci` label, and why only these

The admission rule is deliberately **stricter than "this case happens to run native"**:

> A case carries the `ci` label only if its **test binary contains no Optima code at all** — no
> `USE_OPTIMA_SOLVER`, no `NEED_GEM_AOP/SOP/HOP/SHP/ROP`, in the `.cpp` or in any `difftest`
> header it includes — and every mode it runs is **AIA or SIA**.

Two cases meet it.

| `ci` | s | | benchmark lane — everything else | s |
|---|---:|---|---|---:|
| `ci.baseline` | ~28 | | `optima_regression.aop` | 56.92 |
| `volume.native` | 0.04 | | `evaporation.aop` | 41.11 |
| | | | `trace_phases.aop` | 15.90 |
| | | | `dimreduce.gate` | 3.51 |
| | | | `sweep_regression.aop` | 3.19 |
| | | | `solvus.critical` | 2.99 |
| | | | `solvus.native` | 1.48 |
| | | | `solvus.aop` | 1.41 |
| | | | `proposed.aop` | 1.27 |
| | | | `cement_water.SIA` | 0.87 |
| | | | `cement_water.native` | 0.66 |
| | | | `trace_phases.native` | 0.22 |
| | | | `solvus.stallwatch` | 0.04 |
| | | | `optima_regression.rop` | 0.04 |

### Why the binary, and not just the case

`solvus.native`, `cement_water.native`, `cement_water.SIA` and `trace_phases.native` execute
**nothing but AIA/SIA**, and on that ground alone they would qualify — they were in the label in
the first draft. But their binaries are compiled with `-DUSE_OPTIMA_SOLVER` whenever
`GEMS3K_HAS_OPTIMA`: `test_solvus` and `test_cement_water` need it for their mode dispatch, and
`test_trace_phase_recovery` because `trace_phases.aop` is registered from the *same* executable.
So on an Optima-enabled GEMS3K those binaries carry Optima code paths, and a lane built from them
is one flag away from measuring something it did not intend.

Drawing the line at the **binary** is what makes "AIA and SIA and nothing else" checkable, rather
than a matter of reading each test's `argv`. Audited 2026-09-08 — occurrences of
`USE_OPTIMA_SOLVER` / `NEED_GEM_[ASHR]OP`:

| binary | `.cpp` | cmake define | headers | |
|---|---:|---:|---|---|
| `test_volume_consistency` | 0 | 0 | none | **ci** |
| `test_ci_baseline` | 0 | 0 | none | **ci** |
| `test_solvus` | 3 | 1 | `solvus_sweep.h` 10 | benchmark |
| `test_cement_water` | 1 | 1 | `cement_water_sweep.h` 7 | benchmark |
| `test_trace_phase_recovery` | 5 | 1 | — | benchmark |

(`test_ci_baseline`'s three textual matches are in *comments* explaining why the gate has no
Optima. There is no such code in it.)

`tools/ci.sh` enforces the same line at build time: it builds **only the two CI targets by name**,
so the Optima-compiled binaries are never produced in a CI build tree at all. There is nothing
there to run by mistake, and — a side effect worth having — a clean CI build directory is ~11 MB.
`--full` builds everything, because then the whole suite is what gets run.

### The four removed cases are not retired

They stay in the benchmark lane, which plain `ctest` runs, and each is a candidate to rejoin the
label if it is ever split into an Optima-free binary. What they guard is not lost meanwhile:
`ci.baseline` solves the same `T14_ball120` that `trace_phases.native` guards and the same corpus
`solvus.native` samples, and it **scores the present-phase assemblage**, which is the mechanism
`trace_phases` exists for.

### No benchmark apparatus in the lane

`tools/ci.sh` invokes **no** benchmark script — not `freeze.sh`, not any `run_*_gate.sh`, not
`trace_selfcheck.sh`, not `recheck.py`, not `rop_compare`. It runs `cmake`, `ctest` and `ldd`, and
nothing else. `ci.baseline` likewise reads only `tests/ci-baseline.txt` and the project fixtures
under `Resources/`; the jitter verdicts that decided its cost-pinning are **transcribed into the
header as strings**, not read from a freeze file at run time, so no freeze needs to exist for the
gate to work.

## `ci.baseline` — the new one

The other five cases each assert a *property* of the solver: an identity (`volume.native`),
a repaired defect (`trace_phases.native`), a physical axis (`cement_water.*`), a reference
sweep (`solvus.native`). **None of them answers the plainest question a CI job is asked** —
did this commit change an answer or an iteration count anywhere in the corpus?

The instrument that does answer it is the **freeze**, and it cannot run in CI:

- it shells out to `$GEMS3K/debug-optima-vs-reaktoro/rop_compare`, a binary in the *sibling*
  repo that no build here produces;
- its paths are absolute (`/home/dmiron/git/hub/...`);
- it runs six modes over three corpora for tens of minutes;
- and half its output — the `# set` / `# eff` / `# dec` blocks — is apparatus for *searching
  for a better setting*, which a pass/fail gate does not want.

So `ci.baseline` is the small, self-contained, in-repo subset of the same idea: **66 projects
× 2 native modes = 132 rows**, recorded in `tests/ci-baseline.txt`, scored with the freeze's
own tolerances and its own severity ranking.

The 66 are every native-runnable project across all four corpora:

| corpus | in | cost-pinned | note |
|---|---:|---:|---|
| `Resources/gems3k` | 26 | 20 | the main regression corpus, minus `f_PitzerTHE` which cannot be read |
| `Resources/gems3k-psina` | 28 | 8 | the size ladder — large-system answer coverage, up to 1287 species |
| `Resources/gems3k-fail` | 6 | 6 | a chosen slice; the hard-and-unstable members stay in the benchmark lane |
| `Resources/gems3k-proposed` | 6 | 4 | purpose-built systems reaching mechanisms nothing else here does |
| **total** | **66** | **42** | |

`gems3k-psina` pins the fewest because the freeze's `JITTER` column reads `COUNT-UNSTABLE`
for **every** `T8` rung and three of the five `T14` rungs — those rows are recorded for their
*answer* and not their effort. `TEST-CATALOGUE.md` item 6b already warns that the `T8` axis-2
ladder cannot do its own `N^3` measurement because all five rungs' jitter bands overlap.

- **record it**: `./build-solvus/bin/ci_baseline --out tests/ci-baseline.txt`
- **check it by hand**: `./build-solvus/bin/ci_baseline --check tests/ci-baseline.txt`
- **one project**: add `--project NAME`
- the asserting form is `tests/test_ci_baseline.cpp`, i.e. the `ci.baseline` CTest case

Both halves are thin drivers over **`include/difftest/ci_baseline.h`**, which holds the case
list, the row format and the scoring — the same arrangement as `solvus_sweep.h`, so the
recorder and the checker cannot drift apart. **Read that header before changing a number.**

### The modes are a list

The lane's mode coverage is **one piece of data in one place** — `cibase::knownModes()` — rather
than a fact spread across the solve loop, the record and the CTest registration. Today that list
is:

| mode | `NodeStatusCH` | warm | |
|---|---|---|---|
| `native` | `NEED_GEM_AIA` | no | cold solve |
| `SIA` | `NEED_GEM_SIA` | yes | cold leg first, then re-solve the same node |

**and nothing else.** Not a stub awaiting completion — the lane's declared scope, per the owner's
2026-09-08 instruction that CI notices AIA and SIA and Optima stays in the benchmark lane.

Both drivers take `--modes`, which selects a **subset of that list** and can never introduce a mode
that is not in it:

```bash
ci_baseline --check tests/ci-baseline.txt --modes native
test_ci_baseline tests/ci-baseline.txt --modes native,SIA     # the default
```

An unknown name is **refused, not skipped**, and an Optima mode name is refused *by name*:

```
$ ci_baseline --modes aop
ci_baseline: 'aop' is an Optima mode; the CI lane runs AIA and SIA only
  (Optima is benchmark work - see include/difftest/ci_baseline.h).
  Use the benchmark lane's tools for it.                        [exit 2]
```

Someone asking this binary for AOP has a wrong expectation; silently running two native modes
instead would confirm it. And when `--modes` *narrows* the list, the checker says how many recorded
rows it therefore did not check — a silently unchecked row is the thing this gate exists to prevent:

```
  NOTE  66 recorded row(s) NOT checked - their mode is outside --modes native
```

The record itself carries the list it was taken with, as a `# modes` header line.

**What adding a mode would require**, so nobody does it by appending a row to the vector:

1. the `USE_OPTIMA_SOLVER` compile gate on the header and both drivers — load-bearing, because
   `NEED_GEM_AOP` exists however GEMS3K was built and `GEM_run()` *falls back to native AIA with a
   logged warning* when Optima is absent, so an ungated row would record native's numbers under an
   Optima label and compare green forever;
2. **a jitter measurement for that mode** before any of its rows may be cost-pinned — the verdicts
   recorded against each case are native-*cold* ones and say nothing about any other path, which is
   the same point already biting SIA;
3. a re-recording, and a check that the lane still fits a CI budget — the Optima modes cost
   **25–60 s per point** on T-cement against native's ~12 ms.

### Provenance — what a record pins about its own run

Owner, 2026-09-08: *"probably is safer to always keep frozen regression test results with some
metadata to make the runs reproducible"*. The record's header carries:

```
# recorded  2026-09-08T18:21:05Z
# gems3k    4.6.0   (the version string does NOT track the API - see tools/ci.sh's step 2b)
# built by  gcc 14.3, 64-bit   (floating-point answers depend on this ...)
# modes     native,SIA
# scored    G/Vs/Ms 1e-09 rel, phase amounts 1e-06 rel, pH 0.001 rel, presence floor 1e-12 mol
# recorded-only  EH, MBE, and iteration counts on warm rows and on jitter-unstable projects
# rows      132 over 66 projects
```

The tolerances are in there because they are the record's own contract, and reading them out of a
header file to interpret a diff is a step nobody takes. The compiler and word size are there
because they are what actually moves a floating-point answer.

**Deliberately absent: a git commit for either repo.** It could be plumbed in as a compile
definition, but the recorder is normally run from a build directory configured long ago, so the
value baked into it would be whatever was checked out *then*. A stale sha recorded as current is
worse than no sha at all.

### Input digests — telling a fixture change from a regression

A frozen record answers *did the answer move*. On its own it cannot say **why**, and there are two
entirely different reasons that call for opposite responses:

| | inputs | answer | correct response |
|---|---|---|---|
| the **solver** changed | identical | different | investigate — this is a regression |
| the **fixture** changed | different | different | re-record, deliberately |

So each project carries a digest of the files its rows were produced from — the `-dat.lst` and
every file it names, FNV-1a 64:

```
# fix  f_CalcDolo_G_CalcColumn_0_0_1_25_0  057d7038b8af5739  5 files
# fix  j_CalcDolo_G_CalcColumn_0_0_1_25_0  83533f616736670f  4 files
```

(The `f_` twin has five because its `.lst` also names a `-fun.json`.) A mismatch is reported as
`FIXTURE`, **ranked above every other class**, and that project's rows are **skipped** — verified
by editing one pH digit in a real `-dbr`:

```
FAIL  FIXTURE  j_CalcDolo_G_CalcColumn_0_0_1_25_0  -  input files CHANGED since the record
      was taken (83533f616736670f -> c705b7badb07f322) - this is a fixture change, not a
      solver regression; re-record deliberately
130 row(s) checked, 1 finding(s)
```

One line, not a pile of `ANSWER` findings — and 130 rows rather than 132, because the affected
project's two are not comparable.

**The record describes the COMMITTED corpus, not a working tree.** That is deliberate: the record
ships in the repository and any other checkout has the committed fixtures. A working tree with
uncommitted fixture edits will therefore report `FIXTURE` on those projects — which is accurate,
and is the message worth having. As of 2026-09-09 this repository has exactly that: four
`-ipm.json` files carry an uncommitted `pa_OptimaStallWindow: 500`, so a local run reports four
`FIXTURE` findings and checks the other 124 rows clean. **Nothing is at risk from it** — measured
the same day, all 46 comparable rows are byte-identical between the two fixture states, because
that field is read only on the Optima path. Committing or reverting those four edits makes both
the local run and a clean checkout green.

**Not a directory scan.** Only the files the `.lst` names are hashed: `ipmlog.txt`, `metrics.json`
and stray logs live in those directories and change on every run, which would make the digest
useless within one session.

**Why this matters most here.** `CLAUDE.md` §4's T-cement case is a fixture that was re-exported
mid-flight; a record taken before the re-export would have read as a regression afterwards. The
digest is what turns that into a statement of fact instead of an argument.

The check is shared by the recorder and the gate — a check only one of them performs is a check the
other silently lacks. A record predating the digests still works, and says so.

### What is scored, in order of severity

Deliberately the same ranking `tools/freeze_diff.py` applies, so a CI red and a freeze
finding mean the same thing:

| class | what it is |
|---|---|
| `STATUS` | a converged row stopped converging, or vice versa. Outranks everything — a lost answer is never paid for by an iteration saving |
| `ANSWER` | `G`/`Vs`/`Ms` past 1e-9 relative, **or the present-phase name set changed**, or a phase amount past 1e-6 |
| `COST` | `ITF`/`ITG`/`K2` moved — *only where a jitter measurement admits it* |
| `pH` | past 1e-3 relative, the freeze's own looser band |
| `Eh`, `MBE` | **recorded and never scored** |

Tolerances are literally `freeze_diff.py`'s defaults. `CLAUDE.md` §6: two tools scoring the
same files must share one tolerance — at 1e-6 against 1e-9 two tools once reported *opposite*
verdicts on the same pair.

**`Eh` is never scored** for the same reason the freeze does not score it: it is
underdetermined on unbuffered systems, where a 1e-15 `bIC` nudge moves `10TH`'s by ~1 V.
**Wall time is not even recorded** — two identical runs differ by a median 13 % and a p90 of
68 %, and a CI runner is a shared, noisy, unknown machine.

**The present-phase assemblage is carried in full** because `G`, `Vs` and `Ms` cannot see a
lost phase: on `T14_ball120` native dropped five trace solids, three of them supersaturated,
and the two assemblages differed by 1e-6 RT in `G` and 1.3e-10 in `Vs`. The freeze gained its
`NPH` column for exactly this; `NPH` catches a *count* change but not the count staying at 21
while the *set* drifts, so this record stores the sorted names and their amounts.

### Why iteration counts are scored on only some rows

This is the load-bearing decision in the whole design.

On **20 of 42** corpus projects, nudging `bIC` by `k*1e-15` relative — four or five ulp —
moves native's iteration count by **1.5x to 135x**, and on four it flips the converged/failed
verdict. The runs themselves are deterministic, so a fixed-input comparison is still valid —
but **any code change upstream of the IPM loop acts on those projects exactly as the nudge
does**. A pinned count there goes red on unrelated commits, which is the fastest way to teach
a team to ignore a CI job.

So a row's `COST` is scored only where a jitter verdict reads `stable` — **42 of the 66**. The verdict and the measured `ITG`/`ITF` spread are
recorded *next to each case* in the header, so the admission is auditable and can be re-taken
when the numbers move. The six unpinned projects are still fully scored on `STATUS` and
`ANSWER`; only their cost columns are left free.

**Warm rows (`Mode::warm`, i.e. SIA) are never cost-pinned, and that is a gap rather than a
judgement.**
`tools/iteration_jitter` nudges and re-solves *cold* — it has no warm mode — so no
measurement exists of whether a warm iteration count is stable under the same nudge. The SIA
rows therefore **record** `ITF`/`ITG`/`K2`, so the numbers are in the artefact and a human can
diff them, and do not score them. Teaching `iteration_jitter` a warm mode is the work that
closes this, and it is deliberately not in this commit.

### Why the case list is curated, not a directory scan

Besides the jitter point above: **"someone added a project" must not be a CI failure.**
`CLAUDE.md` §4 is explicit that adding a project is itself a measurement event belonging in a
deliberate commit — `T-cement` landing in a corpus mid-gate moved two `recheck.py` claims to
`STALE` for pure bookkeeping reasons, and `STALE` is supposed to mean a recorded number moved.
A scan would spend that signal on every new export. **Adding a row here is a decision**,
taken in a commit, exactly like promoting a project out of `under_review`.

Projects deliberately absent, with reasons, are listed in the header under
`PROJECTS DELIBERATELY ABSENT`.

### Re-recording is a decision, not a fix

If the gate goes red, the question is *what moved and why*. Overwriting the record with
`--out` because the numbers changed converts a regression into a silent re-baseline — the
exact failure `tools/recheck.py` exists to prevent: *"a `STALE` result means a recorded number
moved: read its note and fix the code or the record — never just update the expected value."*
Re-record deliberately, in its own commit, saying what moved.

---

## What was taken from `MetricsCollector`, and what was left

`include/difftest/metrics_collector.h` is the **benchmark lane's** instrument and is unchanged.
Reviewed for what a *test* can use:

**Taken** — `pm.IT` / `pm.ITF` / `pm.ITG` / `pm.K2` (deterministic for fixed input, and the
direct expression of effort), and `return_status`, the raw `GEM_run()` code, which
`Docs/PROJECT_STATE.md` identifies as the real pass/fail signal.

**Left, with reasons:**

- `ConvergenceMetrics::converged` / `return_code` — **documented dead**. `pm.MK` is set to 0
  or 2 throughout GEMS3K and never to 1, so `converged` is *always false*. Scoring it would
  pass vacuously forever.
- `IterationMetrics::total_iterations` — assigned `0` unconditionally in `recordIterations()`.
- `mass_balance_error` **as defined there** — it is `max(pm.C[i])`, an *unsigned* max over a
  *signed* residual, so a purely negative residual vector reports 0. The CI record stores
  `max|C[i]|` instead and says so, rather than editing `metrics_collector.cpp`:
  `Docs/PROJECT_STATE.md` lists that as a known issue to be fixed only on request, and
  changing it silently would move every recorded benchmark number. The two quantities are
  therefore not comparable across the two files by construction — a second reason the CI one
  is never scored, on top of there being no measured band for it.
- `PerformanceMetrics` entirely — wall time. Note also that `solve_time_ms`,
  `condnum_time_ms` and `solve_call_count` are declared in the struct but written by no
  `to_json`/`from_json` in `src/metrics_collector.cpp`, so they never reach `metrics.json`.
- `MetricsCollector::benchmark()`'s 100-run randomized loops — the benchmark lane's whole
  purpose. What disqualifies them from a gate is that they **measure timing**, which is noise up
  to 68 % between identical runs here and worse on a shared runner; they also **hang indefinitely
  on six `Resources/gems3k-psina` projects**. Neither is a defect for their purpose.

  **Re-checked 2026-09-09 after pulling `origin/develop`, and two earlier claims here were
  withdrawn**: `9486560` made the loop check each run's status and drop failed runs from the
  statistics, and `f6ff85a` precomputes the ±5 % perturbation set once so cold and warm run on the
  *same* inputs. This document previously said it did neither. Recorded rather than quietly edited,
  because a stale claim about someone else's code is the kind that gets repeated.

---

## Verification — that the gate bites

`tests/TEST-CATALOGUE.md`: *"A test must be shown to fail when it should."* Every finding
class was verified by sabotaging a scratch copy of the record, 2026-09-08:

| sabotage | result |
|---|---|
| `G` moved 5e-9 relative on `f_CalcDolo` | `ANSWER` fires, reporting `rel 5.00e-09 > 1e-09` |
| `G` moved **5e-10** relative — below tolerance | **silent**, as it must be |
| `pH` 4.6785740 → 4.7 | `pH` fires at `rel 4.56e-03` |
| `ITG` 72 → 73 on `f_CalcDolo` (**jitter-stable**) | `COST` fires, quoting the admitting verdict |
| `ITG` 82 → 83 on `j_CASHNK` (**COUNT-UNSTABLE 135.8x**) | **silent** — the jitter rule's negative control |
| `status` 2 → 4 on `LBE-6` | `STATUS` fires and short-circuits the rest |
| `Quartz` removed from a solvus row | `ANSWER` fires on the assemblage, `nPh` unchanged at 5 |
| a whole row deleted | `MISSING` fires |

A bug the first self-check found in this file's own parser is worth recording, because it is
the kind that would have made the gate *look* fine: GEMS3K phase names contain spaces
(`Alkali feldspar`), so reading the `PHASES` field with `>>` truncated it to its first word,
every recorded phase then parsed away, and the check reported a full assemblage change on four
projects that had not moved at all. The field is now read as the whole remainder of the line.

**Reproducibility across a rebuild**, which is stronger than a same-process re-run: the record
was taken, the machine then rebooted and `/tmp` was wiped — taking `/tmp/g3k-prefix`, the GEMS3K
install this repo links against, with it. After `tools/build-gems3k.sh` restored it, every row
reproduced exactly.

**Reproducibility across a CONFIGURATION**, which is stronger still. The record was taken against
the benchmark's **Optima-enabled** GEMS3K. `tools/ci.sh` then built a **stock, non-Optima** GEMS3K
from the same source into its own prefix, and all **132 rows reproduced** there — so the native
answers do not depend on whether Optima is compiled into the library at all. That is the check
that makes the CI lane's build independence real rather than assumed.

**The header fix was verified in both directions.** After moving `insBudgetTried` out of the
`#ifdef`, GEMS3K compiles with `USE_OPTIMA_SOLVER=OFF` (which it previously could not) *and* still
compiles with `USE_OPTIMA_SOLVER=ON` against the branch's modified Optima. The class layout is
unchanged by construction: the patch inserts only `#endif`, comments and `#ifdef` — no member is
added, removed or reordered, so with the macro defined the preprocessed token stream is identical.

That reboot is itself worth a note: with the prefix gone, the test binaries **silently fell
through their `DT_RPATH` to a different GEMS3K** in the `xgems-jupyter` conda environment and
failed with an undefined symbol. That is the four-coexisting-installs trap of `CLAUDE.md` §5 in
its cheapest form. `tools/ci.sh` prints the resolved `libGEMS3K` path on every run for exactly
this reason.

---

## Environment traps this lane walked into, so you do not have to

All four were hit while building it on 2026-09-08 and all four are now handled or diagnosed
by `tools/ci.sh`. They are recorded because each one *failed in a place that named neither
the cause nor the cure*.

| symptom | actual cause |
|---|---|
| `undefined symbol: TMultiBase::TotalGibbsEnergy` at **runtime** | `/tmp` was wiped by a reboot, `/tmp/g3k-prefix` with it, and the binaries fell through their `DT_RPATH` to the `xgems-jupyter` GEMS3K. Rebuild with `tools/build-gems3k.sh`. |
| `Target "test_ci_baseline" links to target "Optima::Optima" but the target was not found` | the chosen GEMS3K was built with Optima, and `GEMS3KConfig.cmake` never resolves the dependency its own targets declare. Add `--extra-prefix <optima install>`. |
| `fatal error: GEMS3K/gems3k_version.h: No such file` and `TNode has no member named otherPMM` | **stale GEMS3K headers shadowing the chosen prefix.** `GEMS3KTargets.cmake` sets *no* `INTERFACE_INCLUDE_DIRECTORIES`, so `find_package(GEMS3K)` gives a link target and no include path; the compiler then finds some *other* `include/GEMS3K` from an implicit system directory. There is a stale 30-header tree in `/usr/local/include/GEMS3K`, reached whenever ThermoFun resolves from `/usr/local`. `ci.sh` now passes `-I<prefix>/include` and prints the resolved `ThermoFun_DIR`. |
| `undefined reference to std::ios_base_library_init()@GLIBCXX_3.4.32` | the **system** toolchain linking a conda-built `libGEMS3K.so`. The repo assumes the conda environment is active. `ci.sh` now detects the conda compilers and names this if it still happens. |
| `'insBudgetTried' was not declared in this scope` | **not the environment** — GEMS3K did not compile at its own default `USE_OPTIMA_SOLVER=OFF`. See the section above. `ci.sh` names it exactly. |
| a failed build sailing past into a confusing later error | `cmd \| tee f \|\| true` **does not preserve the status**: `\|\| true` runs a simple command, which resets `PIPESTATUS`. The status is now captured into a variable immediately after the pipeline. A common idiom, and wrong. |

The first and third are the same lesson in two costumes, and it is `CLAUDE.md` §5's: there are
several coexisting GEMS3K installs and **a wrong resolution is silent**. Here they happened to
be *older* copies, so they failed loudly at compile or link time. A *newer* wrong copy would
have built, run, and reported numbers.

---

## The lanes, and what each one answers

| # | question | reference | instrument | verdict |
|---|---|---|---|---|
| **S** | did the **solver** change? | a separate **frozen state file** | `tools/ci.sh` → `ci.baseline`, `volume.native` | **hard gate**, ~30 s |
| **F1a** | did the **system definition** change? | the **previous export** | fresh `gems3k-export` vs `Resources/gems3k`, via `compare_dirs` | report — **not built yet** |
| **F1b** | does the solver **reproduce what the exporter wrote**? | the values **stored in the fresh file** | a round trip, on the GUI side | GUI-side check |
| **B** | how fast / how robust? | itself, and the freeze | the benchmark lane | exploratory, never CI |

Full reasoning, and the plan for the unbuilt parts, in **`Docs/reference/ci-lanes-plan.md`**.

### Why this lane keeps its own frozen record

Every exported project carries its own answer inside its `-dbr` — `IterDone`, `Gs`, `Vs`, `Ms`,
`pH`. So it looks as though a reference has been in the repository all along. **It has not**, and
the measurement is unambiguous. Comparing each shipped `-dbr` against a fresh native solve,
2026-09-08, over the 66 recorded projects:

| | |
|---|---|
| projects with a JSON `-dbr` | 62 |
| stored `Gs` reproduces a fresh solve (rel ≤ 1e-9) | **18** |
| stored `Gs` differs | **30** |
| no `Gs` stored at all | **14** — the whole `gems3k-psina` set |
| stored `IterDone` == fresh `ITG` | **21** |

`j_CASHNK` stores `IterDone: 5818` against today's `82` — a record from a different solver era.
**A round trip against the stored values would report a difference on 44 of 62 projects, none of
them a regression.** A stored result is only trustworthy while it is *fresh*; these are
pre-exported files accumulated over years.

Two further reasons the stored values cannot be the reference:

- **`Resources/dbr_diff.json` marks `IterDone` and `NodeStatusCH` `ignored`** — the iteration count
  and the solver status, exactly what this lane checks. `compare_dirs` was built to compare
  *exports*, and its configuration says so.
- **The stored result is also the INPUT** — a warm (SIA) call starts from that speciation, so it
  cannot be freely re-recorded: refreshing it also changes what every warm restart begins from.
  **That is why `ci.baseline`'s SIA leg runs a cold AIA first and re-solves from that**, rather
  than warm-starting from the shipped `xDC`. Given the numbers above, warm-starting from those
  files would on most projects be measuring how stale they are.

---

## What this lane does *not* cover, deliberately

- **Optima, in any mode.** Stage 1, by owner decision.
- **Sweeps.** Every `ci.baseline` row is one shipped composition. The documented failures of
  `Resources/gems3k-fail` are in the `info.md` *sweep*, not at the base composition — native
  fails 482 sweep steps that AOP solves. A sweep-based CI case is a reasonable stage 2.
- **Warm-path iteration counts** — see above; needs a warm mode in `iteration_jitter` first.
- **Cross-platform numerics.** The workflow runs Linux only. Whether `G` agrees to 1e-9
  relative across compilers and libm implementations is a measurement nobody has taken here.
- **`Resources/under_review`.** Staging — reached by no corpus tool by design, and worked by
  hand until a project is promoted.
