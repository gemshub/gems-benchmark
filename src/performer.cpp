#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include "GEMS3K/gems3k_version.h"
#include "difftest/performer.h"
#include "difftest/diffkvasjson.h"
#include "difftest/detail.h"

namespace difftest {

ComparisonPerformer::ComparisonPerformer(int argc, char *argv[]):
    command(CopmpareFiles), templ_path(""), source_path(""), compare_method(),
    templ_file(nullptr), source_file(nullptr)
{
    if(extract_args( argc, argv)) {
        command = Help;
        std::cout << "Illegal argument number" << std::endl;
    }
}

int ComparisonPerformer::execute_command()
{
    switch(command)  {
    case ComparisonPerformer::Help:
        show_usage("compare_dirs");
        break;
    case ComparisonPerformer::CopmpareFiles:
        compare_files(templ_path, source_path, std::cout);
        break;
    case ComparisonPerformer::CompareDirectories:
        if( compare_dirs(templ_path, source_path) ) {
            std::cout <<  "Template directory (" << Comparator::templ_name << ") : " <<  templ_path << "\n";
            std::cout <<  "Source directory (" << Comparator::source_name << ") : " <<  source_path << "\n";
            std::cout <<  "No Difference-------------------------------------------------\n";
        }
        break;
    }
    return 0;
}

void ComparisonPerformer::set_path(MainTypes type, const char *path)
{
    if(templ_path.empty()) {
        templ_type = type;
        templ_path = path;
    }
    else {
        source_type = type;
        source_path = path;
    }
}

ComparisonPerformer::MainTypes ComparisonPerformer::file_type(const std::string &file, MainTypes def_type)
{
    if(file.ends_with(".json")) {
        return fJSON;
    }
    else if(file.ends_with(".dat")) {
        return fKeyValue;
    }
    return def_type;
}

bool ComparisonPerformer::update_file(MainTypes ftype, const std::string &path, std::shared_ptr<JsonFile>& file)
{
    if(!path.empty()) {
        auto type = file_type(path, ftype);
        switch(type) {
        case fJSON:
            if(file && typeid(file.get()) == typeid(JsonFile)) {
                file->updatePath(path);
            }
            else {
                file.reset(new JsonFile(path));
                file->updateTemplateDiff(compare_method);
                if(!template_diff_json.empty()) {
                    file->updateTemplateDiff(template_diff_json);
                }
            }
            return true;
        case fKeyValue:
            if(file && typeid(file.get()) == typeid(KeyValueJsonFile)) {
                file->updatePath(path);
            }
            else {
                file.reset(new KeyValueJsonFile(path));
                file->updateTemplateDiff(compare_method);
                if(!template_diff_json.empty()) {
                    file->updateTemplateDiff(template_diff_json);
                }
            }
            return true;
        case fUndef:
            break;
        }
    }
    return false;
}

bool ComparisonPerformer::compare_files(const std::string &ftempl, const std::string &fsource, std::ostream& out)
{
    if( !update_file(templ_type, ftempl, templ_file) ||
        !update_file(source_type, fsource, source_file)) {
        return false;
    }

    if(!templ_file->exist()) {
        std::cout <<  "Template directory (" << Comparator::templ_name << ") : " <<  templ_path << " not exists \n";
        if(!source_file->exist()) {
            return true;
        }
        return false;
    }

    if(!source_file->exist()) {
        std::cout <<  "Source directory (" << Comparator::source_name << ") : " <<  source_path << " not exists \n";
        return false;
    }

    templ_file->load_all();
    source_file->load_all();
    return source_file->compare_to(*templ_file, compare_method, out);
}

bool ComparisonPerformer::compare_dirs(const std::string &dtempl, const std::string &dsource)
{
    bool the_same = true;

    if(!dtempl.empty() && !dsource.empty()) {
        fs::path templ_ps(dtempl);
        fs::path source_ps(dsource);

        if(fs::exists(templ_ps) && fs::exists(source_ps))  {
            std::ofstream log_file(dsource+"/"+"diff-test.log");

            for(auto& p: fs::directory_iterator(templ_ps)) {
                if(fs::is_regular_file(p.path()))  {
                    std::string file = p.path().filename().string();
                    if(file_name_templ.empty() || regexp_test(file, file_name_templ)) {
                        std::cout << "file = " << file << std::endl;
                        auto ret = compare_files( dtempl+"/"+file, dsource+"/"+file, log_file);
                        the_same &= ret;
                    }
                }
            }

            if(use_recursion) {
                for(auto& p: fs::directory_iterator(templ_ps)) {
                    if(fs::is_directory(p.path())) {
                        std::string dir = p.path().filename().string();
                        std::cout << "dir = " << dir << std::endl;
                        the_same &= compare_dirs( dtempl+"/"+dir, dsource+"/"+dir);
                    }
                }
            }
        }
    }

    return the_same;
}

void ComparisonPerformer::show_usage(const std::string &name)
{
    std::cout << "Usage: " << name << " [ option(s) ] -k|-j PATH_TEMPLATE -k|-j PATH_SOURCE"
              << "\nCompare FILES block by block\n"
              << "Options:\n"
              << "\t-h,\t--help\t\t\tshow this help message\n"
              << "\t-f,\t--files \t\t\tcompare two files (default)\n"
              << "\t-d,\t--directories \t\tcompare all files in directories\n"
              << "\t-r,\t--recursive   \t\trecursively compare any subdirectories found\n"
              << "\t-t,\t--file-pattern PAT  \tonly files that match PAT\n\n"
              // file type
              << "\t-k,\t--key-value PATH \tcompare key-value file \n"
              << "\t-j,\t--json      PATH \tcompare json  file\n"
              << "\t-rd,\t--rules-document JSONF  \tstructured document describing data to compare\n\n"
              // method
              << "\t-b,\t--difference   \t\toutput all differences between template and source (default)\n"
              << "\t-i,\t--consist-in   \t\toutput all data from the template not consist in the source\n\n"
              // epsilon
              << "\t-a,\t--approximately EPS\tapproximately equal for floating types\n"
              << "\t-e,\t--essentially EPS\tessentially equal for floating types\n"
              << "\t-at,\t--absolute-tolerance EPS\tabsolute tolerance equal for floating types(default)\n"
              << "\t-sd,\t--significant-digits EPS\tthe number of identical significant equal for floating types\n"
              << "\t-lg,\t--log-of-values EPS\tthe number of identical significant digits of log of values equal for floating types\n"

              << std::endl;
}


int ComparisonPerformer::extract_args(int argc, char* argv[])
{
    int i=0;
    //std::string template_diff_json; // = "template_diff.json"; // default
    std::string eps = std::to_string( std::numeric_limits<double>::epsilon());

    for(i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if((arg == "-h") || (arg == "--help")) {
            command = Help;
            return 0;
        }
        else if((arg == "-f") || (arg == "--files")) {
            command = CopmpareFiles;
        }
        else if((arg == "-d") || (arg == "--directories")) {
            command = CompareDirectories;
        }
        else if((arg == "-r") || (arg == "--recursive")) {
            use_recursion = true;
        }
        else if((arg == "-t") || (arg == "--file-pattern")) {
            if(i + 1 < argc) {
                file_name_templ = argv[++i];
            } else {
                std::cerr << "--file-pattern option requires one argument." << std::endl;
                return 1;
            }
        }
        else if((arg == "-k") || (arg == "--key-value")) {
            if(i + 1 < argc) {
                set_path(fKeyValue, argv[++i]);
            } else {
                std::cerr << "--key-value option requires one argument." << std::endl;
                return 1;
            }
        }
        else if((arg == "-j") || (arg == "--json")) {
            if(i + 1 < argc) {
                set_path(fJSON, argv[++i]);
            } else {
                std::cerr << "--json option requires one argument." << std::endl;
                return 1;
            }
        }
        //"\t-rd,\t--rules-document JSONF  \tstructured document describing data to compare\n\n"
        else if((arg == "-rd") || (arg == "--rules-document")) {
            if (i + 1 < argc) {
                template_diff_json = argv[++i];
            } else {
                std::cerr << "--rules-document option requires one argument." << std::endl;
                return 1;
            }
        }
        else if((arg == "-b") || (arg == "--difference")) {
            compare_method.setMethod( Comparator::Difference );
        }
        else if((arg == "-i") || (arg == "--consist-in")) {
            compare_method.setMethod( Comparator::ConsistIn );
        }
        else if((arg == "-a") || (arg == "--approximately")) {
            if(i + 1 < argc) {
                eps = argv[++i];
            } else {
                std::cerr << "--approximately option requires one argument." << std::endl;
                return 1;
            }
            compare_method.setEpsilon( std::stod(eps), FloatCompareMethod::AprDiffMore );
        }
        else if((arg == "-e") || (arg == "--essentially")) {
            if(i + 1 < argc) {
                eps = argv[++i];
            } else {
                std::cerr << "--essentially option requires one argument." << std::endl;
                return 1;
            }
            compare_method.setEpsilon( std::stod(eps), FloatCompareMethod::RelDiffMore );
        }
        else if((arg == "-at") || (arg == "--absolute-tolerance")) {
            if(i + 1 < argc) {
                eps = argv[++i];
            } else {
                std::cerr << "----absolute-tolerance option requires one argument." << std::endl;
                return 1;
            }
            compare_method.setEpsilon( std::stod(eps), FloatCompareMethod::AbsDiffMore );
        }
        else if((arg == "-sd") || (arg == "--significant-digits")) {
            if(i + 1 < argc) {
                eps = argv[++i];
            } else {
                std::cerr << "--significant-digits option requires one argument." << std::endl;
                return 1;
            }
            compare_method.setEpsilon( std::stod(eps), FloatCompareMethod::SigDigLess );
        }
        else if((arg == "-lg") || (arg == "--log-of-values")) {
            if(i + 1 < argc) {
                eps = argv[++i];
            } else {
                std::cerr << "--log-of-values option requires one argument." << std::endl;
                return 1;
            }
            compare_method.setEpsilon( std::stod(eps), FloatCompareMethod::LogDigLess );
        }
        else {
            while (i + 1 < argc) { // by default we use json file
                set_path(fUndef, argv[++i]);
            }
        }
    }

    if( command != Help && (templ_path.empty() || source_path.empty())) {
        templ_path = "gems3k";
        source_path = templ_path+"-"+GEMS3K_VERSION;
    }

    return 0;
}


} // namespace difftest
