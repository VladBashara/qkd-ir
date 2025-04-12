#ifndef UTILS_HPP
#define UTILS_HPP


#include <random>

std::random_device rd;
std::mt19937 gen{rd()};

size_t get_rand_pos_int(size_t from, size_t to)
{
    std::uniform_int_distribution<size_t> distr{from, to};
    return distr(gen);
}


#endif