
#pragma once

#include <chrono>
#include <vector>
#include <functional>
#include <tuple>
#include <string>
#include <nlohmann/json.hpp>
#include "GEMS3K/node.h"

using DataTuple = std::tuple<double, double, std::vector<double>>;
using fGetInputs = std::function<DataTuple(int, double, double, const std::vector<double>&)>;


struct IterationMetrics {
    int total_iterations = 0;
    int ipm_iterations = 0;      // pm.IT
    int mbr_iterations = 0;      // pm.ITF
    int global_iterations = 0;   // pm.ITG
    int phase_selection_loops = 0; // pm.K2
};

void to_json(nlohmann::json& j, const IterationMetrics& p);
void from_json(const nlohmann::json& j, IterationMetrics& p);


struct ConvergenceMetrics {
    int return_status = 0;
    int return_code = 0;          // pm.MK
    bool converged = false;       // pm.MK == 1
    double mass_balance_error = 0.0; // max(pm.C[i])
    double dikin_criterion = 0.0; // pm.PCI
};

void to_json(nlohmann::json& j, const ConvergenceMetrics& p);
void from_json(const nlohmann::json& j, ConvergenceMetrics& p);


struct PerformanceMetrics {
    double total_time_ms = 0.0;
    double time_per_iteration_ms = 0.0;
    double calculations_per_second = 0.0;
};

void to_json(nlohmann::json& j, const PerformanceMetrics& p);
void from_json(const nlohmann::json& j, PerformanceMetrics& p);

// Statistics (across multiple runs)
struct Statistics {
    std::string label;
    int run_number = 0;
    double min_time_ms = 0.0;
    double max_time_ms = 0.0;
    double mean_time_ms = 0.0;
    double median_time_ms = 0.0;
    double stddev_time_ms = 0.0;
};


struct BenchmarkResult {
    std::string system_id;
    int run_number = 0;

    IterationMetrics iterations;
    ConvergenceMetrics convergence;
    PerformanceMetrics performance;

    std::vector<Statistics> stats;
};

void to_json(nlohmann::json &j, const Statistics &p);
void from_json(const nlohmann::json &j, Statistics &p);
void to_json(nlohmann::json &j, const BenchmarkResult &p);
void from_json(const nlohmann::json &j, BenchmarkResult &p);

class MetricsCollector {
public:
    MetricsCollector(std::string path, size_t n=100);

    // Results
    BenchmarkResult getResult();

    void set_generator(const std::string& label, fGetInputs func)
    {
        perturb_label = label;
        generate_perturb_set = func;
    }

private:
    std::string path_to_lst;
    size_t N=100;
    std::string perturb_label;
    fGetInputs generate_perturb_set;

    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point stop_time;

    IterationMetrics current_iterations;
    ConvergenceMetrics current_convergence;
    PerformanceMetrics current_performance;

    std::shared_ptr<TNode> node;
    double T0;
    double P0;
    std::vector<double> b0;

    // Data collection
    void recordIterations(const MULTI& pm);
    void recordConvergence(const MULTI& pm);
    void recordPerformance();
    bool init_task(const std::string& path_to_lst);
    void process_task(bool warmstart);
    Statistics benchmark(const std::string& label, int N, const std::vector<DataTuple>& tuple, const std::string& mode="warm");

    void reset();
    // Timing
    void startTimer();
    void stopTimer();
};
