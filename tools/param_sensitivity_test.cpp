#include <iostream>
#include <cmath>
#include <chrono>
#include <map>
#include <stdexcept>
#include "GEMS3K/node.h"
#include "GEMS3K/jsonconfig.h"

// Regression test for a class of bug that "no crash" testing cannot catch: a species
// pinned at the numerical floor, completely insensitive to a solution-model interaction
// parameter, while the run otherwise "succeeds" (converges, produces plausible-looking
// output).
//
// Runs the Ti-in-Quartz system (QtzRu solid solution, Rutile end-member) twice with
// different Margules (W, W_T, W_P) interaction parameters for the QtzRu phase and asserts
// the equilibrium mole fraction of the Rutile end-member differs meaningfully between runs.

namespace {

// PMc indices of QtzRu's (W, W_T, W_P), confirmed from this project's own -ipm.json:
// PMc = [0, 0, 60301.17578125, -1.18712449073792, 1.79228889942169] -- fluid_gen occupies
// PMc[0:2] (1 param x 2 coef), QtzRu occupies PMc[2:5] (1 param x 3 coef).
constexpr long int kQtzRuW_xPMc  = 2;
constexpr long int kQtzRuWT_xPMc = 3;
constexpr long int kQtzRuWP_xPMc = 4;

// Reference ("params_A") values, taken verbatim from
// j_TiQ_PRSV_G_MySystem_0_0_10000_900_0-ipm.json PMc[2..4].
constexpr double kParamsA_W  = 60301.17578125;
constexpr double kParamsA_WT = -1.18712449073792;
constexpr double kParamsA_WP = 1.79228889942169;

// Synthetic perturbation ("params_B"): +50% on W only, W_T/W_P unchanged. Not a physical
// recalibration -- purely large enough to prove the test detects sensitivity, well above
// solver/round-off noise (see threshold discussion below).
constexpr double kParamsB_W  = kParamsA_W * 1.5;

constexpr double kRelDiffThreshold = 1e-3;

bool is_converged(long int status)
{
    return status == OK_GEM_AIA || status == OK_GEM_SIA;
}

const char* status_name(long int status)
{
    switch (status) {
        case OK_GEM_AIA:  return "OK_GEM_AIA";
        case OK_GEM_SIA:  return "OK_GEM_SIA";
        case NEED_GEM_AIA: return "NEED_GEM_AIA";
        case NEED_GEM_SIA: return "NEED_GEM_SIA";
        default: return "?";
    }
}

// Resolves the DCH index of the QtzRu-phase "Rutile" endmember robustly, disambiguating
// from the unrelated standalone "Rutile" phase (this dataset has two DCs literally named
// "Rutile"), using two independent methods that must agree.
long int resolve_qtzru_rutile_xCH(TNode& node)
{
    // Method 1 (primary): DC_name_to_xCH_map walks nDCinPH internally and returns one
    // DCH index per phase containing a DC of this name.
    auto by_phase = node.DC_name_to_xCH_map("Rutile");
    auto it = by_phase.find("QtzRu");
    if (it == by_phase.end())
        throw std::runtime_error("Could not find a 'Rutile' species in phase 'QtzRu'");
    long int xCH_primary = it->second;

    // Method 2 (cross-check): locate QtzRu's DC block via PhtoDC_DCH and scan its own
    // DC names.
    long int ph_xCH = node.Ph_name_to_xCH("QtzRu");
    if (ph_xCH < 0)
        throw std::runtime_error("Phase 'QtzRu' not found");
    long int nDCinPh = 0;
    long int first_dc_xCH = node.PhtoDC_DCH(ph_xCH, nDCinPh);
    long int xCH_crosscheck = -1;
    for (long int i = 0; i < nDCinPh; ++i) {
        if (node.xCH_to_DC_name(first_dc_xCH + i) == "Rutile") {
            xCH_crosscheck = first_dc_xCH + i;
            break;
        }
    }
    if (xCH_crosscheck < 0 || xCH_crosscheck != xCH_primary)
        throw std::runtime_error("QtzRu-Rutile index resolution mismatch between "
                                  "DC_name_to_xCH_map and PhtoDC_DCH cross-check");
    return xCH_primary;
}

struct RunResult {
    long int status = 0;
    double target_cDC = 0.0;
    double wall_ms = 0.0;
};

RunResult run_with_params(TNode& node, long int target_xDB, const char* label,
                           double W, double W_T, double W_P)
{
    node.Set_PMc(W,   kQtzRuW_xPMc);
    node.Set_PMc(W_T, kQtzRuWT_xPMc);
    node.Set_PMc(W_P, kQtzRuWP_xPMc);

    // GEM_run() only recalculates when NodeStatusCH is NEED_GEM_AIA/SIA -- must reset
    // before *every* call when reusing one TNode instance.
    node.pCNode()->NodeStatusCH = NEED_GEM_AIA;

    auto t0 = std::chrono::high_resolution_clock::now();
    long int status = node.GEM_run(false);
    auto t1 = std::chrono::high_resolution_clock::now();

    RunResult r;
    r.status = status;
    r.wall_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    long int numK2 = 0, numIterFIA = 0, numIterIPM = 0;
    double calc_time_s = node.GEM_CalcTime(numK2, numIterFIA, numIterIPM);

    std::cout << label << ": status=" << status << " (" << status_name(status) << ")"
              << " wall_ms=" << r.wall_ms
              << " GEM_CalcTime_s=" << calc_time_s
              << " iterFIA=" << numIterFIA << " iterIPM=" << numIterIPM << std::endl;

    if (!is_converged(status)) {
        std::cout << "  error code:    " << node.code_error_IPM() << std::endl;
        std::cout << "  error message: " << node.description_error_IPM() << std::endl;
        return r;
    }

    // Bonus diagnostics: only populated if GEMS3K was built with
    // -DENABLE_BENCHMARK_DIAGNOSTICS=ON; print explicitly rather than silently showing 0.
    const MULTI& pm = node.otherPMM();
    if (pm.SolveTimeMs > 0. || pm.CondNum > 0.) {
        std::cout << "  CondNum=" << pm.CondNum << " SolveTimeMs=" << pm.SolveTimeMs << std::endl;
    } else {
        std::cout << "  (CondNum/SolveTimeMs diagnostics unavailable -- GEMS3K not built "
                     "with ENABLE_BENCHMARK_DIAGNOSTICS)" << std::endl;
    }

    r.target_cDC = node.Get_cDC(target_xDB);
    std::cout << "  cDC(Rutile in QtzRu) = " << r.target_cDC << std::endl;
    return r;
}

} // namespace

int main(int argc, char* argv[])
{
    gemsSettings().gems3k_update_loggers(false, "test.log", 3);

    std::string path_to_lst =
        "gems3k/j_TiQ_PRSV_G_MySystem_0_0_10000_900_0/j_TiQ_PRSV_G_MySystem_0_0_10000_900_0-dat.lst";
    if (argc > 1) {
        path_to_lst = argv[1];
    }

    try {
        std::shared_ptr<TNode> node(new TNode());
        if (node->GEM_init(path_to_lst.c_str())) {
            std::cout << "FAIL: error reading GEMS3K files: " << path_to_lst << std::endl;
            return 1;
        }

        long int target_xCH = resolve_qtzru_rutile_xCH(*node);
        long int target_xDB = node->DC_xCH_to_xDB(target_xCH);
        if (target_xDB < 0) {
            std::cout << "FAIL: QtzRu-Rutile DC not present in the data bridge" << std::endl;
            return 1;
        }

        RunResult a = run_with_params(*node, target_xDB, "params_A",
                                       kParamsA_W, kParamsA_WT, kParamsA_WP);
        if (!is_converged(a.status)) {
            std::cout << "FAIL: params_A run did not converge" << std::endl;
            return 1;
        }

        RunResult b = run_with_params(*node, target_xDB, "params_B",
                                       kParamsB_W, kParamsA_WT, kParamsA_WP);
        if (!is_converged(b.status)) {
            std::cout << "FAIL: params_B run did not converge" << std::endl;
            return 1;
        }

        // Round-trip back to params_A on the SAME node instance: confirms the
        // Set_PMc+GEM_run reuse pattern is stateless/order-independent, and guards
        // against a false PASS caused by hidden hysteresis across repeated runs.
        RunResult a2 = run_with_params(*node, target_xDB, "params_A (repeat)",
                                        kParamsA_W, kParamsA_WT, kParamsA_WP);
        if (!is_converged(a2.status)) {
            std::cout << "FAIL: repeat params_A run did not converge" << std::endl;
            return 1;
        }
        double repeat_rel_diff = std::fabs(a2.target_cDC - a.target_cDC) /
                                  std::max({std::fabs(a.target_cDC), std::fabs(a2.target_cDC), 1e-30});
        if (repeat_rel_diff > kRelDiffThreshold) {
            std::cout << "FAIL: re-running params_A on the same TNode instance did not "
                          "reproduce the first result (rel diff=" << repeat_rel_diff
                       << ") -- reuse pattern may be stateful/order-dependent" << std::endl;
            return 1;
        }

        double rel_diff = std::fabs(b.target_cDC - a.target_cDC) /
                           std::max({std::fabs(a.target_cDC), std::fabs(b.target_cDC), 1e-30});
        std::cout << "Relative difference (params_A vs params_B): " << rel_diff
                   << " (threshold " << kRelDiffThreshold << ")" << std::endl;

        if (rel_diff <= kRelDiffThreshold) {
            std::cout << "FAIL: QtzRu-Rutile equilibrium result is insensitive to the "
                          "interaction-parameter change -- possible numerical-floor pinning bug"
                       << std::endl;
            return 1;
        }

        std::cout << "PASS: parameter sensitivity confirmed" << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cout << "std::exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "unknown exception" << std::endl;
    }
    return 1;
}
