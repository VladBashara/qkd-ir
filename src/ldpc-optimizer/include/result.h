#ifndef RESULT_H
#define RESULT_H


#include "benchmarks.h"

#include <ostream>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>


class Result { 

private: 
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & this->result.bers;
        ar & this->result.fers;
        ar & this->result.fer_std_devs;
    }

public:
    Result() = default;
    Result(benchmarks::BaseBenchmark::RunningResult in_result): result(in_result) {};
    bool operator<(Result const& right) const;

    friend std::ostream &operator<<(std::ostream &stream, Result &obj);

    benchmarks::BaseBenchmark::RunningResult result;
};


Result compute_average_result(std::vector<Result> const& results);

#endif