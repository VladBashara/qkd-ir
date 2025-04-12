#ifndef LDPC_OPTIMIZER_RESULT_EXPOSED_H
#define LDPC_OPTIMIZER_RESULT_EXPOSED_H

#include "benchmarks.h"

#include <ostream>


class ExposedResult {

public:
    ExposedResult() = default;
    ExposedResult(benchmarks::ExposedBUSChannellWynersEC::ExposedRunningResult in_result): result(in_result) {};
    bool operator<(ExposedResult const& right) const;

    friend std::ostream &operator<<(std::ostream &stream, ExposedResult &obj);

    benchmarks::ExposedBUSChannellWynersEC::ExposedRunningResult result;
};


ExposedResult compute_average_result(std::vector<ExposedResult> const& results);
double find_intersection_point(std::vector<double> const& x_values, std::vector<double> const& y_values, double line);
std::string exported_result_to_csv(ExposedResult &obj);
#endif //LDPC_OPTIMIZER_RESULT_EXPOSED_H
