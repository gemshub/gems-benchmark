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
