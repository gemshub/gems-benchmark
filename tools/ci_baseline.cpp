// ci_baseline - RECORD the CI lane's regression baseline.
//
// The exploratory / recording half of the pair; tests/test_ci_baseline.cpp is the
// asserting half. Both are thin drivers over include/difftest/ci_baseline.h,
// which holds the case list, the row format and the scoring, so the two can never
// drift apart - the same arrangement as solvus_sweep.h / test_solvus.cpp.
//
// STAGE 1 IS NATIVE ONLY (AIA + SIA). No Optima. See the header.
//
//   usage: ci_baseline [--out FILE] [--check FILE] [--project NAME] [--modes LIST]
//
//     (no arguments)   print the record to stdout
//     --out FILE       write the record to FILE - this is how a baseline is taken
//     --check FILE     re-measure and score against FILE, printing the same
//                      report the CTest case prints. Exit 1 on any finding.
//     --project NAME   restrict to one project (substring match), for working on
//                      a single row without re-recording the corpus
//     --against WHAT   `frozen` (default) or `stored` - WHERE THE REFERENCE COMES
//                      FROM. The chain is identical either way: load the system,
//                      solve it, build a row. Only the row it is compared against
//                      changes. `frozen` reads the external table, which is right
//                      for PRE-EXPORTED files whose own stored results are not
//                      current (18 of 62 reproduce). `stored` compares against the
//                      results inside the system files, which is right for a
//                      FRESHLY exported system, where they are current by
//                      construction and no external table can exist yet.
//     --systems DIR    scan DIR for freshly exported systems instead of using the
//                      curated case list. Implies --against stored unless told
//                      otherwise; this is the one place a directory scan is right,
//                      because enumerating unknown systems IS the task.
//     --modes LIST     comma-separated, from the lane's OWN mode list. Default
//                      "native,SIA", which is currently also all there is - the
//                      list lives in cibase::knownModes() and contains no Optima
//                      mode by design. Asking for one is refused by name, not
//                      silently ignored.
//
// RUN IT FROM THE REPOSITORY ROOT. Every path in the case list is repo-relative,
// matching the convention of every other tool and test here - and running the
// solvus benchmark from build-solvus/bin instead reports "Tc = nan, failed = 301"
// and reads exactly like a catastrophic regression when it is a file-open error
// (CLAUDE.md s4).
//
// TAKING A NEW BASELINE IS A DECISION, NOT A FIX. If --check goes red, the
// question is what moved and why. Overwriting the record with --out because the
// numbers changed converts a regression into a silent re-baseline - the exact
// failure mode tools/recheck.py exists to prevent ("a STALE result means a
// recorded number moved: read its note and fix the code or the record - never
// just update the expected value"). Re-record deliberately, in its own commit,
// saying what moved.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "difftest/ci_baseline.h"
#include "GEMS3K/jsonconfig.h"

int main(int argc, char** argv)
{
    std::string out, check, only, systems, against;
    std::string modeSpec = cibase::defaultModeSpec();
    for (int a = 1; a < argc; ++a) {
        if (!std::strcmp(argv[a], "--out")     && a + 1 < argc) out   = argv[++a];
        else if (!std::strcmp(argv[a], "--check")   && a + 1 < argc) check = argv[++a];
        else if (!std::strcmp(argv[a], "--project") && a + 1 < argc) only  = argv[++a];
        else if (!std::strcmp(argv[a], "--modes")   && a + 1 < argc) modeSpec = argv[++a];
        else if (!std::strcmp(argv[a], "--systems") && a + 1 < argc) systems  = argv[++a];
        else if (!std::strcmp(argv[a], "--against") && a + 1 < argc) against  = argv[++a];
        else { std::fprintf(stderr, "usage: ci_baseline [--out FILE] [--check FILE]"
                                    " [--project NAME] [--modes LIST]"
                                    " [--against frozen|stored] [--systems DIR]\n"); return 2; }
    }

    // The reference source. Explicit and never inferred from the data - on this
    // corpus the stored values differ from a fresh solve on 44 of 62 projects, so
    // a tool that guessed "stored" here would emit 44 findings, none of them real.
    // --systems is the single exception, and only because a scanned directory has
    // no frozen rows by definition.
    cibase::Reference ref = cibase::Reference::Frozen;
    if (against == "stored")      ref = cibase::Reference::Stored;
    else if (against == "frozen") ref = cibase::Reference::Frozen;
    else if (!against.empty()) {
        std::fprintf(stderr, "ci_baseline: --against takes 'frozen' or 'stored', not '%s'\n",
                     against.c_str());
        return 2;
    }
    else if (!systems.empty())    ref = cibase::Reference::Stored;

    // No guard on what --against stored may be pointed at: unlike the frozen
    // reference it never needs an external file, because every system carries its
    // own. Pointed at the curated list it answers "does this fixture still
    // reproduce what it was exported with", which for most of this corpus is
    // "no" - see the measurement in Docs/reference/ci-lanes-plan.md s2.
    const cibase::Scoring scoring = cibase::scoringFor(ref);

    std::vector<cibase::Mode> modes;
    std::string merr;
    if (!cibase::modesFromSpec(modeSpec, modes, merr)) {
        std::fprintf(stderr, "ci_baseline: %s\n", merr.c_str());
        return 2;
    }

    // Route GEMS3K's own loggers to a file rather than the terminal, matching
    // tools/recalc_all.cpp and tools/collect_metrics.cpp. Level 3 keeps warnings.
    gemsSettings().gems3k_update_loggers(false, "test.log", 3);

    std::vector<cibase::Row> rows;
    std::vector<cibase::Case> selected;
    std::vector<cibase::Case> pool = systems.empty() ? cibase::cases()
                                                     : cibase::scanForSystems(systems);
    if (!systems.empty())
        std::fprintf(stderr, "ci_baseline: %zu system(s) found under %s\n",
                     pool.size(), systems.c_str());

    // Drop what this checkout does not have, and say so. A missing corpus is not
    // a finding - see availableCases() - but a gate that silently checks a third
    // of its record while looking just as green is the thing to avoid.
    std::map<std::string, int> absentByCorpus;
    const size_t poolTotal = pool.size();
    pool = cibase::availableCases(pool, absentByCorpus);
    const std::string availNote = cibase::availabilityNote(pool.size(), poolTotal, absentByCorpus);
    if (!availNote.empty()) std::fprintf(stderr, "ci_baseline: %s\n", availNote.c_str());
    if (pool.empty()) {
        std::fprintf(stderr, "ci_baseline: no project in the case list exists here - "
                             "run from the repository root\n");
        return 2;
    }
    for (const auto& c : pool) {
        if (!only.empty() && c.name.find(only) == std::string::npos) continue;
        selected.push_back(c);
        for (const auto& m : modes) {
            cibase::Row r = cibase::solveOne(c, m);
            if (!r.ran) {
                std::fprintf(stderr, "ci_baseline: %s [%s] could not be read or run - "
                                     "run from the repository root\n",
                             c.name.c_str(), m.name.c_str());
                return 2;
            }
            rows.push_back(r);
        }
    }
    if (selected.empty()) { std::fprintf(stderr, "ci_baseline: no project matched '%s'\n", only.c_str()); return 2; }

    if (!check.empty() || ref == cibase::Reference::Stored) {
        std::map<std::string, cibase::Row> base;
        if (ref == cibase::Reference::Frozen) {
            base = cibase::readRecord(check);
            if (base.empty()) {
                std::fprintf(stderr, "ci_baseline: no rows read from %s\n", check.c_str());
                return 2;
            }
        }
        if (!scoring.note.empty()) std::printf("  NOTE  %s\n\n", scoring.note.c_str());

        // Input digests first. A project whose fixture files have changed is not
        // comparable against a record taken from the old ones, so it is reported
        // and its rows are skipped - scoring them would present a re-export as a
        // regression, which is the confusion this exists to remove.
        std::map<std::string, std::string> recorded;
        std::set<std::string> movedFixture;
        if (ref == cibase::Reference::Frozen) {
            recorded = cibase::readFixtures(check);
            if (recorded.empty())
                std::printf("  NOTE  this record predates input digests - fixture changes "
                            "cannot be told from solver changes; re-record to gain that\n\n");
        }

        std::vector<cibase::Finding> findings;
        movedFixture = cibase::checkFixtures(selected, recorded, findings);

        size_t i = 0;
        for (const auto& c : selected)
            for (size_t k = 0; k < modes.size(); ++k, ++i) {
                if (movedFixture.count(c.name)) continue;
                cibase::Row want;
                if (ref == cibase::Reference::Stored) {
                    // The system file's own results. Read once per project - the
                    // stored state is a property of the file, not of the mode - and
                    // labelled with this row's mode so the keys line up.
                    if (!cibase::storedRow(c, modes[k], want)) {
                        findings.push_back({cibase::Klass::Missing, c.name, modes[k].name,
                                            "could not read the system's stored results"});
                        continue;
                    }
                } else {
                    const auto it = base.find(rows[i].project + "\t" + rows[i].mode);
                    if (it == base.end()) {
                        findings.push_back({cibase::Klass::Missing, rows[i].project, rows[i].mode,
                                            "no such row in the baseline"});
                        continue;
                    }
                    want = it->second;
                }
                cibase::compareRow(c, want, rows[i], findings, scoring);
            }
        for (const auto& f : findings)
            std::printf("  %-8s %-46s %-7s %s\n", cibase::klassName(f.klass),
                        f.project.c_str(), f.mode.c_str(), f.detail.c_str());
        std::printf("%zu row(s) checked against the %s reference, %zu finding(s)\n",
                    rows.size(), ref == cibase::Reference::Stored ? "STORED" : "frozen",
                    findings.size());
        return findings.empty() ? 0 : 1;
    }

    std::ostream* os = &std::cout;
    std::ofstream file;
    if (!out.empty()) {
        file.open(out);
        if (!file) { std::fprintf(stderr, "ci_baseline: cannot write %s\n", out.c_str()); return 2; }
        os = &file;
    }
    *os << "# gems-benchmark CI baseline - native (AIA) and SIA only.\n"
           "# Recorded by tools/ci_baseline.cpp; checked by tests/test_ci_baseline.cpp\n"
           "# (ctest -L ci). The case list, the scoring and the tolerances all live in\n"
           "# include/difftest/ci_baseline.h - read its header before changing a number here.\n"
           "#\n"
           "# Re-recording this file is a DECISION taken in its own commit, not a way to\n"
           "# turn a red check green.\n"
           "#\n"
        << cibase::provenance(modeSpec, rows.size(), selected.size());

    // One "# fix" line per project, pinning the input files its rows came from.
    for (const auto& c : selected) {
        std::string hex; int n = 0;
        if (cibase::fixtureDigest(c.lst, hex, n))
            *os << "# fix  " << c.name << "  " << hex << "  " << n << " files\n";
        else
            *os << "# fix  " << c.name << "  UNREADABLE\n";
    }
    *os << "#\n" << cibase::headerLine() << "\n";
    for (const auto& r : rows) *os << cibase::writeRow(r) << "\n";
    if (!out.empty()) std::fprintf(stderr, "ci_baseline: wrote %zu rows to %s\n", rows.size(), out.c_str());
    return 0;
}
