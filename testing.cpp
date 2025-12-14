#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include "Highs.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include "problem_reader.hpp"

namespace fs = std::filesystem;

// Timer utility for benchmarking
struct Timer {
    std::chrono::high_resolution_clock::time_point time_point;

    // Returns duration in milliseconds
    double stop() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms = end - time_point;
        return ms.count();
    }
    void start() {
        auto t = std::chrono::high_resolution_clock::now();
        time_point = t;
    }
};

HighsLp convertToHighsLp(const Problem_csc& p_csc) {
    HighsLp lp;

    // -----------------------------
    // Dimensions
    // -----------------------------
    lp.num_row_ = p_csc.rows;
    lp.num_col_ = p_csc.cols;

    // -----------------------------
    // Objective: min c^T x
    // -----------------------------
    assert((int)p_csc.c.size() == p_csc.cols);
    lp.col_cost_ = p_csc.c;

    // -----------------------------
    // Variable bounds: x >= 0
    // -----------------------------
    lp.col_lower_.assign(p_csc.cols, 0.0);
    lp.col_upper_.assign(p_csc.cols, kHighsInf);

    // -----------------------------
    // Constraint bounds: Ax = b
    // -----------------------------
    assert((int)p_csc.b.size() == p_csc.rows);
    lp.row_lower_ = p_csc.b;
    lp.row_upper_ = p_csc.b;

    // -----------------------------
    // Matrix A in CSC format
    // -----------------------------
    assert((int)p_csc.col_ptr.size() == p_csc.cols + 1);
    assert((int)p_csc.row_idx.size() == p_csc.nnz);
    assert((int)p_csc.values.size()  == p_csc.nnz);

    lp.a_matrix_.format_ = MatrixFormat::kColwise;
    lp.a_matrix_.start_ = p_csc.col_ptr;
    lp.a_matrix_.index_ = p_csc.row_idx;
    lp.a_matrix_.value_ = p_csc.values;

    // -----------------------------
    // Sanity checks
    // -----------------------------
    assert(lp.a_matrix_.start_[0] == 0);
    assert(lp.a_matrix_.start_[lp.num_col_] == p_csc.nnz);

    for (int j = 0; j < lp.num_col_; ++j)
        assert(lp.a_matrix_.start_[j] <= lp.a_matrix_.start_[j + 1]);

    for (int k = 0; k < p_csc.nnz; ++k) {
        assert(lp.a_matrix_.index_[k] >= 0);
        assert(lp.a_matrix_.index_[k] < lp.num_row_);
        assert(std::isfinite(lp.a_matrix_.value_[k]));
    }

    return lp;
}


double run_once(Highs& highs) {
    auto t0 = std::chrono::high_resolution_clock::now();
    highs.run();
    if (highs.getModelStatus() != HighsModelStatus::kOptimal) {
        std::cout<< "HiGHS did not reach optimality" << std::endl;
        return 1; 
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}


double solveWithHighs(const Problem_csc& prob) {
    Highs highs;

    // Solver settings (fair baseline)
    highs.setOptionValue("solver", "simplex");
    highs.setOptionValue("presolve", "off");
    highs.setOptionValue("threads",1);
    highs.setOptionValue("log_to_console", false);

    // Convert problem
    HighsLp lp = convertToHighsLp(prob);

    // Pass model
    HighsStatus status = highs.passModel(lp);
    if (status != HighsStatus::kOk) {
        std::cout << "Failed to pass model to HiGHS" << std::endl;
        return 1; 
    }

    Timer timer;
    timer.start();
    highs.run(); 
    double elapsed_time = timer.stop();
    std::cout << "Elapsed time: " << elapsed_time << " ms" << std::endl;

    if (highs.getModelStatus() != HighsModelStatus::kOptimal) {
        std::cout<< "HiGHS did not reach optimality" << std::endl;
        return 1; 
    }

    // // Solve
    // std::vector<double> times;

    // for (int i = 0; i < 10; ++i) {
    //     double t = run_once(highs);
    //     times.push_back(t);
    //     highs.clearSolver();
    //     HighsModelStatus model_status = highs.getModelStatus();
    //     if (model_status != HighsModelStatus::kOptimal) {
    //     std::cout << "HiGHS did not find optimal solution" << std::endl;
    //     return 1; 
    //     }
    // }
    // Compute average time
   

    // Check result
  

    // Return objective value
    return highs.getInfo().objective_function_value;
}






/**
 * Runs the solver for a single problem and prints the result.
 * Returns TRUE if the test passed (solvers agree),
 * Returns FALSE if the test failed (solvers disagree).
 */
double run_solver_test(const Problem_csc& p_csc, const std::string& problem_name) {
    std::cout << "Reading problem: " << problem_name << std::endl;
    double objective_value = solveWithHighs(p_csc);
    std::cout << "Objective value: " << std::setprecision(10) << objective_value << std::endl;
    return objective_value; // TEST PASSED


}
   


        
void process_problem_file(const fs::path& path, int& files_processed, int& files_failed) {
    // Check extension
    const std::string extension = path.extension().string();
    if (extension != ".txt" && extension != ".csc") {
        std::cout << "Skipping non-problem file: " << path.string() << std::endl;
        return;
    }

    files_processed++;

    // Run test for this file
    Problem_csc p_csc = read_problem(path.string());
    double objective_value = run_solver_test(p_csc, path.string());
        
}

int main(int argc, char** argv) {
    // Set global logging state once for the entire test run

    // Path to test (can be a file or directory)
    fs::path input_path = "test";
    if (argc > 1) {
        input_path = argv[1];
    }

    std::cout << "Processing path: " << input_path << std::endl;
    std::cout << "---------------------------------" << std::endl;

    int files_processed = 0;
    int files_failed = 0;

    try {
        // Check if the path is a directory
        if (fs::is_directory(input_path)) {
            std::cout << "Path is a directory, iterating..." << std::endl << std::endl;
            for (const auto& entry : fs::directory_iterator(input_path)) {
                // Only process regular files
                if (entry.is_regular_file()) {
                    process_problem_file(entry.path(), files_processed, files_failed);
                }
            }
        }
        // Check if the path is a single file
        else if (fs::is_regular_file(input_path)) {
            std::cout << "Path is a single file, processing..." << std::endl << std::endl;
            process_problem_file(input_path, files_processed, files_failed);
        }
        // Handle cases where the path doesn't exist or isn't a file/directory
        else {
            std::cerr << "Fatal Error: Path is not a valid file or directory: " << input_path
                      << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: Error while processing path: " << e.what() << std::endl;
        return 1;
    }

    // Print a final summary
    std::cout << "---------------------------------" << std::endl;
    std::cout << "Test run complete." << std::endl;
    std::cout << "Processed: " << files_processed << " files" << std::endl;
    std::cout << "Failed:    " << files_failed << " files" << std::endl;

    return (files_failed > 0) ? 1 : 0; // Return error code if any tests failed
}
