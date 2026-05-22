#include <iostream>
#include "difftest/metrics_collector.h"


void to_json(nlohmann::json &j, const IterationMetrics &p) {
    j = nlohmann::json{{"total", p.total_iterations},
                       {"ipm", p.ipm_iterations},
                       {"mbr", p.mbr_iterations},
                       {"global", p.global_iterations},
                       {"phase_loops", p.phase_selection_loops}};
}

void from_json(const nlohmann::json &j, IterationMetrics &p) {
    j.at("total").get_to(p.total_iterations);
    j.at("ipm").get_to(p.ipm_iterations);
    j.at("mbr").get_to(p.mbr_iterations);
    j.at("global").get_to(p.global_iterations);
    j.at("phase_loops").get_to(p.phase_selection_loops);
}

void to_json(nlohmann::json &j, const ConvergenceMetrics &p) {
    j = nlohmann::json{{"return_status", p.return_status},
                       {"return_code", p.return_code},
                       {"converged", p.converged},
                       {"mass_balance_error", p.mass_balance_error},
                       {"dikin_criterion", p.dikin_criterion}};
}

void from_json(const nlohmann::json &j, ConvergenceMetrics &p) {
    j.at("return_status").get_to(p.return_status);
    j.at("return_code").get_to(p.return_code);
    j.at("converged").get_to(p.converged);
    j.at("mass_balance_error").get_to(p.mass_balance_error);
    j.at("dikin_criterion").get_to(p.dikin_criterion);
}

void to_json(nlohmann::json &j, const PerformanceMetrics &p) {
    j = nlohmann::json{{"total_ms", p.total_time_ms},
                       {"per_iteration_ms", p.time_per_iteration_ms},
                       {"per_second", p.calculations_per_second}};
}

void from_json(const nlohmann::json &j, PerformanceMetrics &p) {
    j.at("total_ms").get_to(p.total_time_ms);
    j.at("per_iteration_ms").get_to(p.time_per_iteration_ms);
    j.at("per_second").get_to(p.calculations_per_second);
}

void to_json(nlohmann::json &j, const BenchmarkResult::Statistics &p) {
    j = nlohmann::json{{"min_time_ms", p.min_time_ms},
                       {"max_time_ms", p.max_time_ms},
                       {"mean_time_ms", p.mean_time_ms},
                       {"median_time_ms", p.median_time_ms},
                       {"stddev_time_ms", p.stddev_time_ms}};
}

void from_json(const nlohmann::json &j, BenchmarkResult::Statistics &p) {
    j.at("min_time_ms").get_to(p.min_time_ms);
    j.at("max_time_ms").get_to(p.max_time_ms);
    j.at("mean_time_ms").get_to(p.mean_time_ms);
    j.at("median_time_ms").get_to(p.median_time_ms);
    j.at("stddev_time_ms").get_to(p.stddev_time_ms);
}

void to_json(nlohmann::json &j, const BenchmarkResult &p) {
    j = nlohmann::json{{"system_id", p.system_id},
                       {"run_number", p.run_number},
                       {"iterations", p.iterations},
                       {"convergence", p.convergence},
                       {"performance", p.performance},
                       {"stats", p.stats}};
}

void from_json(const nlohmann::json &j, BenchmarkResult &p) {
    j.at("system_id").get_to(p.system_id);
    j.at("run_number").get_to(p.run_number);
    j.at("iterations").get_to(p.iterations);
    j.at("convergence").get_to(p.convergence);
    j.at("performance").get_to(p.performance);
    j.at("stats").get_to(p.stats);
}

void MetricsCollector::recordIterations(const MULTI &pm)
{
    current_iterations.total_iterations = 0;
    current_iterations.ipm_iterations = pm.IT;
    current_iterations.mbr_iterations = pm.ITF;
    current_iterations.global_iterations = pm.ITG;
    current_iterations.phase_selection_loops = pm.K2;
}

void MetricsCollector::recordConvergence(const MULTI &pm)
{
    current_convergence.return_code = pm.MK;
    current_convergence.converged = pm.MK == 1;
    current_convergence.mass_balance_error = 0;
    for(int ii=0; ii<pm.N; ii++) {
        current_convergence.mass_balance_error = std::max(pm.C[ii], current_convergence.mass_balance_error);
    }
    current_convergence.dikin_criterion = pm.PCI;
}

BenchmarkResult MetricsCollector::getResult()
{
    BenchmarkResult result;
    if(!init_task(path_to_lst)) {
        // exception or error message
        return result;
    }

    // calc main task
    process_task(true);
    result.iterations = current_iterations;
    result.convergence = current_convergence;
    result.performance = current_performance;

    // get time statistic
    // ....

    return result;
}

bool MetricsCollector::init_task(const std::string &path_to_lst)
{
    std::cout << "Test path:" << path_to_lst << std::endl;

    // Creates TNode structure instance accessible through the "node" pointer
    node.reset(new TNode());

    // Initialization of GEMS3K internal data by reading  files
    if( node->GEM_init(path_to_lst.c_str()) ) {
        std::cout << "error occured during reading the files: " << path_to_lst << std::endl;
        return false;
    }
    return true;
}


void MetricsCollector::process_task(bool warmstart)
{
    // Getting direct access to work node DATABR structure which exchanges the
    // data with GEM IPM3 (already filled out by reading the DBR input file)
    node->pCNode()->NodeStatusCH = warmstart ? NEED_GEM_SIA : NEED_GEM_AIA;

    // re-calculating equilibrium by calling GEMS3K, getting the status back
    current_convergence.return_status = node->GEM_run(false);

    // collect current statistic
    recordIterations(node->otherPMM());
    recordConvergence(node->otherPMM());
    //node->read_MULTY(read_f);
    // add time metrics
}

