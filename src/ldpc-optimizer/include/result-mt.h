#ifndef RESULT_H
#define RESULT_H


#include "benchmarks.h"

#include <iostream>

double find_intersection_point(std::vector<double> const& x_values, std::vector<double> const& y_values, double line);
bool all_under_line(std::vector<double> const& values, double line);

class Result {
    
public:
    Result() = default;
    Result(benchmarks::BaseBenchmark::RunningResult in_result): result(in_result) {

        double constexpr line{0.001};

        if (bool res1_under_line{all_under_line(this->result.fers, line)}) {
            this->intersection_metric = __DBL_MAX__;
        } else {
            this->intersection_metric = find_intersection_point(this->result.bers, this->result.fers, line);
        }

    };
    bool operator<(Result const& right) const;
    bool operator>(Result const& right) const;

    friend std::ostream &operator<<(std::ostream &stream, Result &obj);

    benchmarks::BaseBenchmark::RunningResult result;

    double intersection_metric = -1.0;
};


Result compute_average_result(std::vector<Result> const& results);

void assign_intersection(Result &res);

#endif