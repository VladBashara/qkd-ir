#include "result-mt.h"

#include <iostream>
#include <fstream>

double h2(double x); // binary entropy
double performance(size_t m, size_t n, double e); // code performance

int main()
{
    std::vector<double> const ber_vals{0.000000, 0.001000, 0.002000, 0.003000, 0.004000, 0.005000, 0.006000, 0.007000, 0.008000, 0.009000, 0.010000, 0.011000, 0.012000, 0.013000, 0.014000, 0.015000, 0.016000, 0.017000, 0.018000, 0.019000, 0.020000, 0.021000, 0.022000, 0.023000, 0.024000, 0.025000, 0.026000, 0.027000, 0.028000, 0.029000};

    std::ifstream res_file{"opt_process_data.txt"};

    if (!res_file.is_open()) {
        std::cerr << "File not found\n" << std::endl;
        return 1; 
    }

    while (!res_file.eof()) {
        int iter_number{-2};
        res_file >> iter_number;
        std::vector<double> fer_line;
        for (size_t dot_number{0}; dot_number < 30; ++dot_number) {
            double fer_val{0};
            res_file >> fer_val;
            fer_line.push_back(fer_val);
        }
        double intersection_point{find_intersection_point(ber_vals, fer_line, 0.001)};
        double av_ber{(0.005 + 0.01 + 0.02 + 0.04) / 4 + intersection_point};
        std::cout << iter_number << "\t" << intersection_point << "\t" << av_ber << "\t" << performance(22, 44, av_ber) << std::endl;
    }

    return 0;
}


double h2(double x)
{
    return -x * log2(x) - (1 - x) * log2(1 - x);
}


double performance(size_t m, size_t n, double e)
{
    return m / (n * h2(e));
}