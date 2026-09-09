// Sweep ONE independent component across a concentration ladder, holding everything else
// byte-identical, and report what the mass-balance check makes of it at each rung.
//
// Used to tune `pa_MbClassRule` (the per-IC-class mass-balance rule -
// relative tolerance for trace ICs, absolute for major, per Kulik 2013
// App. 2.2).
//
// A ladder built in code rather than from separate GUI exports keeps the
// phase list, T, P and every other setting bit-identical, so the
// concentration is provably the only variable that changes between rungs.
//
// usage: trace_ladder --ic=Fe [--mults=1e3,1,1e-3,...] [--warm] <project-dat.lst>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "GEMS3K/node.h"

int main(int argc, char* argv[])
{
    std::string lst, ic, multList = "1e3,1e1,1,1e-1,1e-3,1e-5";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a.rfind("--ic=",0)==0)    ic       = a.substr(5);
        else if (a.rfind("--mults=",0)==0) multList = a.substr(8);
        else lst = a;
    }
    if (lst.empty() || ic.empty()) {
        std::printf("usage: trace_ladder --ic=NAME [--mults=a,b,c] <project-dat.lst>\n");
        return 2;
    }

    double b0 = 0.; long xIC = -1, nIC = 0;
    {   // probe once just to report the shipped value
        TNode n0;
        if (n0.GEM_init(lst.c_str())) { std::printf("GEM_init failed\n"); return 1; }
        xIC = n0.IC_name_to_xDB(ic);
        if (xIC < 0) { std::printf("IC '%s' not found\n", ic.c_str()); return 1; }
        nIC = n0.pCSD()->nICb;
        b0  = n0.pCNode()->bIC[xIC];
    }
    std::printf("project : %s\nIC      : %s, shipped bIC = %.6e\n\n", lst.c_str(), ic.c_str(), b0);

    std::vector<double> mults;
    { size_t p=0; while(true){ size_t c=multList.find(',',p);
        mults.push_back(atof(multList.substr(p, c==std::string::npos?std::string::npos:c-p).c_str()));
        if(c==std::string::npos) break; p=c+1; } }

    std::printf("%-12s %-14s %-14s %-16s %-14s %-8s %11s %11s %-9s %9s %9s\n",
                "bIC[IC]", "cold", "warm(SIA)", "worst rel resid", "worst abs(mol)", "worst IC",
                "resid H", "resid O", "H/O ratio", "H2(aq)", "O2(aq)");
    for (double m : mults)
    {
        // Fresh node per rung: reusing one TNode would let each rung start from
        // the previous rung's converged state, so the ladder would measure a
        // path rather than a composition.
        TNode node;
        if (node.GEM_init(lst.c_str())) { std::printf("GEM_init failed\n"); return 1; }
        node.pCNode()->bIC[xIC] = b0 * m;

        node.pCNode()->NodeStatusCH = NEED_GEM_AIA;
        long st = node.GEM_run(false);
        const bool coldOK = (st == OK_GEM_AIA || st == BAD_GEM_AIA);
        char cold[32]; std::snprintf(cold, sizeof cold, "%s/%ld",
                                     coldOK ? "ok" : "FAIL", node.pCNode()->IterDone);

        // worst residual by class, computed from public data only
        double worstRel = 0., worstAbs = 0.; long worstIdx = -1;
        if (coldOK) {
            const MULTI& pm = node.otherPMM();
            for (long i = 0; i < nIC; i++) {
                const double b = node.pCNode()->bIC[i];
                double sum = 0.;
                for (long j = 0; j < node.pCSD()->nDCb; j++)
                    sum += node.pCSD()->A[j*node.pCSD()->nIC + i] * node.Get_nDC(j);
                const double abs_ = std::fabs(b - sum);
                const double rel  = b > 0. ? abs_/b : 0.;
                // Both columns report the SAME IC - the one with the worst RELATIVE
                // residual, which is what the mass-balance test keys on. Reporting an
                // independent max for the absolute column invites reading them as one IC.
                if (rel > worstRel) { worstRel = rel; worstIdx = i; worstAbs = abs_; }
            }
            (void)pm;
        }

        // warm restart at the SAME composition - the leg pa_MbClassRule actually changes
        char warm[32] = "-";
        if (coldOK) {
            node.pCNode()->NodeStatusCH = NEED_GEM_SIA;
            long st2 = node.GEM_run(false);
            std::snprintf(warm, sizeof warm, "%s/%ld",
                          (st2==OK_GEM_SIA||st2==BAD_GEM_SIA) ? "ok" : "FAIL",
                          node.pCNode()->IterDone);
        }

        // Signed per-IC residuals for H and O, plus the redox carriers. As the
        // redox-active element vanishes its couple empties, leaving the redox
        // state to trace H2/O2; H and O are stiffly coupled through water's
        // fixed H:O = 2:1 ratio (a near-rank-1 direction in MBR's own matrix),
        // so the H and O residuals are expected to be anti-correlated in a 2:1
        // ratio if that effect dominates.
        double rH = 0., rO = 0.; bool haveH=false, haveO=false;
        for (long i = 0; i < nIC && coldOK; i++) {
            const std::string nm = node.xCH_to_IC_name((int)i);
            if (nm != "H" && nm != "O") continue;
            double sum = 0.;
            for (long j = 0; j < node.pCSD()->nDCb; j++)
                sum += node.pCSD()->A[j*node.pCSD()->nIC + i] * node.Get_nDC(j);
            const double resid = node.pCNode()->bIC[i] - sum;
            if (nm == "H") { rH = resid; haveH = true; } else { rO = resid; haveO = true; }
        }
        double h2 = 0., o2 = 0.;
        for (long j = 0; j < node.pCSD()->nDCb && coldOK; j++) {
            const std::string nm = node.xCH_to_DC_name((int)j);
            if (nm.rfind("H2(aq)",0)==0) h2 = node.Get_nDC(j);
            if (nm.rfind("O2(aq)",0)==0) o2 = node.Get_nDC(j);
        }
        std::printf("%-12.3e %-14s %-14s %-16.3e %-14.3e %-8s %+11.3e %+11.3e %-9.2f %9.2e %9.2e\n",
                    b0*m, cold, warm, worstRel, worstAbs,
                    worstIdx>=0 ? node.xCH_to_IC_name((int)worstIdx).c_str() : "-",
                    haveH?rH:0., haveO?rO:0.,
                    (haveH&&haveO&&rO!=0.)? rH/rO : 0., h2, o2);
    }
    return 0;
}
