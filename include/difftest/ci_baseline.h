#pragma once
// ci_baseline - the CI lane's regression record: "the answer and the effort did not move".
//
// THE MODES ARE A LIST, and knownModes() below is the only place it is declared.
// Today it holds AIA (cold) and SIA (warm) and nothing else, by owner decision
// (2026-09-08): Optima is BENCHMARK work and stays in the benchmark lane. There
// is no USE_OPTIMA_SOLVER in this file, none in either driver, and no
// Optima-gated row in the record; the CI CTest label likewise admits only test
// binaries that contain no Optima code at all. Both drivers take --modes, which
// selects a SUBSET of knownModes() and cannot introduce anything outside it.
// What adding a mode would require is written out at knownModes() itself.
//
// ============================================================================
// WHY THIS EXISTS, AND WHY IT IS NOT THE FREEZE
// ============================================================================
//
// This repo already has a "nothing moved" instrument: the freeze
// (tools/freeze.sh + tools/freeze_diff.py, Docs/BENCHMARK-FREEZE.md). It is the
// standard, and nothing here replaces or competes with it. But it cannot be the
// thing a CI job runs, for four reasons that are properties of the freeze rather
// than defects in it:
//
//   1. It shells out to $GEMS3K/debug-optima-vs-reaktoro/rop_compare - a binary
//      that lives in the SIBLING repo and is not part of any build this repo
//      performs. A CI checkout of gems-benchmark cannot produce it.
//   2. Its paths are absolute (/home/dmiron/git/hub/...). A CI runner has none.
//   3. It runs SIX modes over three corpora and takes tens of minutes. The whole
//      point of the CI lane is a budget in seconds.
//   4. Its DATA ROWS are only half of it. The "# set"/"# eff"/"# dec" comment
//      blocks exist to let a human search for a better SETTING, and require the
//      GEMS3K_NATIVE_TRACE_FILE records tools/trace_selfcheck.sh gates on. That
//      is exploratory apparatus; a pass/fail gate does not want it.
//
// So the freeze stays the standard for ALGORITHM WORK, and this is the small,
// self-contained, in-repo subset that answers the one CI question: did a commit
// change an answer or an iteration count on a system where those are constants?
//
// ============================================================================
// WHAT IS SCORED, AND THE ORDER OF SEVERITY
// ============================================================================
//
// Deliberately the same ranking tools/freeze_diff.py applies, so that a CI red
// and a freeze finding mean the same thing and are read the same way:
//
//   STATUS   a converged row stopped converging (or vice versa). Outranks
//            everything: a lost answer is never paid for by an iteration saving.
//   ANSWER   G, Vs or Ms moved past tolerance, the present-phase NAME SET
//            changed, or a present phase's amount moved past tolerance.
//   COST     ITF, ITG or K2 moved. Only ever scored where a JITTER MEASUREMENT
//            admits it - see the next section, which is the whole reason the case
//            list is curated rather than a directory scan.
//   pH       reported with the freeze's own looser tolerance (1e-3 relative): it
//            is a derived logarithm of one activity, and is not an observable at
//            all where there is no free solution (CLAUDE.md s4, T-cement).
//   Eh       PRINTED AND NEVER SCORED. Same rule as the freeze. Eh is
//            underdetermined on unbuffered systems - a 1e-15 bIC nudge moves
//            10TH's by ~1 V - so scoring it manufactures failures.
//   MBE      the mass-balance residual max|C[i]|, PRINTED AND NEVER SCORED. It is
//            carried because it is the one convergence-QUALITY number that is
//            deterministic for fixed input and costs nothing, so a human reading
//            a red row has it to hand. It is not scored because no measurement in
//            this repo establishes a band for it.
//
// Tolerances are LITERALLY freeze_diff.py's defaults (G/Vs/Ms 1e-9 relative, pH
// 1e-3 relative) and must stay that way. CLAUDE.md s6: "Two tools scoring the
// same files must share one tolerance" - at 1e-6 against 1e-9 two tools reported
// OPPOSITE verdicts on the same pair.
//
// NOT scored, and not recorded: WALL TIME. Two identical runs differ by a median
// 13 % and a p90 of 68 % (CLAUDE.md s4). A CI job runs on a shared, noisy,
// unknown machine, so a timing assertion there is a coin flip wearing a lab coat.
// Cost is expressed in ITERATIONS, which are deterministic for fixed input.
//
// ============================================================================
// WHY THE CASE LIST IS CURATED, AND WHAT ADMITS A PROJECT
// ============================================================================
//
// A directory scan would be wrong twice over.
//
// FIRST, ITERATION COUNTS ARE NOT UNIVERSALLY REPRODUCIBLE. On 20 of 42 corpus
// projects, nudging bIC by k*1e-15 relative - four or five ulp - moves native's
// iteration count by 1.5x to 158x, and on four it flips the CONVERGED/FAILED
// verdict (CLAUDE.md s4; tools/iteration_jitter is the instrument). The runs
// themselves are deterministic, so a fixed-input A/B is still valid - but any
// code change UPSTREAM of the IPM loop acts on those projects exactly as the
// nudge does. Pinning an iteration count there produces a test that goes red on
// an unrelated commit, which is the fastest way to teach a team to ignore a CI
// job. So a row's COST is scored only where a jitter measurement says `stable`;
// the verdict and the measured ITG/ITF spread are recorded per row below, so the
// admission is auditable and can be re-taken when the numbers move.
//
// SECOND, "SOMEONE ADDED A PROJECT" MUST NOT BE A CI FAILURE. CLAUDE.md s4 is
// explicit that adding a project is itself a measurement event and belongs in a
// deliberate commit that re-baselines what it touches; T-cement landing in a
// corpus mid-gate moved two recheck.py claims to STALE for pure bookkeeping
// reasons, and STALE is supposed to mean a recorded number moved. A directory
// scan would spend that signal on every new export. Adding a row here is a
// decision, taken in a commit, exactly like promoting a project.
//
// Cost is the third admission test. Measured 2026-09-08 over both corpora, every
// native cold solve is <= 0.56 s and most are under 0.05 s, so native is cheap
// enough that the whole main corpus fits the budget - which is why the list below
// is broad rather than a token sample.
//
// ============================================================================
// WHAT WAS TAKEN FROM MetricsCollector, AND WHAT WAS DELIBERATELY LEFT
// ============================================================================
//
// include/difftest/metrics_collector.h is the BENCHMARK lane's instrument and
// stays exactly as it is. Reviewing it for what a TEST can use:
//
//   TAKEN  IterationMetrics::{ipm,mbr,global}_iterations and
//          phase_selection_loops (pm.IT / pm.ITF / pm.ITG / pm.K2). Deterministic
//          for fixed input, and the direct expression of "effort".
//   TAKEN  ConvergenceMetrics::return_status - the raw GEM_run() code. This is
//          the real pass/fail signal, per Docs/PROJECT_STATE.md.
//   LEFT   ConvergenceMetrics::converged / return_code. Documented dead:
//          pm.MK is set to 0 or 2 throughout GEMS3K and never to 1, so
//          `converged` is ALWAYS false. Scoring it would pass vacuously forever.
//   LEFT   IterationMetrics::total_iterations - assigned 0 unconditionally in
//          MetricsCollector::recordIterations().
//   LEFT   ConvergenceMetrics::mass_balance_error AS DEFINED THERE. It is
//          max(pm.C[i]) - an unsigned max over a SIGNED residual, so a purely
//          negative residual vector reports 0. This file records max|C[i]|
//          instead, and says so here rather than editing metrics_collector.cpp:
//          Docs/PROJECT_STATE.md lists that as a known issue to be fixed only on
//          request, and silently changing it would move every recorded benchmark
//          number. The two quantities are therefore NOT comparable across the two
//          files by construction - which is a second reason this one is never
//          scored, on top of there being no measured band for it.
//   LEFT   PerformanceMetrics entirely - wall time, see above. (Note also that
//          solve_time_ms / condnum_time_ms / solve_call_count are declared in the
//          struct but written by no to_json/from_json in
//          src/metrics_collector.cpp, so they never reach metrics.json at all.)
//   LEFT   MetricsCollector::benchmark()'s 100-run randomized loops. They are the
//          benchmark lane's whole purpose, and what disqualifies them from a gate
//          is that they MEASURE TIMING - which on this machine is noise up to
//          68 % between identical runs, and on a shared CI runner is worse. They
//          also hang indefinitely on six Resources/gems3k-psina projects
//          (Docs/PROJECT_STATE.md). Neither is a defect for their purpose.
//
//          RE-CHECKED 2026-09-09 after pulling origin/develop, and TWO EARLIER
//          ENTRIES HERE WERE WITHDRAWN because the upstream work fixed them:
//          `9486560` made the loop check each run's status and drop failed runs
//          from the statistics (it now collects an iteration statistic too), and
//          `f6ff85a` precomputes the +-5 % perturbation set once so cold and warm
//          are driven with the SAME inputs. This file previously said the loop did
//          neither. It is recorded rather than silently deleted because a stale
//          claim about someone else's code is the kind that goes on being repeated.
//
// ADDED here, and in MetricsCollector nowhere: the PRESENT-PHASE ASSEMBLAGE. G,
// Vs and Ms cannot see a lost phase - on T14_ball120 native dropped five trace
// solids, three of them supersaturated, and the two assemblages differed by 1e-6
// RT in G and 1.3e-10 in Vs, so every scalar column reported them identical
// (tests/TEST-CATALOGUE.md, trace_phases). The freeze gained an NPH column for
// exactly this; NPH catches a COUNT change but not the count staying put while
// the SET drifts, so this record carries the sorted names and their amounts.
//
// ============================================================================
// FILE FORMAT
// ============================================================================
//
// One whitespace-separated DATA line per project per mode, '#' comments ignored,
// column order fixed by writeRow()/parseRow() below. Deliberately a text table
// and not JSON: it is meant to be read as a `git diff` in a pull request, which
// is the artefact a reviewer actually looks at.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "GEMS3K/gems3k_version.h"
#include "GEMS3K/node.h"

namespace cibase {

// ---------------------------------------------------------------------------
// Tolerances - identical to tools/freeze_diff.py's defaults, on purpose.
// ---------------------------------------------------------------------------
constexpr double kTolG   = 1e-9;   ///< relative, for G / Vs / Ms
constexpr double kTolAmt = 1e-6;   ///< relative, for a present phase's moles
constexpr double kTolPh  = 1e-3;   ///< relative, for pH - the freeze's looser band

/// Presence floor for a phase, in moles. Deliberately rop_compare's own 1e-12,
/// the value behind the freeze's NPH column, so this record and a freeze taken on
/// the same commit count the same phases as present.
constexpr double kPresenceFloor = 1e-12;

// ---------------------------------------------------------------------------
// Modes
// ---------------------------------------------------------------------------

/// One solver mode the CI lane can be asked to run.
struct Mode {
    std::string name;       ///< as it appears in the MODE column and in --modes
    NODECODECH  need;       ///< the NodeStatusCH this mode sets
    bool        warm;       ///< re-solves after a cold leg on the same node
    long        okStatus;   ///< the GEM_run() code that means "converged"
};

/// EVERY MODE THE CI LANE KNOWS HOW TO RUN. Today: AIA and SIA, and nothing else.
///
/// THE LIST IS THE DESIGN. The lane's mode coverage is a piece of DATA in one
/// place, not a fact spread across the solve loop, the record and the CTest
/// registration - so widening it later is an edit to this vector plus a
/// re-recording, taken as a decision in one commit, and narrowing it is the same.
/// The `--modes` option on both drivers selects a subset of this list; it can
/// never introduce a mode that is not here.
///
/// WHY NO OPTIMA MODE IS IN IT. Owner instruction, 2026-09-08: the CI lane
/// notices AIA and SIA and nothing else; Optima is benchmark work and stays in
/// the benchmark lane. This is not a stub awaiting completion - it is the lane's
/// declared scope.
///
/// WHAT ADDING ONE WOULD REQUIRE, so that a future editor does not do it by
/// simply appending a row here:
///   1. The `USE_OPTIMA_SOLVER` compile gate, on this header and on both
///      drivers. It is load-bearing and not tidiness: NEED_GEM_AOP is an
///      ordinary NODECODECH value present however GEMS3K was built, and
///      TNode::GEM_run() deliberately FALLS BACK to native AIA with a logged
///      warning when Optima is absent - so an ungated Optima row would record
///      native's numbers under an Optima label and compare green forever.
///   2. A jitter measurement PER MODE before any of its rows may be cost-pinned.
///      The verdicts recorded against each Case below are native-cold ones and
///      say nothing about any other path (see kPinWarmCost, which is the same
///      point already biting SIA).
///   3. A re-recording of tests/ci-baseline.txt, and a check that the lane still
///      fits a CI budget - the Optima modes cost 25-60 s PER POINT on T-cement
///      against native's ~12 ms.
inline std::vector<Mode> knownModes()
{
    return {
        // name      NodeStatusCH   warm   OK code
        { "native",  NEED_GEM_AIA,  false, OK_GEM_AIA },
        { "SIA",     NEED_GEM_SIA,  true,  OK_GEM_SIA },
    };
}

/// The modes the lane runs when nothing else is asked for. Kept as a string, in
/// the same form `--modes` takes, so the default and an override are the same
/// kind of thing and the record's own header can print it back verbatim.
inline const char* defaultModeSpec() { return "native,SIA"; }

/// Parse a comma-separated mode list against knownModes().
///
/// Refuses an unknown name rather than skipping it - including, deliberately, the
/// Optima mode names, which get a message saying the lane has no Optima rather
/// than the bare "unknown mode" a typo gets. Someone asking this binary for AOP
/// has a wrong expectation, and silently running two native modes instead would
/// confirm it.
inline bool modesFromSpec(const std::string& spec, std::vector<Mode>& out, std::string& err)
{
    out.clear();
    size_t pos = 0;
    while (pos <= spec.size()) {
        const size_t c = spec.find(',', pos);
        std::string tok = spec.substr(pos, c == std::string::npos ? std::string::npos : c - pos);
        // trim
        while (!tok.empty() && isspace((unsigned char)tok.front())) tok.erase(tok.begin());
        while (!tok.empty() && isspace((unsigned char)tok.back()))  tok.pop_back();
        if (!tok.empty()) {
            std::string lo = tok;
            std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
            bool found = false;
            for (const auto& m : knownModes()) {
                std::string mn = m.name;
                std::transform(mn.begin(), mn.end(), mn.begin(), ::tolower);
                if (mn == lo) { out.push_back(m); found = true; break; }
            }
            if (!found) {
                if (lo == "aop" || lo == "sop" || lo == "hop" || lo == "shp" || lo == "rop")
                    err = "'" + tok + "' is an Optima mode; the CI lane runs AIA and SIA only "
                          "(Optima is benchmark work - see include/difftest/ci_baseline.h). "
                          "Use the benchmark lane's tools for it.";
                else {
                    err = "unknown mode '" + tok + "'; known modes are:";
                    for (const auto& m : knownModes()) err += " " + m.name;
                }
                return false;
            }
        }
        if (c == std::string::npos) break;
        pos = c + 1;
    }
    if (out.empty()) { err = "empty mode list"; return false; }
    return true;
}

// ---------------------------------------------------------------------------
// The case list
// ---------------------------------------------------------------------------

/// One project admitted to the CI record.
struct Case {
    std::string name;       ///< directory name, and the row's PROJECT key
    std::string lst;        ///< repo-relative path to its *-dat.lst
    /// Whether the NATIVE row's ITF/ITG/K2 may be scored. False means the row is
    /// still recorded and its STATUS and ANSWER are still scored - only the COST
    /// columns are left unpinned. See the jitter discussion above.
    bool costPinnable = true;
    /// The measurement that decided costPinnable, verbatim from the standard
    /// freeze's JITTER column, so a future session can re-take the decision
    /// rather than inherit it.
    std::string jitter;
    std::string why;        ///< why this project is in the CI record at all
};

/// WARM rows (Mode::warm) are NEVER cost-pinned, on every project, and this is a
/// gap rather than a judgement.
///
/// The jitter verdicts recorded per case below all come from
/// tools/iteration_jitter, which nudges bIC and re-solves COLD - it has no warm
/// mode. So there is no measurement of whether a warm iteration count is stable
/// under the same nudge, and pinning one would be exactly the single-draw
/// reasoning CLAUDE.md s4 keeps warning about. The SIA rows therefore RECORD
/// ITF/ITG/K2 - so the numbers are in the artefact and a human can diff them -
/// and do not score them. Teaching iteration_jitter a warm mode is the work that
/// would close this, and it is deliberately not in this commit.
constexpr bool kPinWarmCost = false;

/// The projects in the CI record.
///
/// Group 1 is the WHOLE main regression corpus (Resources/gems3k), minus the one
/// project that cannot be read at all. Listing every member by name rather than
/// scanning the directory is the point: the list is what makes adding a project a
/// decision instead of a CI failure.
///
/// Group 2 is a small, deliberately chosen slice of Resources/gems3k-fail, for
/// regimes the main corpus does not contain.
///
/// The `jitter` string on each row is that project's NATIVE verdict from
/// Docs/freeze/2026-09-08-STANDARD-cleanup-floor-bounded.txt, which is where the
/// admission decision comes from.
inline std::vector<Case> cases()
{
    const std::string A = "Resources/gems3k/";
    const std::string B = "Resources/gems3k-fail/";
    const std::string P = "Resources/gems3k-psina/";
    const std::string Q = "Resources/gems3k-proposed/";
    return {
    // ---- Group 1: Resources/gems3k, the main regression corpus ----
    { "f_CalcDolo_G_CalcColumn_0_0_1_25_0",
      A + "f_CalcDolo_G_CalcColumn_0_0_1_25_0/f_CalcDolo_G_CalcColumn_0_0_1_25_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "calcite/dolomite column; ThermoFun half of an f_/j_ pair" },
    { "f_CASHNK_G_Chen04C-3T_0_0_1_25_0",
      A + "f_CASHNK_G_Chen04C-3T_0_0_1_25_0/f_CASHNK_G_Chen04C-3T_0_0_1_25_0-dat.lst",
      false, "COUNT-UNSTABLE ITG:5.2x ITF:1.0x",
      "two interchangeable Berman multi-site solid solutions - the corpus's phase-extinction case" },
    { "f_ClaySorMo_G_TestUO2I001_7_0_1_25_0",
      A + "f_ClaySorMo_G_TestUO2I001_7_0_1_25_0/f_ClaySorMo_G_TestUO2I001_7_0_1_25_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "clay surface complexation / sorption model" },
    { "f_Flowline_G_series1_0001_0_1000_400_1",
      A + "f_Flowline_G_series1_0001_0_1000_400_1/f_Flowline_G_series1_0001_0_1000_400_1-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "1000 bar / 400 C flowline point" },
    { "f_GEOTHERM_G_Test1_0_0_1_75_0",
      A + "f_GEOTHERM_G_Test1_0_0_1_75_0/f_GEOTHERM_G_Test1_0_0_1_75_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "the corpus's most expensive native solve (~690 it) that is still stable" },
    { "f_Kaolinite_G_pHtitr_0_0_1_25_0",
      A + "f_Kaolinite_G_pHtitr_0_0_1_25_0/f_Kaolinite_G_pHtitr_0_0_1_25_0-dat.lst",
      false, "COUNT-UNSTABLE ITG:5.9x ITF:1.0x", "kaolinite pH titration point" },
    { "f_Solvus_G_series1_0001_0_500_400_0",
      A + "f_Solvus_G_series1_0001_0_500_400_0/f_Solvus_G_series1_0001_0_500_400_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "ternary feldspar (Van Laar) below the miscibility gap" },
    { "f_Solvus_G_Test1_0_0_1000_400_0",
      A + "f_Solvus_G_Test1_0_0_1000_400_0/f_Solvus_G_Test1_0_0_1000_400_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.1x", "the solvus fixture at 1000 bar" },
    { "f_TestPNTDB_G_Neutral_0_0_1_75_0",
      A + "f_TestPNTDB_G_Neutral_0_0_1_75_0/f_TestPNTDB_G_Neutral_0_0_1_75_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "690 DC / 42 IC - the largest system in this record" },
    { "f_TestSUP98_G_TestSystem_0_0_1_25_0",
      A + "f_TestSUP98_G_TestSystem_0_0_1_25_0/f_TestSUP98_G_TestSystem_0_0_1_25_0-dat.lst",
      false, "MBR-UNSTABLE ITG:1.3x ITF:1.8x",
      "IPM loop steady while the MBR phase moves 1.8x - the class iteration_jitter gained ITF for" },
    { "j_10TH_G_seawater_1_0_1_25_0",
      A + "j_10TH_G_seawater_1_0_1_25_0/j_10TH_G_seawater_1_0_1_25_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "12-component THEREDA/Pitzer seawater brine" },
    { "j_CalcDolo_G_CalcColumn_0_0_1_25_0",
      A + "j_CalcDolo_G_CalcColumn_0_0_1_25_0/j_CalcDolo_G_CalcColumn_0_0_1_25_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "DATACH-grid half of the CalcDolo f_/j_ pair - NEVER pooled with f_" },
    { "j_CASHNK_G_Chen04C-3T_0_0_1_25_0",
      A + "j_CASHNK_G_Chen04C-3T_0_0_1_25_0/j_CASHNK_G_Chen04C-3T_0_0_1_25_0-dat.lst",
      false, "COUNT-UNSTABLE ITG:135.8x ITF:1.0x",
      "the corpus's most jitter-unstable iteration count - recorded for its ANSWER, never its cost" },
    { "j_ClaySorMo_G_TestUO2I001_7_0_1_25_0",
      A + "j_ClaySorMo_G_TestUO2I001_7_0_1_25_0/j_ClaySorMo_G_TestUO2I001_7_0_1_25_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "DATACH-grid half of the ClaySorMo pair" },
    { "j_Flowline_G_series1_0001_0_1000_400_1",
      A + "j_Flowline_G_series1_0001_0_1000_400_1/j_Flowline_G_series1_0001_0_1000_400_1-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "DATACH-grid half of the Flowline pair" },
    { "j_GEOTHERM_G_Test1_0_0_1_75_0",
      A + "j_GEOTHERM_G_Test1_0_0_1_75_0/j_GEOTHERM_G_Test1_0_0_1_75_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "DATACH-grid half of the GEOTHERM pair" },
    { "j_Kaolinite_G_pHtitr_0_0_1_25_0",
      A + "j_Kaolinite_G_pHtitr_0_0_1_25_0/j_Kaolinite_G_pHtitr_0_0_1_25_0-dat.lst",
      false, "COUNT-UNSTABLE ITG:8.0x ITF:1.0x", "DATACH-grid half of the Kaolinite pair" },
    { "j_PitzerTHE_G_KCaSO4_10_0_0_100_0",
      A + "j_PitzerTHE_G_KCaSO4_10_0_0_100_0/j_PitzerTHE_G_KCaSO4_10_0_0_100_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.1x",
      "Pitzer K-Ca-SO4 at 100 C. Its f_ twin is NOT here - see the note under this list" },
    { "j_Solvus_G_series1_0001_0_500_400_0",
      A + "j_Solvus_G_series1_0001_0_500_400_0/j_Solvus_G_series1_0001_0_500_400_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "DATACH-grid half of the Solvus series1 pair" },
    { "j_TestPNTDB_G_Neutral_0_0_1_75_0",
      A + "j_TestPNTDB_G_Neutral_0_0_1_75_0/j_TestPNTDB_G_Neutral_0_0_1_75_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "DATACH-grid half of the 690-DC TestPNTDB pair" },
    { "j_TestSUP98_G_TestSystem_0_0_1_25_0",
      A + "j_TestSUP98_G_TestSystem_0_0_1_25_0/j_TestSUP98_G_TestSystem_0_0_1_25_0-dat.lst",
      false, "MBR-UNSTABLE ITG:1.3x ITF:1.9x", "DATACH-grid half of the TestSUP98 pair" },
    { "j_TiQ_PRSV_G_MySystem_0_0_10000_900_0",
      A + "j_TiQ_PRSV_G_MySystem_0_0_10000_900_0/j_TiQ_PRSV_G_MySystem_0_0_10000_900_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.2x",
      "PR78 cubic-EOS fluid at 10 kbar / 900 C - the fixture the 2026-09-04 volume defect was found on" },
    { "o_Kaolinite_G_pHtitrKa_0_0_1_25_0",
      A + "o_Kaolinite_G_pHtitrKa_0_0_1_25_0/o_Kaolinite_G_pHtitrKa_0_0_1_25_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "the TEXT (o_) export format - the only coverage of that reader here" },
    { "o_Solvus_G_series2_0002_0_1000_640_0",
      A + "o_Solvus_G_series2_0002_0_1000_640_0/o_Solvus_G_series2_0002_0_1000_640_0-dat.lst",
      true,  "stable ITG:1.1x ITF:1.0x", "text-format solvus point inside the miscibility gap" },
    { "t_Kaolinite_G_pHtitrKa_0_0_1_25_0",
      A + "t_Kaolinite_G_pHtitrKa_0_0_1_25_0/t_Kaolinite_G_pHtitrKa_0_0_1_25_0-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "the other text export variant (t_)" },
    { "t_Solvus_G_series2_0002_0_1000_640_0",
      A + "t_Solvus_G_series2_0002_0_1000_640_0/t_Solvus_G_series2_0002_0_1000_640_0-dat.lst",
      true,  "stable ITG:1.2x ITF:1.0x", "t_ twin of the o_ solvus point" },

    // ---- Group 2: Resources/gems3k-fail, regimes the main corpus lacks ----
    // Chosen for STABILITY and CHEAPNESS, not for being hard: this record is a
    // gate, and the hard-and-unstable members of that corpus are exactly what a
    // gate must not be built on. The projects native is documented to collapse on
    // stay in the benchmark lane where a spread can be measured properly.
    { "07PSIna_G_iron_1_0_1_25",
      B + "07PSIna_G_iron_1_0_1_25/07PSIna_G_iron_1_0_1_25-dat.lst",
      true,  "stable ITG:1.0x ITF:1.1x", "Fe redox system; its ironsi twin isolates the silica addition" },
    { "07PSIna_G_ironsi_1_0_1_25",
      B + "07PSIna_G_ironsi_1_0_1_25/07PSIna_G_ironsi_1_0_1_25-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "the iron twin plus silica - ITG steady where the MBR phase was not" },
    { "CASH+_G_csh_sol",
      B + "CASH+_G_csh_sol/csh_sol-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "C-A-S-H cement solid solution; converges as shipped, fails only at 10x tighter pa_DK. "
      "NOTE the .lst stem disagrees with the directory name (csh_sol-dat.lst) - CLAUDE.md s4" },
    { "Cu-Pourbaix_G_pHtitr",
      B + "Cu-Pourbaix_G_pHtitr/Cu-Pourbaix_G_pHtitr-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "Cu Pourbaix titration base point - the fixture sweep_regression sweeps" },
    { "FeNaCl_FyGt_SI",
      B + "FeNaCl_FyGt_SI/FeNaCl_FyGt_SI-dat.lst",
      true,  "stable ITG:1.0x ITF:1.2x", "the only genuinely separate member of the FeNaCl family (the other three share a -dbr lineage)" },
    { "LBE-6_G_M1-6_HSCdb",
      B + "LBE-6_G_M1-6_HSCdb/M1-6_HSCdb-dat.lst",
      true,  "stable ITG:1.0x ITF:1.0x", "lead-bismuth eutectic, HSC database - a non-aqueous system with no pH at all. "
      "NOTE the .lst stem disagrees with the directory name (M1-6_HSCdb-dat.lst)" },

    // ---- Group 3: Resources/gems3k-psina, the SIZE ladder ----
    // Added on the owner's question, 2026-09-08: these run on the native path, so
    // there is no reason of principle to leave them out. Measured the same day -
    // all 28 read and solve, every native cold solve is cheap except five
    // (complex x2, vcomplex x3) at 1.6-2.2 s.
    //
    // Cost-pinning follows the same rule as everywhere else, and here it bites
    // hard: the freeze's JITTER column reads COUNT-UNSTABLE for EVERY T8 rung and
    // for three of the five T14 rungs, so most of this group is recorded for its
    // ANSWER and not its effort. That is the honest position - tests/
    // TEST-CATALOGUE.md item 6b already warns that T8 axis 2 cannot do its own
    // N^3 measurement because all five rungs' jitter bands overlap.
    //
    // What the group buys is LARGE-SYSTEM answer coverage: T14_ball120 is 1287
    // species and T8_aq1138 is the other giant, and nothing else in this record
    // approaches them.
    // (the rows themselves)
    { "07PSIna_G_complex_1_0_1_25_0",
      P + "07PSIna_G_complex_1_0_1_25_0/07PSIna_G_complex_1_0_1_25_0-dat.lst",
      true , "stable ITG:1.1x ITF:1.0x",
      "large rung, 25 C" },
    { "07PSIna_G_complex_1_0_1_80_0",
      P + "07PSIna_G_complex_1_0_1_80_0/07PSIna_G_complex_1_0_1_80_0-dat.lst",
      true , "stable ITG:1.1x ITF:1.0x",
      "the same large rung at 80 C - the pair isolates T" },
    { "07PSIna_G_edt_1_0_1_25_0",
      P + "07PSIna_G_edt_1_0_1_25_0/07PSIna_G_edt_1_0_1_25_0-dat.lst",
      true , "stable ITG:1.0x ITF:1.0x",
      "EDTA complexation, first composition" },
    { "07PSIna_G_edt_2_0_1_25_0",
      P + "07PSIna_G_edt_2_0_1_25_0/07PSIna_G_edt_2_0_1_25_0-dat.lst",
      false, "MBR-UNSTABLE ITG:1.0x ITF:2.1x",
      "EDTA complexation, second composition" },
    { "07PSIna_G_mid_1_0_1_25_0",
      P + "07PSIna_G_mid_1_0_1_25_0/07PSIna_G_mid_1_0_1_25_0-dat.lst",
      true , "stable ITG:1.0x ITF:1.0x",
      "the ladder's middle rung" },
    { "07PSIna_G_simple_0_0_0_101_0",
      P + "07PSIna_G_simple_0_0_0_101_0/07PSIna_G_simple_0_0_0_101_0-dat.lst",
      false, "MBR-UNSTABLE ITG:1.4x ITF:8.7x",
      "size ladder, smallest rung; 101 bar" },
    { "07PSIna_G_simple_0_0_0_150_0",
      P + "07PSIna_G_simple_0_0_0_150_0/07PSIna_G_simple_0_0_0_150_0-dat.lst",
      true , "stable ITG:1.2x ITF:1.2x",
      "same system at 150 bar - the pair isolates P" },
    { "07PSIna_G_simple_0_0_1_25_0",
      P + "07PSIna_G_simple_0_0_1_25_0/07PSIna_G_simple_0_0_1_25_0-dat.lst",
      true , "stable ITG:1.1x ITF:1.2x",
      "the ladder's base rung" },
    { "07PSIna_G_simple_1_0_1_25_0",
      P + "07PSIna_G_simple_1_0_1_25_0/07PSIna_G_simple_1_0_1_25_0-dat.lst",
      true , "stable ITG:1.0x ITF:1.0x",
      "simple rung, second composition" },
    { "07PSIna_G_simple_2_0_1_25_0",
      P + "07PSIna_G_simple_2_0_1_25_0/07PSIna_G_simple_2_0_1_25_0-dat.lst",
      false, "COUNT-UNSTABLE ITG:1.5x ITF:1.5x",
      "simple rung, third composition" },
    { "07PSIna_G_vcomplex_0_0_1_25_0",
      P + "07PSIna_G_vcomplex_0_0_1_25_0/07PSIna_G_vcomplex_0_0_1_25_0-dat.lst",
      true , "stable ITG:1.1x ITF:1.2x",
      "the ladder's largest ordinary rung, 25 C" },
    { "07PSIna_G_vcomplex_0_0_1_80_0",
      P + "07PSIna_G_vcomplex_0_0_1_80_0/07PSIna_G_vcomplex_0_0_1_80_0-dat.lst",
      true , "stable ITG:1.1x ITF:1.0x",
      "largest ordinary rung at 80 C" },
    { "07PSIna_G_vcomplex_2_0_1_80_0",
      P + "07PSIna_G_vcomplex_2_0_1_80_0/07PSIna_G_vcomplex_2_0_1_80_0-dat.lst",
      true , "stable ITG:1.1x ITF:1.0x",
      "largest rung, third composition. Docs/PROJECT_STATE.md records this one failing GEM_init on 2026-08-02; it reads and solves now, and this row is what would notice if that came back" },
    { "T14_ball000",
      P + "T14_ball000/T14_ball000-dat.lst",
      false, "COUNT-UNSTABLE ITG:1.9x ITF:1.0x",
      "T14 dissolution ladder rung - ball120 is the 1287-species fixture trace_phases.native guards" },
    { "T14_ball005",
      P + "T14_ball005/T14_ball005-dat.lst",
      false, "COUNT-UNSTABLE ITG:2.1x ITF:1.0x",
      "T14 dissolution ladder rung - ball120 is the 1287-species fixture trace_phases.native guards" },
    { "T14_ball020",
      P + "T14_ball020/T14_ball020-dat.lst",
      false, "COUNT-UNSTABLE ITG:1.6x ITF:1.0x",
      "T14 dissolution ladder rung - ball120 is the 1287-species fixture trace_phases.native guards" },
    { "T14_ball060",
      P + "T14_ball060/T14_ball060-dat.lst",
      true , "stable ITG:1.5x ITF:1.1x",
      "T14 dissolution ladder rung - ball120 is the 1287-species fixture trace_phases.native guards" },
    { "T14_ball120",
      P + "T14_ball120/T14_ball120-dat.lst",
      true , "stable ITG:1.3x ITF:1.0x",
      "T14 dissolution ladder rung - ball120 is the 1287-species fixture trace_phases.native guards" },
    { "T8_aq101",
      P + "T8_aq101/T8_aq0101-dat.lst",
      false, "COUNT-UNSTABLE ITG:3.8x ITF:1.0x",
      "T8 axis-1 species ladder - the rungs that calibrated the 200-species dimension-reduction gate  NOTE the .lst stem disagrees with the directory name (T8_aq0101-dat.lst)" },
    { "T8_aq1138",
      P + "T8_aq1138/T8_aq1138-dat.lst",
      false, "COUNT-UNSTABLE ITG:1.9x ITF:1.0x",
      "T8 axis-1 species ladder - the rungs that calibrated the 200-species dimension-reduction gate" },
    { "T8_aq201",
      P + "T8_aq201/T8_aq0201-dat.lst",
      false, "COUNT-UNSTABLE ITG:3.4x ITF:1.0x",
      "T8 axis-1 species ladder - the rungs that calibrated the 200-species dimension-reduction gate  NOTE the .lst stem disagrees with the directory name (T8_aq0201-dat.lst)" },
    { "T8_aq401",
      P + "T8_aq401/T8_aq0401-dat.lst",
      false, "COUNT-UNSTABLE ITG:1.9x ITF:1.0x",
      "T8 axis-1 species ladder - the rungs that calibrated the 200-species dimension-reduction gate  NOTE the .lst stem disagrees with the directory name (T8_aq0401-dat.lst)" },
    { "T8_aq801",
      P + "T8_aq801/T8_aq0801-dat.lst",
      false, "COUNT-UNSTABLE ITG:2.8x ITF:1.0x",
      "T8 axis-1 species ladder - the rungs that calibrated the 200-species dimension-reduction gate  NOTE the .lst stem disagrees with the directory name (T8_aq0801-dat.lst)" },
    { "T8ax2_nIC17",
      P + "T8ax2_nIC17/T8ax2_nIC17-dat.lst",
      false, "COUNT-UNSTABLE ITG:2.7x ITF:1.0x",
      "T8 axis-2 ladder, varying the number of independent components" },
    { "T8ax2_nIC25",
      P + "T8ax2_nIC25/T8ax2_nIC25-dat.lst",
      false, "COUNT-UNSTABLE ITG:4.9x ITF:1.0x",
      "T8 axis-2 ladder, varying the number of independent components" },
    { "T8ax2_nIC35",
      P + "T8ax2_nIC35/T8ax2_nIC35-dat.lst",
      false, "COUNT-UNSTABLE ITG:3.1x ITF:1.0x",
      "T8 axis-2 ladder, varying the number of independent components" },
    { "T8ax2_nIC45",
      P + "T8ax2_nIC45/T8ax2_nIC45-dat.lst",
      false, "COUNT-UNSTABLE ITG:9.6x ITF:1.0x",
      "T8 axis-2 ladder, varying the number of independent components" },
    { "T8ax2_nIC61",
      P + "T8ax2_nIC61/T8ax2_nIC61-dat.lst",
      false, "COUNT-UNSTABLE ITG:3.8x ITF:1.0x",
      "T8 axis-2 ladder, varying the number of independent components" },

    // ---- Group 4: Resources/gems3k-proposed, the purpose-built systems ----
    // Also added 2026-09-08 on the owner's question. Cheap (every native solve
    // under 0.11 s) and they reach mechanisms no other group here does: T2b dials
    // water's H:O = 2:1 rank deficiency as a controlled knob, T11 approaches a
    // gas-phase appearance boundary by CO2 titration, T5 and T10 are the same
    // chemistry apart from an Fe redox buffer, and T12 is the second cubic-EOS
    // fixture.
    //
    // These have NO jitter verdict in any freeze - the standard freeze covers
    // gems3k, gems3k-fail and gems3k-psina only - so the verdicts below were
    // MEASURED for this commit with `iteration_jitter <lst> 9 1e-15 native`:
    // T1, T5, T10, T11 stable; T12 MBR-UNSTABLE (ITF 2.8x); T2b COUNT-UNSTABLE
    // (ITG 35.6x, which is what a controlled rank deficiency does to an iteration
    // count and is the project working as designed).
    { "T1",
      Q + "T1/T1-dat.lst",
      true , "stable ITG:1.0x ITF:1.0x",
      "purpose-built baseline system from the literature review" },
    { "T10",
      Q + "T10/T10-dat.lst",
      true , "stable ITG:1.0x ITF:1.1x",
      "the Fe-redox-buffered half of the T5/T10 Eh pair" },
    { "T11",
      Q + "T11/T11-dat.lst",
      true , "stable ITG:1.0x ITF:1.1x",
      "CO2 titration toward a gas-phase appearance boundary" },
    { "T12",
      Q + "T12/CO2-dat.lst",
      false, "MBR-UNSTABLE ITG:1.1x ITF:2.8x",
      "PRSV cubic-EOS fluid - the second of the two projects the 2026-09-04 volume defect showed on  NOTE the .lst stem disagrees with the directory name (CO2-dat.lst)" },
    { "T2b",
      Q + "T2b/T2b-dat.lst",
      false, "COUNT-UNSTABLE ITG:35.6x ITF:1.0x",
      "water's H:O = 2:1 rank deficiency as a CONTROLLED knob - the only project that can dial it" },
    { "T5",
      Q + "T5/T5-dat.lst",
      true , "stable ITG:1.0x ITF:1.0x",
      "the unbuffered half of the T5/T10 Eh pair" },
    };
}

/// Which of the listed cases this CHECKOUT actually has, and what it is missing.
///
/// Not every checkout carries every corpus. Measured 2026-09-09 on a fresh
/// checkout of the first commit: `Resources/gems3k` is tracked, but
/// `gems3k-fail`, `gems3k-psina` and `gems3k-proposed` are not, and three
/// `gems3k` projects were added by hand and never committed either - so 23 of the
/// 66 cases were present and 43 were not.
///
/// A missing corpus is NOT a finding. It is not a regression, not a fixture
/// change, and not something a person can act on from a red build; erroring out
/// would simply make the gate unusable anywhere but this machine. So absent cases
/// are excluded from the run and COUNTED, and the count is printed prominently -
/// a gate that quietly checks a third of what its record describes, while looking
/// exactly as green as one that checked all of it, is the failure mode to avoid.
inline std::vector<Case> availableCases(const std::vector<Case>& all,
                                        std::map<std::string, int>& absentByCorpus)
{
    std::vector<Case> present;
    absentByCorpus.clear();
    for (const auto& c : all) {
        if (std::filesystem::exists(c.lst)) { present.push_back(c); continue; }
        // "Resources/<corpus>/<project>/..." - name the corpus, not each project.
        const size_t a = c.lst.find('/');
        const size_t b = (a == std::string::npos) ? a : c.lst.find('/', a + 1);
        absentByCorpus[(b == std::string::npos) ? c.lst : c.lst.substr(a + 1, b - a - 1)] += 1;
    }
    return present;
}

/// One line describing what was skipped, or empty when nothing was.
inline std::string availabilityNote(size_t nPresent, size_t nTotal,
                                    const std::map<std::string, int>& absentByCorpus)
{
    if (absentByCorpus.empty()) return "";
    std::ostringstream o;
    o << nPresent << " of " << nTotal << " projects present in this checkout; "
      << (nTotal - nPresent) << " absent (";
    bool first = true;
    for (const auto& kv : absentByCorpus) {
        if (!first) o << ", ";
        o << kv.first << " " << kv.second;
        first = false;
    }
    o << ") - those corpora are not part of every checkout, so the lane checked what is here";
    return o.str();
}

// PROJECTS DELIBERATELY ABSENT, and why - so the next reader does not re-derive it:
//
//   f_PitzerTHE_G_KCaSO4_10_0_0_100_0  cannot be read at all. GEM_init fails on a
//       file error, in an unmodified checkout too, since 2026-07-28
//       (Docs/PROJECT_STATE.md). Its -fun.json differs from the j_ sibling, which
//       has no such file. A row for it would assert "still unreadable", which is a
//       claim about an export, not about the solver.
//   T-cement  promoted into gems3k-fail on 2026-09-08, AFTER the standard freeze
//       this list takes its jitter verdicts from, so there is no jitter
//       measurement to admit or refuse its cost. It is also already covered by
//       cement_water.native / cement_water.SIA, which are in the CI label.
//   the rest of gems3k-fail  either jitter-unstable, or projects whose documented
//       behaviour is a FAILURE that native finds under sweep. Both belong in the
//       benchmark lane.
//   Resources/gems3k-psina, -proposed, under_review  size ladders, purpose-built
//       systems and staging. Not regression material.

// ---------------------------------------------------------------------------
// Provenance: pinning the INPUTS a record was taken from
// ---------------------------------------------------------------------------
//
// A frozen record answers "did the answer move". On its own it cannot tell you
// WHY, and there are two entirely different reasons:
//
//     the SOLVER changed   - inputs identical, answer different. A regression.
//     the FIXTURE changed  - somebody re-exported the project. Not a regression,
//                            and re-recording is the correct response.
//
// Nothing distinguished them, and the difference decides what a person should do
// next. So each project's row set carries a DIGEST OF THE FILES IT WAS PRODUCED
// FROM, and a mismatch is reported as its own class, ranked above everything
// else, instead of surfacing as a pile of ANSWER findings that look like a
// regression and are not.
//
// This matters most exactly where this corpus is weakest: `CLAUDE.md` s4's
// T-cement case is a fixture that was re-exported mid-flight, and the record
// recorded before the re-export would have read as a regression afterwards.
//
// WHAT IS HASHED: the `-dat.lst` itself and every file it names - the dch, the
// ipm and the dbr - in the order listed. Not a directory scan: `ipmlog.txt`,
// `metrics.json` and stray logs live in those directories and change on every
// run, and hashing them would make the digest useless within one session.
//
// FNV-1a 64-bit, implemented here rather than pulled in: it needs no dependency,
// it is stable across platforms and compilers by construction, and nothing about
// this use is adversarial - it is guarding against a file having been rewritten,
// not against someone forging one.

inline unsigned long long fnv1a(const std::string& bytes, unsigned long long h = 1469598103934665603ULL)
{
    for (unsigned char ch : bytes) { h ^= ch; h *= 1099511628211ULL; }
    return h;
}

/// The files a project is made of: its .lst, then every path the .lst names.
inline std::vector<std::string> fixtureFiles(const std::string& lst)
{
    std::vector<std::string> out{lst};
    std::ifstream in(lst);
    std::string all((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::string dir = lst.substr(0, lst.find_last_of('/') + 1);
    size_t pos = 0;
    while ((pos = all.find('"', pos)) != std::string::npos) {
        const size_t end = all.find('"', pos + 1);
        if (end == std::string::npos) break;
        out.push_back(dir + all.substr(pos + 1, end - pos - 1));
        pos = end + 1;
    }
    return out;
}

/// Digest of a project's input files. Returns false if any of them is unreadable,
/// which is itself worth reporting rather than hashing to some default.
inline bool fixtureDigest(const std::string& lst, std::string& hex, int& nFiles)
{
    unsigned long long h = 1469598103934665603ULL;
    nFiles = 0;
    for (const auto& f : fixtureFiles(lst)) {
        std::ifstream in(f, std::ios::binary);
        if (!in) return false;
        std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        h = fnv1a(bytes, h);
        ++nFiles;
    }
    char buf[24];
    std::snprintf(buf, sizeof buf, "%016llx", h);
    hex = buf;
    return nFiles > 0;
}

// ---------------------------------------------------------------------------
// Where the REFERENCE comes from
// ---------------------------------------------------------------------------

/// The chain is the same either way - load the system, solve it, build a Row -
/// and only the source of the Row it is compared against changes. Owner,
/// 2026-09-08: "in both cases the chain should work ... just that the comparison
/// should be made against external tables, files with results".
///
///   Frozen  an EXTERNAL table (tests/ci-baseline.txt). The right reference for
///           PRE-EXPORTED files, whose own stored results are not current:
///           measured over this corpus, only 18 of 62 shipped -dbr files
///           reproduce a fresh solve, and j_CASHNK stores IterDone 5818 against
///           today's 82. Because the reference is external, the fixtures stay
///           byte-stable and several tables can coexist (per library version, per
///           settings profile) without touching the corpus.
///   Stored  the results INSIDE the system files themselves. The right reference
///           for a FRESHLY exported system, where they are current by
///           construction and no external table can exist yet - and the only
///           check available to a project sitting in Resources/under_review.
enum class Reference { Frozen, Stored };

/// What may be scored against a given reference. Not a preference - each flag is
/// a consequence of what the reference can actually supply. See scoringFor().
struct Scoring {
    bool status = true;   ///< compare the GEM_run() return code
    bool cost   = true;   ///< compare ITF/ITG/K2 where the case admits it
    std::string note;     ///< printed once, so a reader knows what was NOT scored
};

inline Scoring scoringFor(Reference r)
{
    if (r == Reference::Frozen) return Scoring{true, true, ""};

    // STATUS IS NOT SCORABLE AGAINST A STORED RESULT, because the field is an
    // INPUT, not an output. CNode->NodeStatusCH in an exported file reads
    // NEED_GEM_AIA (1) - the code asking for a cold solve - not the OK_GEM_AIA
    // (2) that comes back. The exporter writes what the node should be ASKED to
    // do. This is exactly why Resources/dbr_diff.json marks NodeStatusCH
    // `ignored`, and it means the class at the TOP of the severity ranking has no
    // counterpart here. Saying so is better than comparing an input to an output.
    //
    // COST IS NOT SCORED EITHER, for two reasons that agree. IterDone is a real
    // result, but it came from whatever GEMS3K the EXPORTER linked - comparing it
    // to a fresh ITG is sound only if that is the same build. And a freshly
    // exported system has no jitter measurement at all, so Case::costPinnable is
    // false for it in any case (see the jitter discussion above). Both numbers
    // are still RECORDED and printed; neither decides anything.
    return Scoring{false, false,
        "stored-reference mode: STATUS not scored (NodeStatusCH in an export is a "
        "request, not a result) and iteration counts not scored (they come from "
        "the exporter's own library, and a fresh system has no jitter measurement)"};
}

// ---------------------------------------------------------------------------
// A recorded row
// ---------------------------------------------------------------------------

struct Row {
    std::string project, mode;
    long status = 0;
    long itf = 0, itg = 0, k2 = 0;
    double G = 0., pH = 0., Eh = 0., Vs = 0., Ms = 0., mbe = 0.;
    int  nPh = 0;
    /// Present phases, sorted by name. The names are the SET that is scored; the
    /// amounts are scored per name at kTolAmt.
    std::vector<std::pair<std::string, double>> phases;
    bool ran = false;       ///< false when the project could not even be read
};

/// True for the OK_* half of the status enum. BAD_* and ERR_* are both "not a
/// trustworthy answer" here; the exact code is still compared, so a BAD->ERR move
/// is still a finding - it is just described as a status change either way.
/// True for the OK_* codes of the modes this lane knows. Kept as a set rather
/// than compared against one Mode::okStatus so that a row read back from the
/// record - which carries a mode NAME and a number, not a Mode - can still be
/// classified.
inline bool statusIsOk(long st)
{
    for (const auto& m : knownModes()) if (st == m.okStatus) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Solving
// ---------------------------------------------------------------------------

/// Solve one project in one mode and fill a Row.
///
/// A FRESH TNode per row, so no row can warm-start from another and the record is
/// order-independent - re-recording a single project must reproduce its line.
///
/// The SIA row is the deliberate exception: a warm call means nothing on a node
/// that has never solved, so the SIA leg runs a cold AIA first on the SAME node
/// and then re-solves. What it measures is therefore "can native re-solve its own
/// answer" - the only check anything performs on the cold path's output, since
/// GEM_run()'s cold path does not verify the answer it returns.
///
/// On a FAILED row every packed value is STALE: GEM_run()'s catch never calls
/// packDataBr(), so CNode still holds the input file's numbers (CLAUDE.md s4).
/// The row is still recorded - the STATUS is the finding - but compareRow() stops
/// at the status so that a flip reads as one finding and not as a hundred
/// spurious ANSWER findings.
inline Row solveOne(const Case& c, const Mode& m)
{
    Row r;
    r.project = c.name;
    r.mode    = m.name;

    TNode n;
    if (n.GEM_init(c.lst.c_str())) return r;             // ran stays false

    if (m.warm) {
        // A warm call means nothing on a node that has never solved, so run a
        // cold AIA first on the SAME node and then re-solve.
        n.pCNode()->NodeStatusCH = NEED_GEM_AIA;
        (void)n.GEM_run(false);
    }
    n.pCNode()->NodeStatusCH = m.need;
    r.status = n.GEM_run(false);
    r.ran = true;

    long k2 = 0, itf = 0, itg = 0;
    n.GEM_CalcTime(k2, itf, itg);
    r.k2 = k2; r.itf = itf; r.itg = itg;

    r.G  = n.Get_GibbsEnergy();
    r.pH = n.Get_pH();
    r.Eh = n.Get_Eh();
    r.Vs = n.cVs();
    r.Ms = n.cMs();

    // max|C[i]| - the ABSOLUTE max over a SIGNED residual. See this file's note on
    // why it deliberately differs from MetricsCollector's max(C[i]), and why that
    // difference is recorded here rather than fixed there.
    const MULTI& pm = n.otherPMM();
    for (long i = 0; i < pm.N; ++i) r.mbe = std::max(r.mbe, std::fabs(pm.C[i]));

    for (long k = 0; k < n.pCSD()->nPHb; ++k) {
        const double mol = n.Ph_Moles(k);
        if (mol > kPresenceFloor) {
            r.phases.emplace_back(n.pCSD()->PHNL[k], mol);
            ++r.nPh;
        }
    }
    // Sorted by NAME so the record does not depend on the phase ordering in the
    // project file - the same reason the freeze's "# key" line hashes an
    // order-independent set.
    std::sort(r.phases.begin(), r.phases.end(),
              [](const std::pair<std::string, double>& a,
                 const std::pair<std::string, double>& b) { return a.first < b.first; });
    return r;
}

/// Read the results a system file ALREADY CARRIES, without solving anything.
///
/// This is the whole of the "stored" reference, and it needs no JSON parser and
/// no format switch: GEM_init() loads the -dbr into CNode, and unpackDataBr() -
/// which would overwrite it from the solver's own state - is called inside
/// GEM_run(), not GEM_init(). So immediately after init the node HOLDS the file's
/// stored results, and the same accessors the solve path uses read them back:
/// Ph_Moles(k) is CNode->xPH[k] (node2.cpp:812), cVs()/cMs() are CNode->Vs/Ms.
/// That works for every export format the reader supports, text ones included -
/// which matters, because 4 of the corpus's projects are not JSON at all.
///
/// The two fields that do NOT go through an accessor are read from CNode
/// directly and on purpose:
///   Gs        Get_GibbsEnergy() routes through multi_ptr()->TotalGibbsEnergy(),
///             i.e. the SOLVER's state, which after a bare GEM_init has not been
///             unpacked from the file. CNode->Gs is the stored number, and it is
///             the same quantity to every digit - verified on the 18 projects
///             whose stored value still reproduces.
///   status    CNode->NodeStatusCH, recorded so it can be PRINTED. It is a
///             request code, never scored - see scoringFor().
inline bool storedRow(const Case& c, const Mode& m, Row& r)
{
    r = Row{};
    r.project = c.name;
    r.mode    = m.name;

    TNode n;
    if (n.GEM_init(c.lst.c_str())) return false;

    r.status = n.pCNode()->NodeStatusCH;
    r.itg    = n.pCNode()->IterDone;
    r.itf    = 0;                       // no separate MBR count is stored
    r.k2     = 0;                       // nor a phase-selection loop count
    r.G      = n.pCNode()->Gs;
    r.pH     = n.pCNode()->pH;
    r.Eh     = n.pCNode()->Eh;
    r.Vs     = n.cVs();
    r.Ms     = n.cMs();
    r.mbe    = 0.;                      // no residual is stored
    r.ran    = true;

    for (long k = 0; k < n.pCSD()->nPHb; ++k) {
        const double mol = n.Ph_Moles(k);
        if (mol > kPresenceFloor) { r.phases.emplace_back(n.pCSD()->PHNL[k], mol); ++r.nPh; }
    }
    std::sort(r.phases.begin(), r.phases.end(),
              [](const std::pair<std::string, double>& a,
                 const std::pair<std::string, double>& b) { return a.first < b.first; });
    return true;
}

/// True when a system file carries NO stored result - it was exported as a
/// definition, without a calculation.
///
/// This is a DIFFERENT outcome from "the stored result differs", and conflating
/// them makes the fresh-export report actively misleading exactly where it is
/// most needed. Found immediately on first use: `Resources/gems3k-proposed/T10`
/// stores Ms = 0, Vs = 0 and an empty assemblage, and comparing a real solve
/// against those zeros produced five ANSWER findings and a pH finding, none of
/// which mean anything.
///
/// Ms is the discriminator: the reactive mass of any real system is positive, and
/// a file that has been through a solve always carries it. The empty assemblage
/// is required as well so that a genuinely mass-free system, if one ever existed,
/// could not be misread.
inline bool hasNoStoredResult(const Row& r)
{
    return r.Ms <= 0. && r.nPh == 0;
}

/// Discover systems in a directory - the ONE place a scan is correct.
///
/// Everywhere else this file insists on a curated list, because "someone added a
/// project" must not be a CI failure. That argument does not apply here: these
/// are freshly exported systems that by definition have no entry in any list and
/// no frozen row, and enumerating them IS the task. They get costPinnable=false,
/// which is not a policy choice - nobody has measured their jitter.
inline std::vector<Case> scanForSystems(const std::string& dir)
{
    std::vector<Case> out;
    // Glob for *-dat.lst; never derive it from the directory name. Several stems
    // in this corpus disagree with their directory (T8_aq101/T8_aq0101-dat.lst,
    // CASH+_G_csh_sol/csh_sol-dat.lst) - CLAUDE.md s4.
    for (const auto& e : std::filesystem::directory_iterator(dir)) {
        if (!e.is_directory()) continue;
        for (const auto& f : std::filesystem::directory_iterator(e.path())) {
            const std::string p = f.path().string();
            if (p.size() > 8 && p.compare(p.size() - 8, 8, "-dat.lst") == 0) {
                out.push_back({e.path().filename().string(), p, false,
                               "no jitter measurement - freshly exported system",
                               "discovered by scan under " + dir});
                break;
            }
        }
    }
    std::sort(out.begin(), out.end(),
              [](const Case& a, const Case& b) { return a.name < b.name; });
    return out;
}

// ---------------------------------------------------------------------------
// Format
// ---------------------------------------------------------------------------

inline std::string phasesField(const Row& r)
{
    std::string out;
    char buf[64];
    for (size_t i = 0; i < r.phases.size(); ++i) {
        if (i) out += ';';
        std::snprintf(buf, sizeof buf, "%.9e", r.phases[i].second);
        out += r.phases[i].first;
        out += ':';
        out += buf;
    }
    return out.empty() ? "-" : out;
}

/// The provenance block: what this record was produced by, and from what.
///
/// Deliberately more than a timestamp. A frozen record is only reproducible if a
/// reader can tell whether their situation matches the one it was taken in, and
/// the things that move a floating-point answer are the library, the compiler and
/// the platform - so those are named. The tolerances are here too, because they
/// are the record's own contract and reading them out of a header file to
/// interpret a diff is a step nobody takes.
///
/// What is NOT here, and why: a git commit for either repo. It could be plumbed
/// in as a compile definition, but the recorder is normally run from a build
/// directory configured long ago, so the value baked into it would be whatever
/// was checked out THEN - a stale sha recorded as current is worse than no sha at
/// all. The per-project input digests below are what actually pins the corpus.
inline std::string provenance(const std::string& modeSpec, size_t nRows, size_t nProjects)
{
    char when[32] = "unknown";
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::strftime(when, sizeof when, "%Y-%m-%dT%H:%M:%SZ", &tm);

    std::ostringstream o;
    o << "# recorded  " << when << "\n"
      << "# gems3k    " << GEMS3K_VERSION << "   (the version string does NOT track the API - "
                                             "see tools/ci.sh's step 2b)\n"
      << "# built by  "
#if defined(__clang__)
      << "clang " << __clang_major__ << "." << __clang_minor__
#elif defined(__GNUC__)
      << "gcc " << __GNUC__ << "." << __GNUC_MINOR__
#else
      << "unknown compiler"
#endif
      << ", " << (sizeof(void*) * 8) << "-bit"
      << "   (floating-point answers depend on this - the record is not portable "
         "across toolchains until somebody measures that)\n"
      << "# modes     " << modeSpec << "   (the lane's whole mode list is cibase::knownModes(); "
         "it has no Optima mode by design)\n"
      << "# scored    G/Vs/Ms " << kTolG << " rel, phase amounts " << kTolAmt
      << " rel, pH " << kTolPh << " rel, presence floor " << kPresenceFloor << " mol\n"
      << "# recorded-only  EH, MBE, and iteration counts on warm rows and on "
         "jitter-unstable projects\n"
      << "# rows      " << nRows << " over " << nProjects << " projects\n"
      << "#\n"
      << "# The \"# fix\" lines below pin the INPUT FILES each project's rows were produced\n"
      << "# from - the -dat.lst and every file it names, FNV-1a 64. A mismatch means the\n"
      << "# FIXTURE changed, not the solver, and is reported as its own class ranked above\n"
      << "# everything else. Without it a re-export reads as a regression.\n"
      << "#\n";
    return o.str();
}

inline std::string headerLine()
{
    char buf[512];
    std::snprintf(buf, sizeof buf,
                  "%-46s %-7s %6s %5s %6s %4s %-22s %-12s %-12s %-16s %-16s %4s %-11s %s",
                  "PROJECT", "MODE", "STATUS", "ITF", "ITG", "K2",
                  "G", "PH", "EH", "VS", "MS", "NPH", "MBE", "PHASES");
    return buf;
}

inline std::string writeRow(const Row& r)
{
    char buf[512];
    std::snprintf(buf, sizeof buf,
                  "%-46s %-7s %6ld %5ld %6ld %4ld %-22.14e %-12.7f %-12.6f %-16.9e %-16.9e %4d %-11.4e ",
                  r.project.c_str(), r.mode.c_str(), r.status, r.itf, r.itg, r.k2,
                  r.G, r.pH, r.Eh, r.Vs, r.Ms, r.nPh, r.mbe);
    return std::string(buf) + phasesField(r);
}

inline bool parseRow(const std::string& line, Row& r)
{
    if (line.empty() || line[0] == '#') return false;
    std::istringstream ss(line);
    std::string ph;
    if (!(ss >> r.project >> r.mode >> r.status >> r.itf >> r.itg >> r.k2
             >> r.G >> r.pH >> r.Eh >> r.Vs >> r.Ms >> r.nPh >> r.mbe))
        return false;
    if (r.project == "PROJECT") return false;                  // the header line
    // PHASES is the LAST field and is read as the whole remainder of the line,
    // not with >>. GEMS3K phase names contain spaces - "Alkali feldspar" on the
    // solvus projects - so a whitespace-extracted field silently truncates the
    // assemblage to its first word, every recorded phase then parses away, and
    // the check reports a full assemblage change on four projects that had not
    // moved at all. (Found exactly that way on this file's first self-check.)
    std::getline(ss, ph);
    const size_t nb = ph.find_first_not_of(" \t");
    ph = (nb == std::string::npos) ? std::string() : ph.substr(nb);
    if (ph.empty()) return false;
    r.ran = true;
    r.phases.clear();
    if (ph != "-") {
        size_t pos = 0;
        while (pos <= ph.size()) {
            const size_t sep = ph.find(';', pos);
            const std::string tok = ph.substr(pos, sep == std::string::npos
                                                   ? std::string::npos : sep - pos);
            const size_t col = tok.rfind(':');
            if (col != std::string::npos)
                r.phases.emplace_back(tok.substr(0, col), atof(tok.c_str() + col + 1));
            if (sep == std::string::npos) break;
            pos = sep + 1;
        }
    }
    return true;
}

/// Read the "# fix" provenance lines: project -> input digest.
inline std::map<std::string, std::string> readFixtures(const std::string& path)
{
    std::map<std::string, std::string> out;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("# fix ", 0) != 0) continue;
        std::istringstream ss(line.substr(6));
        std::string project, hex;
        if (ss >> project >> hex) out[project] = hex;
    }
    return out;
}

/// Read a recorded file, keyed by "project\tmode".
inline std::map<std::string, Row> readRecord(const std::string& path)
{
    std::map<std::string, Row> out;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        Row r;
        if (parseRow(line, r)) out[r.project + "\t" + r.mode] = r;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Scoring
// ---------------------------------------------------------------------------

/// Relative difference, with an absolute fallback for values at or near zero.
inline double rel(double a, double b)
{
    const double d = std::fabs(a - b);
    const double s = std::max(std::fabs(a), std::fabs(b));
    return s < 1e-30 ? d : d / s;
}

enum class Klass { Fixture, Status, Answer, Cost, Ph, Missing };

struct Finding { Klass klass; std::string project, mode, detail; };

inline const char* klassName(Klass k)
{
    switch (k) {
        case Klass::Fixture: return "FIXTURE";
        case Klass::Status: return "STATUS";
        case Klass::Answer: return "ANSWER";
        case Klass::Cost:   return "COST";
        case Klass::Ph:     return "pH";
        default:            return "MISSING";
    }
}

/// Compare each project's CURRENT input files against the digests a record was
/// taken with. Returns the set of projects whose fixtures have moved; their rows
/// must then be skipped, because a record taken from different inputs cannot
/// score them - and presenting a re-export as a regression is the confusion this
/// whole mechanism exists to remove.
///
/// Shared by the recorder and the gate on purpose: a check that only one of them
/// performs is a check the other silently lacks.
inline std::set<std::string> checkFixtures(const std::vector<Case>& cases,
                                           const std::map<std::string, std::string>& recorded,
                                           std::vector<Finding>& out)
{
    std::set<std::string> moved;
    for (const auto& c : cases) {
        const auto rec = recorded.find(c.name);
        if (rec == recorded.end()) continue;      // record predates digests
        std::string hex; int n = 0;
        if (!fixtureDigest(c.lst, hex, n)) {
            out.push_back({Klass::Fixture, c.name, "-", "input files unreadable"});
            moved.insert(c.name);
        } else if (hex != rec->second) {
            out.push_back({Klass::Fixture, c.name, "-",
                           "input files CHANGED since the record was taken (" + rec->second
                           + " -> " + hex + ") - this is a fixture change, not a solver "
                           "regression; re-record deliberately"});
            moved.insert(c.name);
        }
    }
    return moved;
}

/// Compare one recorded row against one freshly measured row.
///
/// Order matters: a status change short-circuits the ANSWER and COST columns,
/// because on a failed row those numbers are the input file's and comparing them
/// would bury the one real finding under noise.
inline void compareRow(const Case& c, const Row& base, const Row& now,
                       std::vector<Finding>& out, const Scoring& sc = Scoring{})
{
    char buf[512];

    // A reference with no result in it cannot be compared - say that once, rather
    // than emitting an ANSWER finding per column against zeros. Only reachable in
    // stored-reference mode; a frozen row always carries a result.
    if (hasNoStoredResult(base)) {
        out.push_back({Klass::Missing, base.project, base.mode,
                       "the system file carries no stored result (exported without a "
                       "calculation) - nothing to compare against"});
        return;
    }

    if (sc.status && base.status != now.status) {
        std::snprintf(buf, sizeof buf, "status %ld -> %ld  (%s -> %s)",
                      base.status, now.status,
                      statusIsOk(base.status) ? "OK" : "not-OK",
                      statusIsOk(now.status)  ? "OK" : "not-OK");
        out.push_back({Klass::Status, base.project, base.mode, buf});
        return;
    }
    // An agreed failure makes nothing below meaningful - every packed value on a
    // FAIL row is the input file's, GEM_run()'s catch never calling packDataBr().
    // Only consulted when the status is scorable at all; against a stored
    // reference the fresh row's own status still decides this.
    if (!statusIsOk(now.status)) return;

    const std::pair<const char*, std::pair<double, double>> scalars[] = {
        { "G",  { base.G,  now.G  } },
        { "Vs", { base.Vs, now.Vs } },
        { "Ms", { base.Ms, now.Ms } },
    };
    for (const auto& q : scalars) {
        const double d = rel(q.second.first, q.second.second);
        if (d > kTolG) {
            std::snprintf(buf, sizeof buf, "%s %.14e -> %.14e  (rel %.2e > %.0e)",
                          q.first, q.second.first, q.second.second, d, kTolG);
            out.push_back({Klass::Answer, base.project, base.mode, buf});
        }
    }

    // The present-phase NAME SET, then the amounts. The set is checked first and
    // separately because it is the finding no scalar column can produce: a lost
    // trace phase moves G by ~1e-6 RT, inside every tolerance above.
    {
        std::vector<std::string> bn, nn, lost, gained;
        for (const auto& p : base.phases) bn.push_back(p.first);
        for (const auto& p : now.phases)  nn.push_back(p.first);
        std::set_difference(bn.begin(), bn.end(), nn.begin(), nn.end(), std::back_inserter(lost));
        std::set_difference(nn.begin(), nn.end(), bn.begin(), bn.end(), std::back_inserter(gained));
        if (!lost.empty() || !gained.empty()) {
            std::string d = "assemblage";
            for (const auto& p : lost)   d += "  -" + p;
            for (const auto& p : gained) d += "  +" + p;
            d += "   (nPh " + std::to_string(base.nPh) + " -> " + std::to_string(now.nPh) + ")";
            out.push_back({Klass::Answer, base.project, base.mode, d});
        } else {
            std::map<std::string, double> nowAmt;
            for (const auto& p : now.phases) nowAmt[p.first] = p.second;
            for (const auto& p : base.phases) {
                const double d = rel(p.second, nowAmt[p.first]);
                if (d > kTolAmt) {
                    std::snprintf(buf, sizeof buf, "%s  %.9e -> %.9e mol  (rel %.2e > %.0e)",
                                  p.first.c_str(), p.second, nowAmt[p.first], d, kTolAmt);
                    out.push_back({Klass::Answer, base.project, base.mode, buf});
                }
            }
        }
    }

    {
        const double d = rel(base.pH, now.pH);
        if (d > kTolPh) {
            std::snprintf(buf, sizeof buf, "pH %.7f -> %.7f  (rel %.2e > %.0e)",
                          base.pH, now.pH, d, kTolPh);
            out.push_back({Klass::Ph, base.project, base.mode, buf});
        }
    }

    // COST last, and only where a jitter measurement admits it. The warm path has
    // no such measurement at all - see kPinWarmCost.
    bool isWarm = false;
    for (const auto& m : knownModes()) if (m.name == base.mode) isWarm = m.warm;
    const bool pinCost = sc.cost && (isWarm ? kPinWarmCost : c.costPinnable);
    if (pinCost && (base.itf != now.itf || base.itg != now.itg || base.k2 != now.k2)) {
        std::snprintf(buf, sizeof buf,
                      "iterations ITF %ld -> %ld, ITG %ld -> %ld, K2 %ld -> %ld"
                      "   [cost-pinned because native jitter reads: %s]",
                      base.itf, now.itf, base.itg, now.itg, base.k2, now.k2,
                      c.jitter.c_str());
        out.push_back({Klass::Cost, base.project, base.mode, buf});
    }
}

} // namespace cibase
