#include <iostream>
#include <chrono>
#include "GEMS3K/jsonconfig.h"
#include "GEMS3K/gems3k_version.h"
#include "difftest/metrics_collector.h"


int main(int argc, char* argv[])
{
    gemsSettings().gems3k_update_loggers(false, "test1.log", 3);

    try {
        std::string in_folder = "gems3k-";
        in_folder += GEMS3K_VERSION;

        auto t0 = std::chrono::high_resolution_clock::now();

        MetricsCollector task(in_folder, argc, argv);
        auto ret = task.execute_command();

        auto t_total = std::chrono::high_resolution_clock::now();
        auto elapsed_mins = std::chrono::duration_cast<std::chrono::minutes>(t_total - t0);

        std::cout << "Elapsed: " << elapsed_mins.count() << " minutes" << std::endl;
        return ret;
    }
    catch(std::exception& e) {
        std::cout << "std::exception: " << e.what() <<  std::endl;
    }
    catch(...) {
        std::cout << "unknown exception" <<  std::endl;
    }
    return 1;
}
