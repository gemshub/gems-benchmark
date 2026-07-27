#include <iostream>
#include "difftest/performer.h"

// -d -r -t ".*-dbr-[\d-]*.*" -rd  dbr_diff.json -a  0.1e-6 -j gems3k -j out-gems3k
int main(int argc, char* argv[])
{
    try {
        difftest::ComparisonPerformer performer(argc, argv);
        return performer.execute_command();
    }
    catch(std::exception& e) {
        std::cerr <<   "std::exception: " << e.what() <<  std::endl;
    }
    catch(...) {
       std::cerr <<  "unknown exception" <<  std::endl;
    }
    return 0;
}


// -f -rd  dbr_diff.json -a  0.1e-6  -j "tst_inf/pHtitr-dbr-0-0000.json"  -j tst_inf/"pHtitr-dbr-0-1.json"
// -f -rd  dbr_diff.json -a  0.1e-6  -k "tst_inf/pHtitr-dbr-0-0000.dat"  -k "tst_inf/pHtitr-dbr-0-1.dat"
// -f -rd  dbr_diff.json -a  0.1e-6  -j "test_dir/pHtitr-dbr-0-0000.json"  -j "test_dir/Calculated-dbr.json"
// -d -r -t ".*-dbr-[\d-]*\.dat" -rd  dbr_diff.json -a  0.1e-6  -k Reactoro/v36 -k Reactoro/reac
