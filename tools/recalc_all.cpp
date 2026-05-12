
#ifdef OVERFLOW_EXCEPT
#ifdef __linux__
#include <cfenv>
#elif _MSC_VER
#include <float.h>
#else
#include <cfenv>
#endif
#endif
#include <iostream>
#include "GEMS3K/nodearray.h"
#include "GEMS3K/jsonconfig.h"
#include "difftest/txtfiles.h"
#include "difftest/detail.h"

static int process_task(const std::string& in_lst, const std::string& out_lst)
{
    int nIV = 1; // number of nodes
    std::cout << "Run task:" << in_lst << std::endl;

    // Get inital file format
    GEMS3KGenerator input_data(in_lst);

    // Creates TNodeArray structure instance accessible through the "node_arr" pointer
    std::shared_ptr<TNodeArray> node_arr  = TNodeArray::create(nIV);

    // (1) Initialization of GEMS3K internal data by reading  files
    //     whose names are given in the in_lst
    if(node_arr->GEM_init(in_lst.c_str(), nullptr, nullptr, false)) {
        std::cout << "error occured during reading the files" << std::endl;
        return 1;
    }

    // use default data
    TestModeGEMParam calc_param;
    calc_param.useSIA = '-';
    // (2) re-calculating equilibrium by calling GEMS3K, getting the status back
    if(!node_arr->CalcIPM_List(calc_param, 0, nIV-1, nullptr)) {
        std::cout << "error occured during inital calculation" << std::endl;
        return 1;
    }
    for(int ii=0; ii<nIV; ii++) {
        node_arr->CopyNodeFromTo(ii, nIV, node_arr->pNodT1(), node_arr->pNodT0());
    }

    // (3) Writing results in defined output format
    ProcessProgressFunction messageF = [](const std::string& message, long point) {
        //std::cout <<  message.c_str() << point << std::endl;
        return false;
    };

    auto dbr_list =  node_arr->genGEMS3KInputFiles(out_lst, messageF, nIV, input_data.files_mode(),
                                                  false, true, false, false);

    std::cout <<  "Results: " << out_lst << std::endl;
    return 0;
}

// cmake -DCMAKE_BUILD_TYPE=Debug -fsanitize=thread ..
int main(int argc, char* argv[])
{

#if  defined(OVERFLOW_EXCEPT)
#ifdef __linux__
    feenableexcept (FE_DIVBYZERO|FE_OVERFLOW|FE_UNDERFLOW);
#elif _MSC_VER
    _clearfp();
    _controlfp(_controlfp(0, 0) & ~(_EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW),
               _MCW_EM);
#else

#endif
#endif

    gemsSettings().gems3k_update_loggers(false, "test.log", 3);

    try{
        std::string in_folder = "gems3k";
        std::string out_folder = "out-gems3k";
        std::vector<std::string> dat_lst_files;

        if( argc > 1) {
            in_folder = argv[1];
        }
        if( argc > 2) {
            out_folder = argv[2];
        }

        dat_lst_files = difftest::files_into_directory(in_folder, ".*-dat.lst", true);

        for (const auto& file : dat_lst_files) {
            process_task(file, difftest::regexp_replace(file, in_folder, out_folder));
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

