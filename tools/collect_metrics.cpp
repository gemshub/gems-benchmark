#include <iostream>
#include <fstream>
#include <chrono>
#include "GEMS3K/jsonconfig.h"
#include "GEMS3K/gems3k_version.h"
#include "difftest/metrics_collector.h"
#include "difftest/txtfiles.h"

int main(int argc, char* argv[])
{

    gemsSettings().gems3k_update_loggers(false, "test.log", 3);

    try {
        std::string in_folder = "gems3k-";
        in_folder += GEMS3K_VERSION;
        if( argc > 1) {
            in_folder = argv[1];
        }

        auto t0 = std::chrono::high_resolution_clock::now();

        auto dat_lst_files = difftest::files_into_directory(in_folder, ".*-dat.lst", true);

        for(const auto& file : dat_lst_files) {
            GEMS3KGenerator input_data(file);
            MetricsCollector task(file, 10);
            BenchmarkResult data = task.getResult();
            nlohmann::json js{data};
            std::ofstream ostr(input_data.get_dir()+"metrics.json");
            ostr << std::setw(4) << js << std::endl;
        }

        auto t_total = std::chrono::high_resolution_clock::now();
        auto elapsed_mins = std::chrono::duration_cast<std::chrono::minutes>(t_total - t0);

        std::cout << "Elapsed: " << elapsed_mins.count() << " minutes" << std::endl;
        return 0;
    }
    catch(std::exception& e) {
        std::cout << "std::exception: " << e.what() <<  std::endl;
    }
    catch(...) {
        std::cout << "unknown exception" <<  std::endl;
    }
    return 1;
}
