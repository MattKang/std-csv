#include "csv.hpp"

#include <iostream>

int main()
{
    auto tup = csv::toTuples<int, float, bool, csv::IGNORE, csv::IGNORE, int>("/mnt/c/Users/mattk/develop/std-csv/test.csv");
    std::cout << std::get<1>(tup[1]) << std::endl;
    return 0;
}