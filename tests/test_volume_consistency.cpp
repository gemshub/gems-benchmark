// Volume self-consistency across the corpus: checks the identity
// sum_k Ph_Volume(k) == cVs() on the native solve path.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include "difftest/volume_consistency.h"
#include "GEMS3K/node.h"

namespace {

int fails = 0;

void check(bool ok, const std::string& what)
{
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok) ++fails;
}

/// One cold native solve; returns false if the project cannot be solved at all.
bool nativeVolumes(const std::string& lst, double& sum, double& vs)
{
    TNode n;
    if (n.GEM_init(lst.c_str())) return false;
    n.pCNode()->NodeStatusCH = NEED_GEM_AIA;
    const long st = n.GEM_run(false);
    // BAD is accepted: the identity is about how the answer is reported, so it must hold
    // even for a state the solver is not happy with. A thrown failure is not, since
    // GEM_run()'s catch never calls packDataBr(), leaving CNode holding the input file.
    if (st != OK_GEM_AIA && st != BAD_GEM_AIA) return false;
    sum = 0.;
    for (long k = 0; k < n.pCSD()->nPHb; ++k) sum += n.Ph_Volume(k);
    vs = n.cVs();
    return vs > 0.;
}

} // namespace

int main()
{
    std::cout << "\n=== volume self-consistency: sum(Ph_Volume) == cVs(), native ===\n\n";

    for (const auto& c : volcons::volumeCases()) {
        double sum = 0., vs = 0.;
        if (!nativeVolumes(c.lst, sum, vs)) {
            std::cout << "  FAIL  " << c.name << ": native solve did not produce a state\n";
            ++fails;
            continue;
        }
        const double ratio = sum / vs;
        std::printf("  %-18s sum=%.9e  Vs=%.9e  ratio=%.6f%s%s\n",
                    c.name.c_str(), sum, vs, ratio,
                    c.why.empty() ? "" : "   <- ", c.why.c_str());
        if (c.pinsKnownDeviation)
            check(ratio >= c.lo && ratio <= c.hi,
                  c.name + ": known deviation still the recorded size"
                           " (if this fails it may be FIXED - see the header)");
        else
            check(ratio >= c.lo && ratio <= c.hi,
                  c.name + ": per-phase volumes sum to the reported total");
    }

    std::cout << "\n" << (fails ? "FAILED - " + std::to_string(fails) + " failed check(s)"
                                : "OK - all checks passed") << "\n";
    return fails ? 1 : 0;
}
