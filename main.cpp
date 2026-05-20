#include <iostream>
#include "difftest/metrics_collector.h"
#include "GEMS3K/jsonconfig.h"
#include "difftest/txtfiles.h"
#include "difftest/detail.h"

int main(int argc, char* argv[])
{

    gemsSettings().gems3k_update_loggers(false, "test.log", 3);

    try {
        std::string in_folder = "gems3k";
        if( argc > 1) {
            in_folder = argv[1];
        }

        auto dat_lst_files = difftest::files_into_directory(in_folder, ".*-dat.lst", true);

        for(const auto& file : dat_lst_files) {
            MetricsCollector task(file);
            BenchmarkResult data = task.getResult();
            nlohmann::json js{data};
            std::cout << js << std::endl;
        }

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
