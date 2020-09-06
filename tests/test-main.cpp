//
// Created by Matthew Kang on 2020-09-01.
//

#define CATCH_CONFIG_MAIN

#include "csv.hpp"

#include <catch2/catch.hpp>
#include <filesystem>

const auto projectRoot = std::filesystem::current_path().parent_path().parent_path().string();

TEST_CASE("Read tuples with ignored columns")
{
    const auto dataPath = projectRoot + "/data/test.csv";
    auto tups = csv::toTuples<int, float, bool, csv::IGNORE, csv::IGNORE, int>(dataPath);
    const auto& t = tups.front();
    CHECK(std::get<0>(t) == 1);
    CHECK(std::get<2>(t) == true);
    CHECK(std::get<3>(t) == -6);
}

TEST_CASE("Read header")
{
    const auto dataPath = projectRoot + "/data/test_int_with_header.csv";
    auto header = csv::getHeader(dataPath);
    CHECK(header == std::vector<std::string>{"Index", "Age", "Score"});
}

TEST_CASE("Read arrays")
{
    const auto dataPath = projectRoot + "/data/test_float.csv";
    auto arr = csv::toArrays<double, 4>(dataPath);
    CHECK(arr == std::vector<std::array<double, 4>>{
            {3.3l, 25.0, 1.4738, 4789.1},
            {4.18374, 0.48734, 47839247.471890234, 0},
            {9.324123, 3.14159, 6.677, 1234.567890}
    });
}

TEST_CASE("Read arrays with header")
{
    const auto dataPath = projectRoot + "/data/test_int_with_header.csv";
    using Header = std::array<std::string, 3>;
    Header header;
    auto arr = csv::toArrays<int, 3>(dataPath, header);
    CHECK(header == Header{"Index", "Age", "Score"});
}

TEST_CASE("Read vectors")
{
    const auto dataPath = projectRoot + "/data/test_int_with_header.csv";
    using Header = std::vector<std::string>;
    Header header;
    auto arr = csv::toVectors<int>(dataPath, header);
    CHECK(header == Header{"Index", "Age", "Score"});
    CHECK(arr == std::vector<std::vector<int>>{{1, 25, 100}, {2, 38, 87}, {3, 19, 55}});
}

TEST_CASE("Array check")
{
    CHECK(csv::detail::is_array<std::array<int, 4>>::value == true);
    CHECK(csv::detail::is_array<std::vector<int>>::value == false);
}