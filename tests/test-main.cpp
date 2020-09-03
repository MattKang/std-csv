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
    const auto dataPath = projectRoot + "/data/test_ints_with_header.csv";
    auto header = csv::getHeader(dataPath);
    CHECK(header == std::vector<std::string>{"Index", "Age", "Score"});
}

TEST_CASE("Read arrays")
{
    const auto dataPath = projectRoot + "/data/test_ints_with_header.csv";
    using Header = std::array<std::string, 3>;
    Header header;
    auto arr = csv::toArrays<int, 3>(dataPath, header);
    CHECK(header == Header{"Index", "Age", "Score"});
}

TEST_CASE("Read vectors")
{
    const auto dataPath = projectRoot + "/data/test_ints_with_header.csv";
    using Header = std::vector<std::string>;
    Header header;
    auto arr = csv::toVectors<int>(dataPath, header);
    CHECK(header == Header{"Index", "Age", "Score"});
}

TEST_CASE("Array check")
{
    CHECK(csv::detail::is_array<std::array<int, 4>>::value == true);
    CHECK(csv::detail::is_array<std::vector<int>>::value == false);
}