#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include "difftest/metrics_collector.h"

static std::vector<double> nothing_change(int, double&, double&, const std::vector<double>& b)
{
    return b;
}

std::mt19937 rng(42);
static std::vector<double> perturb_b_randomly(int, double&, double&, const std::vector<double>& b)
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::vector<double> perturbed_b{b};

    for(double& val : perturbed_b) {
        double random_val = dist(rng);
        double perturb = 1.0 + 0.05 * (random_val - 0.5) * 2.0;
        val *= perturb;
    }

    return perturbed_b;
}

static double get_median(std::vector<double> v)
{
    if(v.empty()) {
        return 0.0;
    }
    size_t n = v.size() / 2;

    std::nth_element(v.begin(), v.begin() + n, v.end());
    if (v.size() % 2 != 0) {
        return v[n];
    }
    else {
        auto it = std::max_element(v.begin(), v.begin() + n);
        return (*it + v[n]) / 2.0;
    }
}

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

void to_json(nlohmann::json &j, const Statistics &p) {
    j = nlohmann::json{{"label", p.label},
                       {"run_number", p.run_number},
                       {"min_time_ms", p.min_time_ms},
                       {"max_time_ms", p.max_time_ms},
                       {"mean_time_ms", p.mean_time_ms},
                       {"median_time_ms", p.median_time_ms},
                       {"stddev_time_ms", p.stddev_time_ms}};
}

void from_json(const nlohmann::json &j, Statistics &p) {
    j.at("label").get_to(p.label);
    j.at("run_number").get_to(p.run_number);
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
    result.system_id = path_to_lst;

    if(!init_task(path_to_lst)) {
        // exception or error message
        return result;
    }

    // Calc main task
    process_task(true);
    result.iterations = current_iterations;
    result.convergence = current_convergence;
    result.performance = current_performance;

    // get time statistic
    std::cout <<"get time statistic" << std::endl;
    result.stats.push_back(benchmark("A: same input, warm start", 100, nothing_change, "warm"));
    result.stats.push_back(benchmark("D: same input, cold start", 100, nothing_change, "cold"));
    result.stats.push_back(benchmark("C: composition sweep ±5%, warm", 100, perturb_b_randomly, "warm"));
    result.stats.push_back(benchmark("E: composition sweep ±5%, cold", 100, perturb_b_randomly, "cold"));

    return result;
}

bool MetricsCollector::init_task(const std::string &path_to_lst)
{
    std::cout << "Collect metrics:" << path_to_lst << std::endl;

    // Creates TNode structure instance accessible through the "node" pointer
    node.reset(new TNode());

    // Initialization of GEMS3K internal data by reading  files
    if( node->GEM_init(path_to_lst.c_str()) ) {
        std::cout << "error occured during reading the files: " << path_to_lst << std::endl;
        return false;
    }

    T0 = node->Get_TK();
    P0 = node->Get_P();
    b0.clear();
    for(int ii=0; ii<node->pCSD()->nIC; ++ii) {
        b0.push_back(node->pCNode()->bIC[ii]);
    }
    return true;
}


void MetricsCollector::process_task(bool warmstart)
{
    // Getting direct access to work node DATABR structure which exchanges the
    // data with GEM IPM3 (already filled out by reading the DBR input file)
    node->pCNode()->NodeStatusCH = warmstart ? NEED_GEM_SIA : NEED_GEM_AIA;

    // Re-calculating equilibrium by calling GEMS3K, getting the status back
    auto t1 = std::chrono::high_resolution_clock::now();
    current_convergence.return_status = node->GEM_run(false);
    auto t2 = std::chrono::high_resolution_clock::now();

    // Collect current statistic
    recordIterations(node->otherPMM());
    recordConvergence(node->otherPMM());

    // Calculate duration with double precision in milliseconds
    std::chrono::duration<double, std::milli> run_ms = t2 - t1;
    current_performance.total_time_ms = run_ms.count();
}

Statistics MetricsCollector::benchmark(const std::string &label, int N, fGetInputs perturbf, const std::string &mode)
{
    //std::vector<IterationMetrics> iters;
    //std::vector<ConvergenceMetrics> convs;
    //std::vector<PerformanceMetrics> perfs;
    std::vector<double> times_ms;

    bool warmstart = (mode=="warm");
    std::vector<double> perturbed_b = b0;
    double perturbed_T = T0;
    double perturbed_P = P0;

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < N; ++i) {
        // The composition, T or P sweeps
        perturbed_T = T0;
        perturbed_P = P0;
        perturbed_b = perturbf(i, perturbed_T, perturbed_P, b0);

        // Set temperature and pressure
        node->Set_TK(perturbed_T);
        node->Set_P(perturbed_P);

        // Set the mole amounts of the elements
        for(int ii=0; ii<perturbed_b.size(); ++ii) {
            node->Set_bIC(ii, perturbed_b[ii]);
        }

        // Equilibrate
        process_task(warmstart);

        //iters.push_back(current_iterations);
        //convs.push_back(current_convergence);
        //perfs.push_back(current_performance);
        times_ms.push_back(current_performance.total_time_ms);
    }
    auto t_total = std::chrono::high_resolution_clock::now();

    // get statistics
    Statistics ret_data;
    ret_data.label = label;
    ret_data.run_number = N;

    if (!times_ms.empty()) {
        size_t n = times_ms.size();

        auto minmax = std::minmax_element(times_ms.begin(), times_ms.end());
        ret_data.min_time_ms = *minmax.first;
        ret_data.max_time_ms = *minmax.second;

        double sum = std::accumulate(times_ms.begin(), times_ms.end(), 0.0);
        ret_data.mean_time_ms = sum/n;

        ret_data.median_time_ms = get_median(times_ms);

        double variance_sum = 0.0;
        for(double t : times_ms) {
            variance_sum += (t - ret_data.mean_time_ms) * (t - ret_data.mean_time_ms);
        }
        ret_data.stddev_time_ms = std::sqrt(variance_sum / n);
    }

    return ret_data;
}
