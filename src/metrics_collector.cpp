#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include "difftest/metrics_collector.h"

static DataTuple nothing_change(int, double T, double P, const std::vector<double>& b)
{
    return {T, P, b};
}

std::mt19937 rng(42);
static DataTuple perturb_b_randomly(int, double T, double P, const std::vector<double>& b)
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::vector<double> perturbed_b{b};

    for(double& val : perturbed_b) {
        double random_val = dist(rng);
        double perturb = 1.0 + 0.05 * (random_val - 0.5) * 2.0;
        val *= perturb;
    }

    return {T, P, perturbed_b};
}

static double get_median(std::vector<double> v)
{
    if(v.empty()) {
        return 0.0;
    }
    size_t n = v.size() / 2;

    std::nth_element(v.begin(), v.begin() + n, v.end());
    if(v.size() % 2 != 0) {
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
                       {"converged", p.converged},
                       {"min_time_ms", p.min_time_ms},
                       {"max_time_ms", p.max_time_ms},
                       {"mean_time_ms", p.mean_time_ms},
                       {"median_time_ms", p.median_time_ms},
                       {"stddev_time_ms", p.stddev_time_ms},
                       {"iters_min", p.iters_min},
                       {"iters_max", p.iters_max},
                       {"iters_mean", p.iters_mean},
                       };
}

void from_json(const nlohmann::json &j, Statistics &p) {
    j.at("label").get_to(p.label);
    j.at("run_number").get_to(p.run_number);
    j.at("converged").get_to(p.converged);
    j.at("min_time_ms").get_to(p.min_time_ms);
    j.at("max_time_ms").get_to(p.max_time_ms);
    j.at("mean_time_ms").get_to(p.mean_time_ms);
    j.at("median_time_ms").get_to(p.median_time_ms);
    j.at("stddev_time_ms").get_to(p.stddev_time_ms);
    j.at("iters_min").get_to(p.iters_min);
    j.at("iters_max").get_to(p.iters_max);
    j.at("iters_mean").get_to(p.iters_mean);
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

MetricsCollector::MetricsCollector():
    path_to_lst()
{
    perturb_label = "composition sweep ±5%";
    generate_perturb_set = perturb_b_randomly;
}

BenchmarkResult MetricsCollector::getResult(std::string path, size_t n)
{
    path_to_lst = path;
    N = n;
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

    //Generate an iterable of (T, P, b) tuples (composition, T or P sweeps)
    std::vector<DataTuple> perturb_tuple;
    for(int i = 0; i<N; ++i) {
        perturb_tuple.push_back(generate_perturb_set(i, T0, P0, b0));
    }

    result.stats.push_back(benchmark("A: same input", N, {{T0, P0, b0}}, "warm"));
    result.stats.push_back(benchmark("B: same input", N, {{T0, P0, b0}}, "cold"));
    result.stats.push_back(benchmark("C: "+perturb_label, N, perturb_tuple, "warm"));
    result.stats.push_back(benchmark("D: "+perturb_label, N, perturb_tuple, "cold"));

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

Statistics MetricsCollector::benchmark(const std::string &label, int N, const std::vector<DataTuple>& tuple, const std::string &mode)
{
    size_t convergedN=0;
    std::vector<double> times_ms;
    std::vector<double> iters;

    bool warmstart = (mode=="warm");

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < N; ++i) {
        // The composition, T or P sweeps
        const auto [pert_T, pert_P, pert_b] = tuple[i%tuple.size()];

        // Set temperature and pressure
        node->Set_TK(pert_T);
        node->Set_P(pert_P);

        // Set the mole amounts of the elements
        for(int ii=0; ii<pert_b.size(); ++ii) {
            node->Set_bIC(ii, pert_b[ii]);
        }

        // Equilibrate
        process_task(warmstart);

        auto status = current_convergence.return_status;
        if(status == OK_GEM_AIA || status == OK_GEM_SIA) {
            ++convergedN;
            times_ms.push_back(current_performance.total_time_ms);
            iters.push_back(current_iterations.global_iterations);
        }
        // else {
        //     std::cout << "not convergered " << status << std::endl;
        // }
    }
    auto t_total = std::chrono::high_resolution_clock::now();

    // get statistics
    Statistics ret_data;
    ret_data.label = label;
    ret_data.label += warmstart ? ", warm start" : ", cold start";
    ret_data.run_number = N;
    ret_data.converged = convergedN;

    if(!times_ms.empty()) {
        size_t n = times_ms.size();

        auto minmax = std::minmax_element(times_ms.begin(), times_ms.end());
        ret_data.min_time_ms = *minmax.first;
        ret_data.max_time_ms = *minmax.second;

        minmax = std::minmax_element(iters.begin(), iters.end());
        ret_data.iters_min = *minmax.first;
        ret_data.iters_max = *minmax.second;

        double sum = std::accumulate(times_ms.begin(), times_ms.end(), 0.0);
        ret_data.mean_time_ms = sum/n;

        sum = std::accumulate(iters.begin(), iters.end(), 0.0);
        ret_data.iters_mean = sum/n;

        ret_data.median_time_ms = get_median(times_ms);

        double variance_sum = 0.0;
        for(double t : times_ms) {
            variance_sum += (t - ret_data.mean_time_ms) * (t - ret_data.mean_time_ms);
        }
        ret_data.stddev_time_ms = std::sqrt(variance_sum / n);
    }

    return ret_data;
}
